#pragma once

#include <memory>
#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

namespace picotado {

class PicotadoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer {
 public:
  explicit PicotadoAudioProcessorEditor(PicotadoAudioProcessor&);
  ~PicotadoAudioProcessorEditor() override;

  void resized() override;
  void timerCallback() override;

 private:
  using Resource = juce::WebBrowserComponent::Resource;
  std::optional<Resource> getResource(const juce::String& url) const;
  void loadSofaFunction(
      const juce::Array<juce::var>& args,
      juce::WebBrowserComponent::NativeFunctionCompletion completion);
  void unloadSofaFunction(
      const juce::Array<juce::var>& args,
      juce::WebBrowserComponent::NativeFunctionCompletion completion);

  PicotadoAudioProcessor& processorRef_;

  juce::WebSliderRelay gainRelay_;
  juce::WebSliderRelay dryWetRelay_;
  juce::WebSliderRelay grainSizeRelay_;
  juce::WebSliderRelay densityRelay_;
  juce::WebSliderRelay pitchRelay_;
  juce::WebSliderRelay pitchRandRelay_;
  juce::WebSliderRelay positionRelay_;
  juce::WebSliderRelay positionRandRelay_;
  juce::WebSliderRelay spreadRelay_;
  juce::WebToggleButtonRelay freezeRelay_;
  juce::WebToggleButtonRelay bypassRelay_;

  juce::WebBrowserComponent webView_;

  juce::WebSliderParameterAttachment gainAttachment_;
  juce::WebSliderParameterAttachment dryWetAttachment_;
  juce::WebSliderParameterAttachment grainSizeAttachment_;
  juce::WebSliderParameterAttachment densityAttachment_;
  juce::WebSliderParameterAttachment pitchAttachment_;
  juce::WebSliderParameterAttachment pitchRandAttachment_;
  juce::WebSliderParameterAttachment positionAttachment_;
  juce::WebSliderParameterAttachment positionRandAttachment_;
  juce::WebSliderParameterAttachment spreadAttachment_;
  juce::WebToggleButtonParameterAttachment freezeAttachment_;
  juce::WebToggleButtonParameterAttachment bypassAttachment_;

  std::unique_ptr<juce::FileChooser> fileChooser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PicotadoAudioProcessorEditor)
};

}  // namespace picotado
