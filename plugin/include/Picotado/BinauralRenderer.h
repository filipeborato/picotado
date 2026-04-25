#pragma once

#include <atomic>
#include <vector>

#include <juce_core/juce_core.h>

struct MYSOFA_EASY;

namespace picotado {

/**
 * Wrapper around libmysofa for SOFA-based HRTF lookup.
 *
 * loadSOFA / unloadSOFA are intended to be called from the message
 * thread. getHRIR is called from the audio thread and uses tryEnter on
 * the internal lock — if the message thread is mid-load, the audio
 * thread receives an identity (silent) IR for that grain spawn.
 */
class BinauralRenderer {
 public:
  BinauralRenderer();
  ~BinauralRenderer();

  void prepare(double sampleRate);

  bool loadSOFA(const juce::File& file);
  void unloadSOFA();

  bool hasHRTF() const noexcept { return easy_.load() != nullptr; }
  int getFilterLength() const noexcept { return filterLength_.load(); }
  juce::String getLoadedFileName() const { return loadedName_; }

  /**
   * Fill irL / irR with the HRIR pair for the given direction.
   *
   * azimuth: radians, 0 = front, +pi/2 = left, -pi/2 = right
   * elevation: radians, 0 = horizontal, +pi/2 = above
   * outCapacity: caller-side buffer size; written length is min(outCapacity, getFilterLength())
   */
  void getHRIR(float azimuthRad, float elevationRad, float* irL, float* irR,
               int outCapacity);

 private:
  double sampleRate_ = 48000.0;
  std::atomic<int> filterLength_{0};
  std::atomic<MYSOFA_EASY*> easy_{nullptr};
  juce::String loadedName_;
  juce::CriticalSection sofaLock_;
  std::vector<float> scratchL_;
  std::vector<float> scratchR_;

  JUCE_DECLARE_NON_COPYABLE(BinauralRenderer)
};

}  // namespace picotado
