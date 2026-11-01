#pragma once
// synthetic soul-ish loop: 92 BPM, 4 bars, F minor chords (Fm7 / Db / Eb / Cm) + kick/snare/hat
#include <vector>
#include <cmath>
#include <cstdint>
inline std::vector<float> makeLoop (double sr, double bpm = 92.0, int bars = 4)
{
    const double beat = 60.0 / bpm; const int64_t N = (int64_t) (bars * 4 * beat * sr);
    std::vector<float> x ((size_t) N, 0.0f);
    const int chords[4][4] = { { 41, 53, 56, 60 }, { 41, 53, 56, 60 }, { 46, 53, 58, 61 }, { 48, 52, 55, 58 } };
    uint32_t s = 1;
    auto rnd = [&] { s = s * 1664525u + 1013904223u; return (float) (s >> 9) / 4194304.0f - 1.0f; };
    for (int64_t i = 0; i < N; ++i)
    {
        const double t = i / sr; const int bar = (int) (t / (4 * beat)) % 4; const double tb = std::fmod (t, 4 * beat);
        double v = 0;
        for (int k = 0; k < 4; ++k) { const double f = 440 * std::pow (2.0, (chords[bar][k] - 69) / 12.0); v += 0.07 * (std::sin (2 * M_PI * f * t) + 0.3 * std::sin (4 * M_PI * f * t)) * std::min (1.0, tb * 20); }
        const double b = std::fmod (t, beat); const int bi = (int) (t / beat) % 4;
        if (bi == 0 || bi == 2) v += 0.8 * std::sin (2 * M_PI * (50 + 120 * std::exp (-b * 30)) * b) * std::exp (-b * 9);
        if (bi == 1 || bi == 3) v += 0.45 * rnd() * std::exp (-b * 14) + 0.3 * std::sin (2 * M_PI * 190 * b) * std::exp (-b * 20);
        const double h = std::fmod (t, beat / 2); v += 0.12 * rnd() * std::exp (-h * 60);
        x[(size_t) i] = (float) v;
    }
    return x;
}
