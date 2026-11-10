#pragma once
// AE-8 MEMORY v0.1 DSP — framework-independent C++17.
// Original lo-fi digital sampling character; NOT an exact emulation of any vintage sampler.
#include <algorithm>
#include <array>
#include <cmath>

namespace ae8memory {
class Engine {
public:
  void prepare(double sampleRate) {
    sr = std::max(8000.0, sampleRate);
    phases = { 0.0, 0.0 }; held = { 0.0f, 0.0f }; low = { 0.0f, 0.0f };
  }
  void set(float rateHz, int bitDepth, float bandwidthHz, float drive, float wet) {
    rate = std::clamp((double)rateHz, 3000.0, std::min(48000.0, sr));
    bits = std::clamp(bitDepth, 4, 16);
    bandwidth = std::clamp((double)bandwidthHz, 400.0, sr * 0.45);
    saturation = std::clamp(drive, 0.0f, 1.0f);
    mix = std::clamp(wet, 0.0f, 1.0f);
  }
  void process(float* const* channels, int numChannels, int frames) {
    const float levels = (float)((1 << (bits - 1)) - 1);
    const float gain = 1.0f + saturation * 9.0f;
    const float norm = 1.0f / std::tanh(gain);
    const float a = (float)(1.0 - std::exp(-6.283185307179586 * bandwidth / sr));
    const double increment = rate / sr;
    for (int c = 0; c < std::min(numChannels, 2); ++c) {
      for (int i = 0; i < frames; ++i) {
        const float dry = channels[c][i];
        if (phases[c] <= 0.0) {
          // Sample-and-hold followed by signed quantization, creating real aliasing.
          const float clipped = std::clamp(dry, -1.0f, 1.0f);
          held[c] = std::round(clipped * levels) / levels;
          phases[c] += 1.0;
        }
        phases[c] -= increment;
        const float distorted = std::tanh(held[c] * gain) * norm;
        low[c] += a * (distorted - low[c]);
        channels[c][i] = dry * (1.0f - mix) + low[c] * mix;
      }
    }
  }
private:
  double sr = 44100.0, rate = 11025.0, bandwidth = 5500.0;
  int bits = 12;
  float saturation = 0.2f, mix = 1.0f;
  std::array<double, 2> phases { 0, 0 };
  std::array<float, 2> held { 0, 0 }, low { 0, 0 };
};
} // namespace ae8memory
