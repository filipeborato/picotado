#include "Picotado/BinauralRenderer.h"

#include <cmath>
#include <cstring>

#include <mysofa.h>

namespace picotado {

BinauralRenderer::BinauralRenderer() = default;

BinauralRenderer::~BinauralRenderer() {
  unloadSOFA();
}

void BinauralRenderer::prepare(double sampleRate) {
  sampleRate_ = sampleRate;
  // If a SOFA was already loaded, reload it at the new sample rate.
  // (libmysofa resamples at open time.)
  // We only attempt this if we know the original file path.
  if (auto* old = easy_.load(); old != nullptr && loadedName_.isNotEmpty()) {
    juce::File f{loadedName_};
    if (f.existsAsFile()) {
      loadSOFA(f);
    }
  }
}

bool BinauralRenderer::loadSOFA(const juce::File& file) {
  if (!file.existsAsFile()) {
    return false;
  }

  int err = 0;
  int filterLen = 0;
  auto* newEasy = mysofa_open(file.getFullPathName().toRawUTF8(),
                              static_cast<float>(sampleRate_), &filterLen,
                              &err);
  if (err != 0 || newEasy == nullptr) {
    return false;
  }

  MYSOFA_EASY* old = nullptr;
  {
    const juce::ScopedLock sl(sofaLock_);
    old = easy_.exchange(newEasy);
    filterLength_.store(filterLen);
    scratchL_.assign(static_cast<size_t>(filterLen), 0.0f);
    scratchR_.assign(static_cast<size_t>(filterLen), 0.0f);
    loadedName_ = file.getFullPathName();
  }
  if (old != nullptr) {
    mysofa_close(old);
  }
  return true;
}

void BinauralRenderer::unloadSOFA() {
  MYSOFA_EASY* old = nullptr;
  {
    const juce::ScopedLock sl(sofaLock_);
    old = easy_.exchange(nullptr);
    filterLength_.store(0);
    loadedName_.clear();
  }
  if (old != nullptr) {
    mysofa_close(old);
  }
}

void BinauralRenderer::getHRIR(float azimuthRad, float elevationRad, float* irL,
                               float* irR, int outCapacity) {
  // Default to identity (single-sample bypass) so callers always have valid IRs.
  for (int i = 0; i < outCapacity; ++i) {
    irL[i] = 0.0f;
    irR[i] = 0.0f;
  }
  if (outCapacity > 0) {
    irL[0] = 1.0f;
    irR[0] = 1.0f;
  }

  const juce::ScopedTryLock stl(sofaLock_);
  if (!stl.isLocked()) {
    return;  // load in flight; identity IR is fine for one grain
  }
  auto* easy = easy_.load();
  if (easy == nullptr) {
    return;
  }

  const int actualLen = filterLength_.load();
  if (actualLen <= 0) {
    return;
  }
  if (static_cast<int>(scratchL_.size()) < actualLen) {
    return;  // shouldn't happen; loadSOFA sizes the scratch
  }

  // libmysofa Cartesian: x forward, y left, z up.
  const float r = 1.0f;
  const float ce = std::cos(elevationRad);
  const float x = r * ce * std::cos(azimuthRad);
  const float y = r * ce * std::sin(azimuthRad);
  const float z = r * std::sin(elevationRad);

  float delayL = 0.0f;
  float delayR = 0.0f;
  mysofa_getfilter_float(easy, x, y, z, scratchL_.data(), scratchR_.data(),
                         &delayL, &delayR);

  const int copyLen = juce::jmin(outCapacity, actualLen);
  std::memcpy(irL, scratchL_.data(), static_cast<size_t>(copyLen) * sizeof(float));
  std::memcpy(irR, scratchR_.data(), static_cast<size_t>(copyLen) * sizeof(float));
}

}  // namespace picotado
