#include "Picotado/GranularEngine.h"

#include <cmath>

#include "Picotado/BinauralRenderer.h"

namespace picotado {

GranularEngine::GranularEngine() = default;

void GranularEngine::prepare(double sampleRate, int /*maxBlockSize*/,
                             int /*numInputChannels*/) {
  sampleRate_ = sampleRate;
  captureSize_ = static_cast<int>(sampleRate * MAX_CAPTURE_SECONDS);
  captureBuffer_.setSize(1, captureSize_);
  captureBuffer_.clear();
  captureWritePos_ = 0;
  grainAccumulator_ = 0.0;
  for (auto& g : grains_) {
    g.active = false;
  }
}

void GranularEngine::reset() {
  captureBuffer_.clear();
  captureWritePos_ = 0;
  grainAccumulator_ = 0.0;
  for (auto& g : grains_) {
    g.active = false;
  }
  activeGrainCount_.store(0);
}

void GranularEngine::process(juce::AudioBuffer<float>& buffer) {
  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();
  if (numSamples == 0 || numChannels == 0 || captureSize_ <= 0) {
    return;
  }

  // 1) Capture input -> mono ring buffer (paused on freeze)
  if (!frozen_.load()) {
    auto* capture = captureBuffer_.getWritePointer(0);
    const float invCh = 1.0f / static_cast<float>(numChannels);
    for (int i = 0; i < numSamples; ++i) {
      float monoIn = 0.0f;
      for (int ch = 0; ch < numChannels; ++ch) {
        monoIn += buffer.getSample(ch, i);
      }
      monoIn *= invCh;
      capture[captureWritePos_] = monoIn;
      captureWritePos_ = (captureWritePos_ + 1) % captureSize_;
    }
  }

  // 2) Stash dry copy for dry/wet mix
  juce::AudioBuffer<float> dry;
  dry.makeCopyOf(buffer);

  // 3) Output starts silent; we accumulate wet grains into it
  buffer.clear();

  // 4) Schedule grain spawns by density (sample-accurate accumulator)
  const float density = juce::jmax(0.001f, densityHz_.load());
  const double samplesPerGrain = sampleRate_ / static_cast<double>(density);
  grainAccumulator_ += static_cast<double>(numSamples);

  // Cap spawns per block to avoid pathological loops if density is huge
  int spawnsThisBlock = 0;
  constexpr int kMaxSpawnsPerBlock = 8;
  while (grainAccumulator_ >= samplesPerGrain &&
         spawnsThisBlock < kMaxSpawnsPerBlock) {
    spawnGrain();
    grainAccumulator_ -= samplesPerGrain;
    ++spawnsThisBlock;
  }
  if (grainAccumulator_ >= samplesPerGrain) {
    // density faster than block can handle; clamp leftover to prevent runaway
    grainAccumulator_ = std::fmod(grainAccumulator_, samplesPerGrain);
  }

  // 5) Mix all live grains into output
  int active = 0;
  for (auto& g : grains_) {
    if (g.active) {
      mixGrain(g, buffer);
      if (g.active) {
        ++active;
      }
    }
  }
  activeGrainCount_.store(active);

  // 6) Dry/wet crossfade (we treat the engine output as the wet bus)
  const float wet = juce::jlimit(0.0f, 1.0f, dryWet_.load());
  const float dryGain = 1.0f - wet;
  const int outChannels = juce::jmin(buffer.getNumChannels(), 2);
  for (int ch = 0; ch < outChannels; ++ch) {
    auto* out = buffer.getWritePointer(ch);
    const auto* d = dry.getReadPointer(juce::jmin(ch, dry.getNumChannels() - 1));
    for (int i = 0; i < numSamples; ++i) {
      out[i] = wet * out[i] + dryGain * d[i];
    }
  }
}

void GranularEngine::spawnGrain() {
  Grain* slot = nullptr;
  for (auto& g : grains_) {
    if (!g.active) {
      slot = &g;
      break;
    }
  }
  if (slot == nullptr) {
    return;  // pool exhausted; drop the spawn
  }

  Grain& g = *slot;

  // Duration + symmetrical Tukey ramps (~12.5% of grain on each side, min 5ms)
  const float sizeMs = juce::jlimit(5.0f, MAX_GRAIN_SECONDS * 1000.0f,
                                    grainSizeMs_.load());
  g.durationSamples =
      juce::jmax(1, static_cast<int>(sizeMs * 0.001f * sampleRate_));
  const int minRamp = static_cast<int>(0.005f * sampleRate_);
  g.attackSamples = juce::jmin(g.durationSamples / 2,
                               minRamp + g.durationSamples / 8);
  g.releaseSamples = g.attackSamples;
  g.sampleIndex = 0;

  // Pitch (semitone randomization on top of base ratio)
  float ratio = pitchRatio_.load();
  const float rand = pitchRandomSemis_.load();
  if (rand > 0.0f) {
    const float semis = (random_.nextFloat() * 2.0f - 1.0f) * rand;
    ratio *= std::pow(2.0f, semis / 12.0f);
  }
  g.pitchRatio = juce::jlimit(0.25f, 4.0f, ratio);

  // Read position: walk back from write head by (offset + uniform random)
  const float offsetMs = juce::jmax(0.0f, positionOffsetMs_.load());
  const float randMs = juce::jmax(0.0f, positionRandomMs_.load());
  const float chosenOffsetMs = offsetMs + random_.nextFloat() * randMs;
  const int offsetSamples =
      static_cast<int>(chosenOffsetMs * 0.001f * sampleRate_);
  int start = captureWritePos_ - offsetSamples;
  while (start < 0) {
    start += captureSize_;
  }
  start %= captureSize_;
  g.readPosition = static_cast<double>(start);

  // Spatial direction
  const float spread =
      juce::jlimit(0.0f, juce::MathConstants<float>::pi, spreadRad_.load());
  const float az = (random_.nextFloat() * 2.0f - 1.0f) * spread;
  const float el = 0.0f;

  // Per-grain HRIR (or ITD+ILD fallback)
  if (binaural_ != nullptr && binaural_->hasHRTF()) {
    g.useHrtf = true;
    g.hrirLength = juce::jmin(MAX_HRIR_LENGTH, binaural_->getFilterLength());
    binaural_->getHRIR(az, el, g.hrirL.data(), g.hrirR.data(), MAX_HRIR_LENGTH);
    g.history.fill(0.0f);
    g.historyIndex = 0;
  } else {
    g.useHrtf = false;
    constexpr float headRadiusMeters = 0.0875f;
    constexpr float speedOfSound = 343.0f;
    const float maxItd =
        (headRadiusMeters / speedOfSound) * static_cast<float>(sampleRate_);
    const float itd = std::sin(az) * maxItd;
    g.itdSamplesL = juce::jlimit(
        0, MAX_HRIR_LENGTH - 1, static_cast<int>(juce::jmax(0.0f, -itd)));
    g.itdSamplesR = juce::jlimit(
        0, MAX_HRIR_LENGTH - 1, static_cast<int>(juce::jmax(0.0f, itd)));
    const float pan = 0.5f + 0.5f * std::sin(az);
    g.ildL = std::cos(pan * juce::MathConstants<float>::halfPi);
    g.ildR = std::sin(pan * juce::MathConstants<float>::halfPi);
    g.history.fill(0.0f);
    g.historyIndex = 0;
    g.hrirLength = 0;
  }

  g.active = true;
}

void GranularEngine::mixGrain(Grain& g, juce::AudioBuffer<float>& output) {
  const auto* capture = captureBuffer_.getReadPointer(0);
  const int numSamples = output.getNumSamples();
  const int outChannels = output.getNumChannels();
  auto* outL = output.getWritePointer(0);
  auto* outR = (outChannels > 1) ? output.getWritePointer(1) : nullptr;

  const int hrirLen = g.hrirLength;
  const float* hrirL = g.hrirL.data();
  const float* hrirR = g.hrirR.data();

  for (int i = 0; i < numSamples; ++i) {
    if (g.sampleIndex >= g.durationSamples) {
      g.active = false;
      break;
    }

    // Linear-interpolated source readout from the ring buffer
    const double readPos = g.readPosition;
    int idx0 = static_cast<int>(readPos);
    int idx1 = idx0 + 1;
    const double frac = readPos - static_cast<double>(idx0);
    idx0 %= captureSize_;
    if (idx0 < 0) idx0 += captureSize_;
    idx1 %= captureSize_;
    if (idx1 < 0) idx1 += captureSize_;
    const float s0 = capture[idx0];
    const float s1 = capture[idx1];
    float src = static_cast<float>((1.0 - frac) * s0 + frac * s1);

    // Tukey envelope (linear ramp; cheap & smooth enough for grains)
    float env = 1.0f;
    if (g.sampleIndex < g.attackSamples) {
      env = static_cast<float>(g.sampleIndex) /
            static_cast<float>(juce::jmax(1, g.attackSamples));
    } else if (g.sampleIndex >= g.durationSamples - g.releaseSamples) {
      env = static_cast<float>(g.durationSamples - g.sampleIndex) /
            static_cast<float>(juce::jmax(1, g.releaseSamples));
    }
    src *= env;

    if (g.useHrtf && hrirLen > 0) {
      g.history[g.historyIndex] = src;
      float l = 0.0f;
      float r = 0.0f;
      int idx = g.historyIndex;
      for (int k = 0; k < hrirLen; ++k) {
        const float h = g.history[idx];
        l += hrirL[k] * h;
        r += hrirR[k] * h;
        --idx;
        if (idx < 0) {
          idx = MAX_HRIR_LENGTH - 1;
        }
      }
      g.historyIndex = (g.historyIndex + 1) % MAX_HRIR_LENGTH;
      outL[i] += l;
      if (outR != nullptr) {
        outR[i] += r;
      }
    } else {
      g.history[g.historyIndex] = src;
      const int idxL = (g.historyIndex - g.itdSamplesL + MAX_HRIR_LENGTH) %
                       MAX_HRIR_LENGTH;
      const int idxR = (g.historyIndex - g.itdSamplesR + MAX_HRIR_LENGTH) %
                       MAX_HRIR_LENGTH;
      const float l = g.history[idxL] * g.ildL;
      const float r = g.history[idxR] * g.ildR;
      g.historyIndex = (g.historyIndex + 1) % MAX_HRIR_LENGTH;
      outL[i] += l;
      if (outR != nullptr) {
        outR[i] += r;
      }
    }

    g.readPosition += g.pitchRatio;
    while (g.readPosition >= captureSize_) g.readPosition -= captureSize_;
    while (g.readPosition < 0.0) g.readPosition += captureSize_;
    ++g.sampleIndex;
  }
}

}  // namespace picotado
