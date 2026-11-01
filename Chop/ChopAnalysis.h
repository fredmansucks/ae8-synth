#pragma once
// AE-8 CHOP — sample analysis (framework-free C++17)
// onset detection (spectral flux), tempo estimate, key estimate, slice-point generation, Find / Random.

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

namespace ae8chop
{
constexpr double kPiD = 3.14159265358979323846;

inline void fft (std::vector<std::complex<float>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2 * kPiD / (double) len;
        const std::complex<float> wl ((float) std::cos (ang), (float) std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<float> w (1, 0);
            for (size_t j = 0; j < len / 2; ++j)
            {
                const auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v; a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

struct Onset { int64_t pos; float strength; };

struct Analysis
{
    std::vector<Onset> onsets;     // sorted by position
    std::vector<float> flux;       // onset-strength envelope, one value per hop
    int hop = 512;
    double bpm = 120.0;
    int keyRoot = 0;               // 0 = C
    bool keyMinor = false;
};

inline const char* keyName (int root, bool minor)
{
    static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static char buf[16];
    snprintf (buf, sizeof (buf), "%s %s", n[((root % 12) + 12) % 12], minor ? "MIN" : "MAJ");
    return buf;
}

// ---------------------------------------------------------------------------------------------
inline Analysis analyse (const std::vector<float>& mono, double sr)
{
    Analysis A;
    const int N = 1024, hop = 512;
    A.hop = hop;
    const int64_t len = (int64_t) mono.size();
    const int frames = (int) std::max<int64_t> (0, (len - N) / hop + 1);
    std::vector<float> win ((size_t) N);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * (float) std::cos (2 * kPiD * i / (N - 1));

    std::vector<float> prev ((size_t) N / 2, 0.0f);
    std::vector<std::complex<float>> buf ((size_t) N);
    A.flux.assign ((size_t) frames, 0.0f);
    std::vector<float> rms ((size_t) frames, 0.0f);
    for (int f = 0; f < frames; ++f)
    {
        double e = 0;
        for (int i = 0; i < N; ++i) { const float x = mono[(size_t) (f * hop + i)]; buf[(size_t) i] = { x * win[(size_t) i], 0 }; e += x * x; }
        rms[(size_t) f] = (float) std::sqrt (e / N);
        fft (buf);
        float fl = 0;
        for (int k = 1; k < N / 2; ++k)
        {
            const float m = std::log1p (100.0f * std::abs (buf[(size_t) k]));
            fl += std::max (0.0f, m - prev[(size_t) k]);
            prev[(size_t) k] = m;
        }
        A.flux[(size_t) f] = fl;
    }

    // peak picking with an adaptive threshold
    if (frames > 2)
    {
        std::vector<float> sorted = A.flux;
        std::nth_element (sorted.begin(), sorted.begin() + (long) sorted.size() / 2, sorted.end());
        const float globalMed = sorted[sorted.size() / 2];
        const float mx = *std::max_element (A.flux.begin(), A.flux.end());
        const int minGap = std::max (1, (int) (0.045 * sr / hop));
        int last = -minGap;
        for (int f = 1; f < frames - 1; ++f)
        {
            const float v = A.flux[(size_t) f];
            if (v < A.flux[(size_t) f - 1] || v < A.flux[(size_t) f + 1]) continue;
            float local = 0; int cnt = 0;
            for (int k = std::max (0, f - 10); k <= std::min (frames - 1, f + 10); ++k) { local += A.flux[(size_t) k]; ++cnt; }
            local /= (float) cnt;
            if (v < local * 1.25f + globalMed * 0.25f || v < mx * 0.04f) continue;
            if (f - last < minGap) continue;
            // refine: walk back to where the level starts to rise
            int64_t p = (int64_t) f * hop + N / 2;
            const int64_t lo = std::max<int64_t> (0, p - hop * 2);
            float peak = 0; int64_t peakPos = p;
            for (int64_t i = lo; i < std::min (len, p + hop); ++i) if (std::fabs (mono[(size_t) i]) > peak) { peak = std::fabs (mono[(size_t) i]); peakPos = i; }
            int64_t s = peakPos;
            while (s > lo && std::fabs (mono[(size_t) s]) > peak * 0.12f) --s;
            s = std::max<int64_t> (0, s - (int64_t) (0.002 * sr));
            A.onsets.push_back ({ s, v / (mx + 1e-9f) });
            last = f;
        }
    }

    // the file usually starts on a hit: give it an onset at 0 when the start is not silent
    if (frames > 0 && rms[0] > 0.01f && (A.onsets.empty() || A.onsets.front().pos > (int64_t) (0.03 * sr)))
        A.onsets.insert (A.onsets.begin(), { 0, 1.0f });

    // ---- tempo: autocorrelation of the flux, plus a preference for whole bars when the file is a loop
    {
        const double fps = sr / hop;
        auto score = [&] (double bpm)
        {
            const double lag = 60.0 / bpm * fps;
            double s = 0;
            for (int m = 1; m <= 4; ++m)
            {
                const double L = lag * m;
                const int li = (int) L; const double fr = L - li;
                double acc = 0;
                for (int f = 0; f + li + 1 < frames; ++f) acc += A.flux[(size_t) f] * ((1 - fr) * A.flux[(size_t) (f + li)] + fr * A.flux[(size_t) (f + li + 1)]);
                s += acc / (m == 1 ? 1.0 : 2.0 * m);
            }
            const double pref = std::exp (-0.5 * std::pow (std::log2 (bpm / 105.0) / 0.6, 2));
            return s * (0.6 + 0.4 * pref);
        };
        double best = 120, bestS = -1;
        for (double b = 70; b <= 180; b += 0.5) { const double s = score (b); if (s > bestS) { bestS = s; best = b; } }
        // whole-bar candidates
        const double secs = (double) len / sr;
        double loopBest = 0, loopS = -1;
        for (int beats : { 4, 8, 12, 16, 24, 32, 48, 64 })
        {
            const double b = beats * 60.0 / secs;
            if (b < 70 || b > 180) continue;
            const double s = score (b);
            if (s > loopS) { loopS = s; loopBest = b; }
        }
        A.bpm = (loopBest > 0 && loopS > bestS * 0.8) ? loopBest : best;
        A.bpm = std::round (A.bpm * 10.0) / 10.0;
    }

    // ---- key: chroma + Krumhansl-Schmuckler profiles
    {
        const int K = 8192;
        std::vector<std::complex<float>> b2 ((size_t) K);
        std::vector<float> w2 ((size_t) K);
        for (int i = 0; i < K; ++i) w2[(size_t) i] = 0.5f - 0.5f * (float) std::cos (2 * kPiD * i / (K - 1));
        double chroma[12] {};
        const int64_t step = K / 2;
        for (int64_t st = 0; st + K <= len; st += step)
        {
            for (int i = 0; i < K; ++i) b2[(size_t) i] = { mono[(size_t) (st + i)] * w2[(size_t) i], 0 };
            fft (b2);
            // keep only spectral peaks that stand above their neighbourhood (tones, not drum noise)
            const int k0 = (int) (90.0 * K / sr), k1 = std::min (K / 2 - 4, (int) (2200.0 * K / sr));
            for (int k = k0; k <= k1; ++k)
            {
                const float m = std::abs (b2[(size_t) k]);
                if (m < std::abs (b2[(size_t) k - 1]) || m < std::abs (b2[(size_t) k + 1])) continue;
                float nb = 0; for (int d = 4; d <= 12; ++d) nb += std::abs (b2[(size_t) std::max (1, k - d)]) + std::abs (b2[(size_t) std::min (K / 2 - 1, k + d)]);
                nb /= 18.0f;
                if (m < nb * 4.0f) continue;
                const double f = k * sr / K;
                const double midi = 69 + 12 * std::log2 (f / 440.0);
                const int pc = (((int) std::lround (midi)) % 12 + 12) % 12;
                chroma[pc] += std::log1p (m / (nb + 1e-6f));
            }
        }
        static const double maj[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
        static const double mnr[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
        auto corr = [&] (const double* prof, int root)
        {
            double mx = 0, my = 0;
            for (int i = 0; i < 12; ++i) { mx += chroma[(i + root) % 12]; my += prof[i]; }
            mx /= 12; my /= 12;
            double sxy = 0, sxx = 0, syy = 0;
            for (int i = 0; i < 12; ++i) { const double x = chroma[(i + root) % 12] - mx, y = prof[i] - my; sxy += x * y; sxx += x * x; syy += y * y; }
            return sxy / std::sqrt (sxx * syy + 1e-12);
        };
        double best = -2;
        for (int r = 0; r < 12; ++r)
        {
            const double a = corr (maj, r), b = corr (mnr, r);
            if (a > best) { best = a; A.keyRoot = r; A.keyMinor = false; }
            if (b > best) { best = b; A.keyRoot = r; A.keyMinor = true; }
        }
    }
    return A;
}

// ---------------------------------------------------------------------------------------------
enum ChopMode { CHOP_TRANSIENT, CHOP_BEAT, CHOP_EQUAL, CHOP_MANUAL };

inline std::vector<int64_t> transientSlices (const Analysis& A, float sens, int maxSlices)
{
    // sens 0..1 → minimum strength
    const float thr = 0.9f * std::pow (1.0f - sens, 1.5f);
    std::vector<Onset> pick;
    for (auto& o : A.onsets) if (o.strength >= thr) pick.push_back (o);
    if ((int) pick.size() > maxSlices)
    {
        std::sort (pick.begin(), pick.end(), [] (auto& a, auto& b) { return a.strength > b.strength; });
        pick.resize ((size_t) maxSlices);
    }
    std::vector<int64_t> r;
    for (auto& o : pick) r.push_back (o.pos);
    std::sort (r.begin(), r.end());
    if (r.empty() || r.front() > 0) { if (! r.empty() && r.front() < 2048) r.front() = 0; else r.insert (r.begin(), 0); }
    if ((int) r.size() > maxSlices) r.resize ((size_t) maxSlices);
    return r;
}

inline std::vector<int64_t> beatSlices (int64_t len, double sr, double bpm, double beatsPerSlice, int maxSlices)
{
    std::vector<int64_t> r;
    const double step = 60.0 / bpm * beatsPerSlice * sr;
    for (double p = 0; p < (double) len - 64 && (int) r.size() < maxSlices; p += step) r.push_back ((int64_t) std::llround (p));
    return r;
}

inline std::vector<int64_t> equalSlices (int64_t len, int n)
{
    std::vector<int64_t> r;
    for (int i = 0; i < n; ++i) r.push_back (len * i / n);
    return r;
}

// "Find": strongest onsets spread across the file (one per region), like a DJ dropping cue points
inline std::vector<int64_t> findSlices (const Analysis& A, int64_t len, int n)
{
    std::vector<Onset> c = A.onsets;
    std::sort (c.begin(), c.end(), [] (auto& a, auto& b) { return a.strength > b.strength; });
    std::vector<int64_t> r;
    const int64_t gap = len / (n * 2);
    for (auto& o : c)
    {
        bool ok = true;
        for (auto p : r) if (std::llabs (p - o.pos) < gap) { ok = false; break; }
        if (ok) r.push_back (o.pos);
        if ((int) r.size() >= n) break;
    }
    if (r.empty()) return equalSlices (len, n);
    std::sort (r.begin(), r.end());
    return r;
}

inline std::vector<int64_t> randomSlices (const Analysis& A, int64_t len, int n, uint32_t seed)
{
    std::mt19937 rng (seed);
    std::vector<int64_t> pool;
    for (auto& o : A.onsets) pool.push_back (o.pos);
    if ((int) pool.size() < n) for (int i = 0; i < n * 2; ++i) pool.push_back ((int64_t) (rng() % (uint32_t) std::max<int64_t> (1, len - 1)));
    std::shuffle (pool.begin(), pool.end(), rng);
    std::vector<int64_t> r;
    const int64_t gap = len / (n * 4);
    for (auto p : pool)
    {
        bool ok = true;
        for (auto q : r) if (std::llabs (p - q) < gap) { ok = false; break; }
        if (ok) r.push_back (p);
        if ((int) r.size() >= n) break;
    }
    std::sort (r.begin(), r.end());
    return r;
}

// semitone shift that takes the detected key to the target key (minor/major mapped through the relative key)
inline int keyShift (int srcRoot, bool srcMinor, int dstRoot, bool dstMinor)
{
    int d = dstRoot;
    if (srcMinor && ! dstMinor) d = dstRoot + 9;   // relative minor of target major
    if (! srcMinor && dstMinor) d = dstRoot + 3;   // relative major of target minor
    int s = ((d - srcRoot) % 12 + 12) % 12;
    if (s > 6) s -= 12;
    return s;
}
} // namespace ae8chop
