#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace picotado::id {
const juce::ParameterID GAIN{"GAIN", 1};
const juce::ParameterID DRY_WET{"DRY_WET", 1};
const juce::ParameterID GRAIN_SIZE{"GRAIN_SIZE", 1};
const juce::ParameterID DENSITY{"DENSITY", 1};
const juce::ParameterID PITCH{"PITCH", 1};
const juce::ParameterID PITCH_RAND{"PITCH_RAND", 1};
const juce::ParameterID POSITION{"POSITION", 1};
const juce::ParameterID POSITION_RAND{"POSITION_RAND", 1};
const juce::ParameterID SPREAD{"SPREAD", 1};
const juce::ParameterID FREEZE{"FREEZE", 1};
const juce::ParameterID BYPASS{"BYPASS", 1};
}  // namespace picotado::id
