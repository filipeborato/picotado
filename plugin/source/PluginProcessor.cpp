#include "Picotado/PluginProcessor.h"

#include <cmath>

#include "Picotado/ParameterIDs.h"
#include "Picotado/PluginEditor.h"

namespace picotado {

PicotadoAudioProcessor::PicotadoAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(),
                                    true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(),
                                     true)),
      state_(*this, nullptr, "PARAMETERS", createLayout(*this)) {
  auto getFloat = [&](const juce::ParameterID& pid) {
    return dynamic_cast<juce::AudioParameterFloat*>(
        state_.getParameter(pid.getParamID()));
  };
  auto getBool = [&](const juce::ParameterID& pid) {
    return dynamic_cast<juce::AudioParameterBool*>(
        state_.getParameter(pid.getParamID()));
  };
  gain_ = getFloat(id::GAIN);
  dryWet_ = getFloat(id::DRY_WET);
  grainSize_ = getFloat(id::GRAIN_SIZE);
  density_ = getFloat(id::DENSITY);
  pitch_ = getFloat(id::PITCH);
  pitchRand_ = getFloat(id::PITCH_RAND);
  position_ = getFloat(id::POSITION);
  positionRand_ = getFloat(id::POSITION_RAND);
  spread_ = getFloat(id::SPREAD);
  freeze_ = getBool(id::FREEZE);
  bypass_ = getBool(id::BYPASS);

  engine_.setBinauralRenderer(&binaural_);
}

void PicotadoAudioProcessor::prepareToPlay(double newSampleRate,
                                           int newSamplesPerBlock) {
  binaural_.prepare(newSampleRate);
  engine_.prepare(newSampleRate, newSamplesPerBlock, getTotalNumInputChannels());
}

void PicotadoAudioProcessor::releaseResources() {}

bool PicotadoAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }
  const auto in = layouts.getMainInputChannelSet();
  if (in != juce::AudioChannelSet::stereo() &&
      in != juce::AudioChannelSet::mono()) {
    return false;
  }
  return true;
}

void PicotadoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;

  if (bypass_->get() || buffer.getNumSamples() == 0) {
    return;
  }

  // Push current parameter snapshot to engine (cheap atomics)
  engine_.setGrainSizeMs(grainSize_->get());
  engine_.setDensityHz(density_->get());
  engine_.setPitchRatio(std::pow(2.0f, pitch_->get() / 12.0f));
  engine_.setPitchRandomSemis(pitchRand_->get());
  engine_.setPositionOffsetMs(position_->get());
  engine_.setPositionRandomMs(positionRand_->get());
  engine_.setSpreadRad(juce::degreesToRadians(spread_->get()));
  engine_.setFreeze(freeze_->get());
  engine_.setDryWet(dryWet_->get());

  engine_.process(buffer);

  buffer.applyGain(gain_->get());

  activeGrains.store(engine_.getActiveGrainCount());
}

juce::AudioProcessorEditor* PicotadoAudioProcessor::createEditor() {
  return new PicotadoAudioProcessorEditor(*this);
}

void PicotadoAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto state = state_.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void PicotadoAudioProcessor::setStateInformation(const void* data,
                                                 int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml != nullptr && xml->hasTagName(state_.state.getType())) {
    state_.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
PicotadoAudioProcessor::createLayout(PicotadoAudioProcessor&) {
  using namespace juce;
  AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<AudioParameterFloat>(
      id::GAIN, "Gain", NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f),
      1.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::DRY_WET, "Dry/Wet", NormalisableRange<float>(0.0f, 1.0f, 0.001f),
      0.7f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::GRAIN_SIZE, "Grain Size",
      NormalisableRange<float>(5.0f, 500.0f, 0.1f, 0.5f), 80.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::DENSITY, "Density",
      NormalisableRange<float>(1.0f, 200.0f, 0.1f, 0.3f), 20.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::PITCH, "Pitch",
      NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::PITCH_RAND, "Pitch Random",
      NormalisableRange<float>(0.0f, 12.0f, 0.01f), 0.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::POSITION, "Position",
      NormalisableRange<float>(0.0f, 2000.0f, 0.1f, 0.4f), 50.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::POSITION_RAND, "Position Random",
      NormalisableRange<float>(0.0f, 1000.0f, 0.1f, 0.4f), 50.0f));
  layout.add(std::make_unique<AudioParameterFloat>(
      id::SPREAD, "Spread",
      NormalisableRange<float>(0.0f, 180.0f, 0.1f), 90.0f));
  layout.add(std::make_unique<AudioParameterBool>(id::FREEZE, "Freeze", false));
  layout.add(std::make_unique<AudioParameterBool>(id::BYPASS, "Bypass", false));

  return layout;
}

}  // namespace picotado

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new picotado::PicotadoAudioProcessor();
}
