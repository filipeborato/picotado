#pragma once

#include <array>
#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "Grain.h"

namespace picotado {

class BinauralRenderer;

/**
 * Live-input granular engine with per-grain HRTF/ITD spatialisation.
 *
 * Audio thread API: prepare(), reset(), process(buffer). Parameter setters
 * are atomic and lock-free. Each block, we:
 *   1. mix incoming buffer to mono and write into a circular capture
 *      buffer (paused when freeze is on);
 *   2. spawn new grains based on density (sample-accurate accumulator);
 *   3. for every active grain, advance its varispeed source readout,
 *      apply Tukey envelope, then either FIR-convolve with the per-grain
 *      HRIR pair or apply ITD+ILD (fallback) and mix into output L/R;
 *   4. crossfade with dry input via dry/wet.
 */
class GranularEngine {
 public:
  static constexpr int MAX_GRAINS = 64;
  static constexpr float MAX_CAPTURE_SECONDS = 10.0f;
  static constexpr float MAX_GRAIN_SECONDS = 0.5f;

  GranularEngine();

  void prepare(double sampleRate, int maxBlockSize, int numInputChannels);
  void reset();

  void process(juce::AudioBuffer<float>& buffer);

  void setGrainSizeMs(float ms) noexcept { grainSizeMs_.store(ms); }
  void setDensityHz(float hz) noexcept { densityHz_.store(hz); }
  void setPitchRatio(float ratio) noexcept { pitchRatio_.store(ratio); }
  void setPitchRandomSemis(float semis) noexcept {
    pitchRandomSemis_.store(semis);
  }
  void setPositionOffsetMs(float ms) noexcept { positionOffsetMs_.store(ms); }
  void setPositionRandomMs(float ms) noexcept { positionRandomMs_.store(ms); }
  void setSpreadRad(float rad) noexcept { spreadRad_.store(rad); }
  void setFreeze(bool f) noexcept { frozen_.store(f); }
  void setDryWet(float w) noexcept { dryWet_.store(w); }

  void setBinauralRenderer(BinauralRenderer* r) noexcept { binaural_ = r; }

  int getActiveGrainCount() const noexcept { return activeGrainCount_.load(); }

 private:
  void spawnGrain();
  void mixGrain(Grain& g, juce::AudioBuffer<float>& output);

  double sampleRate_ = 48000.0;
  int captureSize_ = 0;
  juce::AudioBuffer<float> captureBuffer_;
  int captureWritePos_ = 0;

  std::array<Grain, MAX_GRAINS> grains_{};
  double grainAccumulator_ = 0.0;

  juce::Random random_;
  BinauralRenderer* binaural_ = nullptr;

  std::atomic<float> grainSizeMs_{80.0f};
  std::atomic<float> densityHz_{20.0f};
  std::atomic<float> pitchRatio_{1.0f};
  std::atomic<float> pitchRandomSemis_{0.0f};
  std::atomic<float> positionOffsetMs_{50.0f};
  std::atomic<float> positionRandomMs_{50.0f};
  std::atomic<float> spreadRad_{juce::MathConstants<float>::halfPi};
  std::atomic<bool> frozen_{false};
  std::atomic<float> dryWet_{0.7f};

  std::atomic<int> activeGrainCount_{0};

  JUCE_DECLARE_NON_COPYABLE(GranularEngine)
};

}  // namespace picotado
