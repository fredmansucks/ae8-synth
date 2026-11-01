// Offline test: an off-key synthetic vocal through AE-8 TUNE; measures the output pitch.
#include "../TuneEngine.h"
#include <cstdio>
#include <string>
using namespace ae8tune;

struct Reson { float b0, a1, a2, y1 = 0, y2 = 0; void set (float fs, float f, float bw) { float r = std::exp (-M_PI * bw / fs); a1 = 2 * r * std::cos (2 * M_PI * f / fs); a2 = -r * r; b0 = 1 - r; }
    float run (float x) { float y = b0 * x + a1 * y1 + a2 * y2; y2 = y1; y1 = y; return y; } };

static void wav (const std::string& p, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (p.c_str(), "wb"); uint32_t n = (uint32_t) L.size(), db = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + db); std::fwrite ("WAVEfmt ", 1, 8, f); u32 (16); u16 (1); u16 (2); u32 (sr); u32 (sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (db);
    for (uint32_t i = 0; i < n; ++i) { int16_t a = (int16_t) std::lround (std::clamp (L[i], -1.f, 1.f) * 32767), b = (int16_t) std::lround (std::clamp (R[i], -1.f, 1.f) * 32767); std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f); }
    std::fclose (f);
}

// plain full-rate YIN for measuring
static float measure (const std::vector<float>& x, size_t start, int sr)
{
    const int W = 1024, tmax = sr / 70, tmin = sr / 1000;
    std::vector<double> d (tmax + 1); d[0] = 1; double run = 0;
    for (int tau = 1; tau <= tmax; ++tau) { double s = 0; for (int j = 0; j < W; ++j) { double df = x[start + j] - x[start + j + tau]; s += df * df; } run += s; d[tau] = run > 0 ? s * tau / run : 1; }
    int best = -1; for (int tau = tmin; tau < tmax; ++tau) if (d[tau] < 0.12) { while (tau + 1 < tmax && d[tau + 1] < d[tau]) ++tau; best = tau; break; }
    if (best < 0) return -1;
    double a = d[best - 1], b = d[best], c = d[best + 1], tf = best + 0.5 * (a - c) / (a - 2 * b + c);
    return 69 + 12 * std::log2 (sr / tf / 440.0);
}

int main (int argc, char** argv)
{
    const int sr = 48000; const double dur = 6.0; const int N = (int) (dur * sr);
    // pitch contour: hovers between notes, glides, with vibrato (A minor context)
    std::vector<float> src (N), f0c (N);
    Reson f1, f2, f3; f1.set (sr, 700, 90); f2.set (sr, 1150, 110); f3.set (sr, 2600, 160);
    double ph = 0; uint32_t rng = 1;
    for (int i = 0; i < N; ++i)
    {
        double tt = (double) i / sr, m;
        if (tt < 1.5) m = 57.35;                          // A3 +35 cents (sharp)
        else if (tt < 2.0) m = 57.35 + (tt - 1.5) * 2 * (60.3 - 57.35);  // glide
        else if (tt < 3.5) m = 60.3 + 0.35 * std::sin (2 * M_PI * 5.5 * tt); // C4 +30c, wide vibrato
        else if (tt < 5.0) m = 62.55;                     // between D4 and D#4 -> D4 (minor scale)
        else m = 64.0 - 0.4;                              // E4 -40c
        f0c[i] = (float) m;
        const double f = 440 * std::pow (2, (m - 69) / 12);
        ph += f / sr; if (ph >= 1) ph -= 1;
        float glott = (float) (ph < 0.6 ? std::sin (M_PI * ph / 0.6) : 0) ; glott = glott * glott;  // Rosenberg-ish
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; float noise = ((rng >> 8) / 16777216.0f - 0.5f) * 0.02f;
        float s = glott - 0.4f + noise;
        float v = f1.run (s) * 1.0f + f2.run (s) * 0.6f + f3.run (s) * 0.3f;
        float env = (float) std::min (1.0, std::min (tt / 0.05, (dur - tt) / 0.05));
        src[i] = v * 0.25f * env;
    }
    auto run = [&] (const char* name, float speed, float human, float dbl, float harm, std::vector<float>& L, std::vector<float>& R)
    {
        TuneEngine e; e.prepare (sr);
        e.setParam (KEY, 9); e.setParam (SCALE, S_MINOR); e.setParam (SPEED, speed); e.setParam (HUMANIZE, human);
        e.setParam (DOUBLE, dbl); e.setParam (HARMONY, harm);
        L.assign (N, 0); R.assign (N, 0);
        for (int p = 0; p < N; p += 256) { int n = std::min (256, N - p); e.process (src.data() + p, nullptr, L.data() + p, R.data() + p, n); }
        const int lat = e.latencySamples();
        // measure deviation from the nearest scale note in voiced frames (skip transitions)
        double sumAbs = 0; int cnt = 0, bad = 0; float worst = 0;
        for (int fr = 0; fr * 2400 + 3000 + lat < N - 4000; ++fr)
        {
            size_t s0 = (size_t) fr * 2400 + 3000 + lat;
            const double tin = (double) (s0 - lat) / sr;
            if (std::fabs (tin - 1.5) < 0.6 || std::fabs (tin - 2.0) < 0.15 || std::fabs (tin - 3.5) < 0.15 || std::fabs (tin - 5.0) < 0.15) continue;
            float m = measure (L, s0, sr); if (m < 0) continue;
            float tgt = (float) nearestNote (m, 9, scaleMask (S_MINOR));
            float dev = std::fabs (m - tgt) * 100; sumAbs += dev; ++cnt; worst = std::max (worst, dev); if (dev > 15) ++bad;
        }
        double peak = 0; for (int i = 0; i < N; ++i) peak = std::max (peak, (double) std::fabs (L[i]));
        std::printf ("%-28s latency %d smp (%.1f ms)  mean dev %.1f ct  worst %.1f ct  frames>15ct %d/%d  peak %.2f\n",
                     name, lat, lat * 1000.0 / sr, cnt ? sumAbs / cnt : -1, worst, bad, cnt, peak);
    };
    std::vector<float> L, R;
    // input deviation for reference
    { double s = 0; int c = 0; for (int fr = 0; fr * 2400 + 3000 < N - 4000; ++fr) { float m = measure (src, fr * 2400 + 3000, sr); if (m < 0) continue; s += std::fabs (m - nearestNote (m, 9, scaleMask (S_MINOR))) * 100; ++c; }
      std::printf ("%-28s mean dev %.1f ct over %d frames\n", "INPUT (uncorrected)", s / c, c); }
    const std::string dir = argc > 1 ? argv[1] : ".";
    std::vector<float> dryR (src); wav (dir + "/tune_input.wav", src, dryR, sr);
    run ("Robot (speed 0)", 0, 0, 0, 0, L, R);              wav (dir + "/tune_robot.wav", L, R, sr);
    run ("Classic (20 ms)", 20, 0.2f, 0, 0, L, R);          wav (dir + "/tune_classic.wav", L, R, sr);
    run ("Natural (80 ms, humanize)", 80, 0.7f, 0, 0, L, R); wav (dir + "/tune_natural.wav", L, R, sr);
    run ("Double + Harmony", 20, 0.2f, 0.7f, 0.7f, L, R);   wav (dir + "/tune_harmony.wav", L, R, sr);
    return 0;
}
