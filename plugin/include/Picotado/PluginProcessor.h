#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "BinauralRenderer.h"
#include "GranularEngine.h"

namespace picotado {

class PicotadoAudioProcessor : public juce::AudioProcessor {
 public:
  PicotadoAudioProcessor();
  ~PicotadoAudioProcessor() override = default;

  void prepareToPlay(double newSampleRate, int newSamplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.5; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState& getState() noexcept { return state_; }
  BinauralRenderer& getBinauralRenderer() noexcept { return binaural_; }
  GranularEngine& getEngine() noexcept { return engine_; }

  // For UI metering (read from the message thread)
  std::atomic<int> activeGrains{0};

 private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createLayout(
      PicotadoAudioProcessor&);

  juce::AudioProcessorValueTreeState state_;

  GranularEngine engine_;
  BinauralRenderer binaural_;

  juce::AudioParameterFloat* gain_{nullptr};
  juce::AudioParameterFloat* dryWet_{nullptr};
  juce::AudioParameterFloat* grainSize_{nullptr};
  juce::AudioParameterFloat* density_{nullptr};
  juce::AudioParameterFloat* pitch_{nullptr};
  juce::AudioParameterFloat* pitchRand_{nullptr};
  juce::AudioParameterFloat* position_{nullptr};
  juce::AudioParameterFloat* positionRand_{nullptr};
  juce::AudioParameterFloat* spread_{nullptr};
  juce::AudioParameterBool* freeze_{nullptr};
  juce::AudioParameterBool* bypass_{nullptr};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PicotadoAudioProcessor)
};

}  // namespace picotado
