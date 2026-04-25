#pragma once

#include <array>

namespace picotado {

// Maximum HRIR length supported per grain. SOFA files typically have
// 128–512 taps; we cap at 256 to keep per-grain memory and CPU bounded.
inline constexpr int MAX_HRIR_LENGTH = 256;

struct Grain {
  bool active = false;

  // Source readout from the live capture buffer (varispeed, fractional)
  double readPosition = 0.0;
  double pitchRatio = 1.0;

  // Lifetime / Tukey envelope
  int durationSamples = 0;
  int sampleIndex = 0;
  int attackSamples = 0;
  int releaseSamples = 0;

  // Per-grain HRIR coefficients (filled once at spawn)
  std::array<float, MAX_HRIR_LENGTH> hrirL{};
  std::array<float, MAX_HRIR_LENGTH> hrirR{};
  int hrirLength = 0;

  // Circular history buffer for streaming FIR convolution
  std::array<float, MAX_HRIR_LENGTH> history{};
  int historyIndex = 0;

  // Fallback ITD+ILD path (used when no SOFA is loaded)
  bool useHrtf = true;
  int itdSamplesL = 0;
  int itdSamplesR = 0;
  float ildL = 1.0f;
  float ildR = 1.0f;
};

}  // namespace picotado
