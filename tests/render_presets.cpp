// Offline test: renders every factory preset to a WAV and reports levels.
#include "../Source/SynthEngine.h"
#include "../Source/Presets.h"
#include <cstdio>
#include <string>
#include <cmath>

using namespace ae8;

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lround (std::clamp (L[i], -1.0f, 1.0f) * 32767.0f);
        int16_t b = (int16_t) std::lround (std::clamp (R[i], -1.0f, 1.0f) * 32767.0f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

struct Ev { double t; int note; bool on; };

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : ".";
    const int sr = 48000, block = 256;
    const auto& presets = factoryPresets();
    int idx = 0, bad = 0;
    std::printf ("%-3s %-30s %8s %8s\n", "#", "preset", "peak", "rms");
    for (const auto& pr : presets)
    {
        Engine e;
        for (int i = 0; i < NUM_PARAMS; ++i) e.setParam (i, paramDefs()[(size_t) i].def);
        for (auto& kv : pr.v) e.setParam (kv.first, kv.second);
        e.prepare (sr);

        const bool mono = e.getParam (MODE) > 0.5f;
        const bool bass = std::string (pr.name).rfind ("BS", 0) == 0;
        std::vector<Ev> ev;
        if (mono)
        {
            const int base = bass ? 36 : 60;
            const int seq[] = { 0, 3, 7, 12, 10, 7, 3, 0 };
            for (int k = 0; k < 8; ++k) { ev.push_back ({ k * 0.3, base + seq[k], true }); ev.push_back ({ k * 0.3 + 0.27, base + seq[k], false }); }
        }
        else
        {
            const int base = bass ? 36 : 48;
            const int ch[] = { 0, 7, 12, 16, 19 };
            for (int k = 0; k < 5; ++k) { ev.push_back ({ 0.0 + k * 0.01, base + ch[k], true }); ev.push_back ({ 2.2, base + ch[k], false }); }
        }
        const double total = 5.0;
        const int N = (int) (total * sr);
        std::vector<float> L ((size_t) N), R ((size_t) N);
        std::sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
        size_t ei = 0;
        for (int pos = 0; pos < N; pos += block)
        {
            const int len = std::min (block, N - pos);
            while (ei < ev.size() && ev[ei].t * sr < pos + len)
            {
                if (ev[ei].on) e.noteOn (ev[ei].note, 0.85f); else e.noteOff (ev[ei].note);
                ++ei;
            }
            e.render (L.data() + pos, R.data() + pos, len);
        }
        double peak = 0, sum = 0; bool nan = false;
        for (int i = 0; i < N; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) nan = true;
            peak = std::max (peak, (double) std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i])));
            sum += L[(size_t) i] * L[(size_t) i];
        }
        const double rms = std::sqrt (sum / N);
        const bool flag = nan || peak > 1.0 || peak < 0.02;
        bad += flag;
        std::printf ("%-3d %-30s %8.3f %8.4f %s\n", idx + 1, pr.name, peak, rms, nan ? "NaN!" : (flag ? "<-- check" : ""));
        char fn[256]; std::snprintf (fn, sizeof fn, "%s/%02d.wav", outDir.c_str(), idx + 1);
        writeWav (fn, L, R, sr);
        ++idx;
    }
    std::printf ("presets: %d, flagged: %d\n", idx, bad);
    return bad ? 1 : 0;
}
