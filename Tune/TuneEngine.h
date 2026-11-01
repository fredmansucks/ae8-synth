#pragma once
// AE-8 TUNE — real-time pitch correction engine (framework-free C++17)
//
//  detection : YIN on a decimated (~11 kHz) copy of the input, every ~4 ms
//  correction: nearest note of key/scale with hysteresis, retune-speed glide, humanize (keeps vibrato)
//  shifting  : streaming TD-PSOLA (pitch-synchronous grains) -> formants stay natural
//  voices    : lead + 2 doubles (L/R) + 2 harmony voices, all from the same analysis
//  effects   : distortion -> ping-pong delay (tempo synced) -> reverb

#include "TuneParams.h"
#include "../Source/SynthEngine.h"   // ae8::Reverb, ae8::PingPongDelay, fastTanh
#include <array>
#include <atomic>
#include <cmath>
#include <deque>
#include <vector>
#include <algorithm>

namespace ae8tune
{
constexpr double kPiD = 3.14159265358979323846;

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void lowpass (double fs, double fc, double q = 0.7071)
    {
        const double w = 2.0 * kPiD * fc / fs, c = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        b0 = (float) ((1 - c) / 2 / a0); b1 = (float) ((1 - c) / a0); b2 = b0;
        a1 = (float) (-2 * c / a0); a2 = (float) ((1 - al) / a0);
    }
    float run (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};

inline int scaleMask (int scale)
{
    auto m = [] (std::initializer_list<int> iv) { int r = 0; for (int i : iv) r |= 1 << i; return r; };
    switch (scale)
    {
        case S_MAJOR:      return m ({ 0, 2, 4, 5, 7, 9, 11 });
        case S_MINOR:      return m ({ 0, 2, 3, 5, 7, 8, 10 });
        case S_HARM_MINOR: return m ({ 0, 2, 3, 5, 7, 8, 11 });
        case S_PENT_MAJOR: return m ({ 0, 2, 4, 7, 9 });
        case S_PENT_MINOR: return m ({ 0, 3, 5, 7, 10 });
        case S_BLUES:      return m ({ 0, 3, 5, 6, 7, 10 });
        default:           return 0xFFF;
    }
}
inline bool inScale (int note, int key, int mask) { return (mask >> (((note - key) % 12 + 12) % 12)) & 1; }

inline int nearestNote (float m, int key, int mask)
{
    int best = (int) std::lround (m); float bd = 1e9f;
    for (int n = (int) std::floor (m) - 3; n <= (int) std::ceil (m) + 3; ++n)
        if (inScale (n, key, mask) && std::fabs ((float) n - m) < bd) { bd = std::fabs ((float) n - m); best = n; }
    return best;
}
inline int scaleStep (int note, int steps, int key, int mask)   // move N scale degrees
{
    int n = note;
    const int dir = steps > 0 ? 1 : -1;
    for (int s = 0; s != steps; s += dir) { do n += dir; while (! inScale (n, key, mask)); }
    return n;
}

class TuneEngine
{
public:
    static constexpr int kVoices = 5;   // lead, doubleL, doubleR, harmA, harmB
    static constexpr float kMinHz = 80.0f, kMaxHz = 1000.0f;

    // read by the GUI
    std::atomic<float> guiInMidi { 0 }, guiOutMidi { 0 }, guiLevel { 0 };
    std::atomic<int> guiTarget { -1 };
    std::atomic<bool> guiVoiced { false };

    TuneEngine() { for (int i = 0; i < NUM_PARAMS; ++i) prm[(size_t) i] = paramDefs()[(size_t) i].def; }

    void setParam (int i, float v) { prm[(size_t) i] = v; }
    float getParam (int i) const { return prm[(size_t) i]; }
    int latencySamples() const { return L; }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        Tmax = (int) std::ceil (sr / kMinHz);
        L = (int) std::ceil (2.6f * (float) Tmax) + 64;
        size = 1; while (size < 8 * (L + Tmax)) size <<= 1;
        mask = size - 1;
        in.assign ((size_t) size, 0.0f); lp.assign ((size_t) size, 0.0f);
        for (auto& o : ola) o.assign ((size_t) size, 0.0f);
        t = (int64_t) L + 2 * Tmax;      // start with history of silence
        marks.clear(); marks.push_back (t - Tmax);
        for (auto& s : synth) s = (double) (t - L + Tmax);

        D = std::max (1, (int) std::lround (sr / 11025.0f));
        dsr = sr / (float) D;
        aa1.lowpass (sr, 0.38 * dsr); aa2 = aa1;
        det.assign (4096, 0.0f); dn = 0; dPhase = 0; hopCount = 0;
        W = (int) std::ceil (dsr * 0.022f);
        tauMax = (int) std::ceil (dsr / kMinHz);
        tauMin = std::max (2, (int) std::floor (dsr / kMaxHz));
        yin.assign ((size_t) tauMax + 2, 0.0f);

        T0 = sr / 200.0f; voiced = false; voicedAmt = 0; target = -1; corr = 0; mSlow = 60; mHist[0] = mHist[1] = mHist[2] = 60;
        lpC = 1.0f - std::exp (-2.0f * (float) kPiD * 900.0f / sr);
        lpS = 0; distLp = 0;
        delay.prepare (sr); reverb.prepare (sr);
        for (auto& r : ratioSm) r = 1.0f;
        for (auto& g : gainSm) g = 0.0f;
        outGainSm = 1.0f;
    }

    void setTempo (double bpm) { tempo = bpm; }

    void process (const float* inL, const float* inR, float* outL, float* outR, int n)
    {
        const int key = (int) prm[KEY];
        const int msk = scaleMask ((int) prm[SCALE]);
        const float amount = prm[AMOUNT], human = prm[HUMANIZE], shift = std::round (prm[SHIFT]);
        const float speedSec = prm[SPEED] / 1000.0f;
        const int harmMode = (int) prm[HARM_INT];
        const float dbl = prm[DOUBLE], harm = prm[HARMONY], mix = prm[MIX];
        float level = 0;

        for (int i = 0; i < n; ++i)
        {
            const float x = inR ? 0.5f * (inL[i] + inR[i]) : inL[i];
            level = std::max (level, std::fabs (x));
            ++t;
            in[(size_t) (t & mask)] = x;
            lpS += (x - lpS) * lpC;
            lp[(size_t) (t & mask)] = lpS;

            // ---- decimated feed for the detector
            const float d = aa2.run (aa1.run (x));
            if (++dPhase >= D)
            {
                dPhase = 0;
                det[(size_t) (dn & 4095)] = d; ++dn;
                if (++hopCount >= 48) { hopCount = 0; detect(); control (key, msk, amount, human, shift, speedSec, harmMode); }
            }

            // ---- analysis marks (pitch-synchronous when voiced)
            updateMarks();

            // ---- voice gains / ratios, lightly smoothed per sample
            const float vt = voiced ? 1.0f : 0.0f;
            voicedAmt += (vt - voicedAmt) * 0.002f;
            const float gT[kVoices] = { 1.0f, dbl * 0.7f, dbl * 0.7f, harm * 0.65f * voicedAmt,
                                        (harmMode == H_3RD_5TH ? harm * 0.55f : 0.0f) * voicedAmt };
            for (int v = 0; v < kVoices; ++v)
            {
                gainSm[(size_t) v] += (gT[v] - gainSm[(size_t) v]) * 0.003f;
                ratioSm[(size_t) v] += (ratioT[(size_t) v] - ratioSm[(size_t) v]) * 0.02f;
            }

            // ---- synthesis: drop grains for each voice as the output clock approaches them
            const int64_t u = t - L;
            for (int v = 0; v < kVoices; ++v)
            {
                double& s = synth[(size_t) v];
                if (s < (double) u) s = (double) u;
                const bool active = gainSm[(size_t) v] > 1e-4f;
                while (s <= (double) (u + Tmax))
                {
                    const float Tc = voiced ? T0 : sr * 0.006f;
                    if (active) grain (v, s, Tc);
                    s += std::max (8.0f, Tc / std::max (0.25f, ratioSm[(size_t) v]));
                }
            }

            // ---- read the output clock
            const size_t ui = (size_t) (u & mask);
            const float lead = ola[0][ui], dL = ola[1][ui], dR = ola[2][ui], hA = ola[3][ui], hB = ola[4][ui];
            for (auto& o : ola) o[ui] = 0.0f;
            const float dry = in[ui];
            float wl = lead + dL + 0.75f * hA + 0.45f * hB;
            float wr = lead + dR + 0.45f * hA + 0.75f * hB;
            outL[i] = dry * (1.0f - mix) + wl * mix;
            outR[i] = dry * (1.0f - mix) + wr * mix;
        }

        guiLevel.store (level);
        effects (outL, outR, n);
    }

private:
    // --------------------------------------------------------------- detection (YIN)
    void detect()
    {
        const int need = W + tauMax;
        if (dn < need) return;
        const int64_t start = dn - need;
        auto X = [&] (int64_t k) { return det[(size_t) ((start + k) & 4095)]; };
        double energy = 0;
        for (int j = 0; j < W; ++j) energy += X (j) * X (j);
        const float rms = (float) std::sqrt (energy / W);

        // difference function + cumulative mean normalisation
        yin[0] = 1.0f; double running = 0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double s = 0;
            for (int j = 0; j < W; ++j) { const float df = X (j) - X (j + tau); s += df * df; }
            running += s;
            yin[(size_t) tau] = (float) (running > 0 ? s * tau / running : 1.0);
        }
        int best = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
            if (yin[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < tauMax && yin[(size_t) tau + 1] < yin[(size_t) tau]) ++tau;
                best = tau; break;
            }
        if (best < 0)
        {
            float mv = 1e9f;
            for (int tau = tauMin; tau < tauMax; ++tau) if (yin[(size_t) tau] < mv) { mv = yin[(size_t) tau]; best = tau; }
        }
        const float conf = yin[(size_t) best];
        const bool isVoiced = rms > 0.004f && conf < 0.3f;
        if (! isVoiced) { voiced = false; guiVoiced = false; return; }

        // parabolic interpolation
        float tauF = (float) best;
        if (best > 1 && best < tauMax)
        {
            const float a = yin[(size_t) best - 1], b = yin[(size_t) best], c = yin[(size_t) best + 1];
            const float den = a - 2 * b + c;
            if (std::fabs (den) > 1e-9f) tauF += 0.5f * (a - c) / den;
        }
        const float f0 = dsr / tauF;
        float m = 69.0f + 12.0f * std::log2 (f0 / 440.0f);

        // median of 3 to reject single-frame octave errors
        mHist[0] = mHist[1]; mHist[1] = mHist[2]; mHist[2] = m;
        float a = mHist[0], b = mHist[1], c = mHist[2];
        const float med = std::max (std::min (a, b), std::min (std::max (a, b), c));
        if (voiced && std::fabs (m - med) > 5.0f) m = med;

        curMidi = m;
        T0 = sr / (440.0f * std::exp2 ((m - 69.0f) / 12.0f));
        T0 = std::clamp (T0, sr / kMaxHz, (float) Tmax);
        if (! voiced) mSlow = m;
        voiced = true; guiVoiced = true;
    }

    // --------------------------------------------------------------- correction curve
    void control (int key, int msk, float amount, float human, float shift, float speedSec, int harmMode)
    {
        const float hop = 48.0f * (float) D / sr;
        float ratioLead = std::exp2 (shift / 12.0f);
        if (voiced)
        {
            const float m = curMidi;
            mSlow += (m - mSlow) * (1.0f - std::exp (-hop / 0.15f));
            // hysteresis: keep the current note until the singer clearly moves
            const int cand = nearestNote (m, key, msk);
            if (target < 0 || ! inScale (target, key, msk) || std::fabs (m - (float) target) > 0.62f) target = cand;
            const float mEff = human * mSlow + (1.0f - human) * m;
            const float want = (float) target - mEff;
            const float k = speedSec <= 0.0005f ? 1.0f : 1.0f - std::exp (-hop / speedSec);
            corr += (want - corr) * k;
            const float semis = amount * corr + shift;
            ratioLead = std::exp2 (semis / 12.0f);
            guiInMidi = m; guiOutMidi = m + semis; guiTarget = target;

            int hA = target, hB = target;
            switch (harmMode)
            {
                case H_3RD_UP:   hA = scaleStep (target, 2, key, msk); break;
                case H_5TH_UP:   hA = scaleStep (target, 4, key, msk); break;
                case H_3RD_5TH:  hA = scaleStep (target, 2, key, msk); hB = scaleStep (target, 4, key, msk); break;
                case H_OCT_DOWN: hA = target - 12; break;
                case H_3RD_DOWN: hA = scaleStep (target, -2, key, msk); break;
                default: break;
            }
            ratioT[3] = ratioLead * std::exp2 ((float) (hA - target) / 12.0f);
            ratioT[4] = ratioLead * std::exp2 ((float) (hB - target) / 12.0f);
        }
        else
        {
            guiTarget = -1;
            ratioT[3] = ratioT[4] = ratioLead;
        }
        ratioT[0] = ratioLead;
        ratioT[1] = ratioLead * std::exp2 (0.09f / 12.0f);
        ratioT[2] = ratioLead * std::exp2 (-0.11f / 12.0f);
    }

    // --------------------------------------------------------------- PSOLA
    void updateMarks()
    {
        const float T = voiced ? T0 : sr * 0.006f;
        while ((float) marks.back() + 1.3f * T <= (float) t)
        {
            int64_t cand = marks.back() + (int64_t) std::lround (T);
            if (voiced)
            {
                const int r = (int) (0.3f * T);
                int64_t bestPos = cand; float bestV = -1e9f;
                for (int64_t p = cand - r; p <= cand + r; ++p)
                {
                    const float v = lp[(size_t) (p & mask)];
                    if (v > bestV) { bestV = v; bestPos = p; }
                }
                cand = std::max (bestPos, marks.back() + (int64_t) (0.5f * T));
            }
            marks.push_back (cand);
        }
        while (marks.size() > 3 && marks[1] < t - L - 3 * Tmax) marks.pop_front();
    }

    int64_t nearestMark (double pos) const
    {
        int64_t best = marks.front(); double bd = 1e18;
        for (auto m : marks) { const double dd = std::fabs ((double) m - pos); if (dd < bd) { bd = dd; best = m; } }
        return best;
    }

    void grain (int v, double centre, float Tc)
    {
        static const float offsets[kVoices] = { 0.0f, 0.011f, 0.017f, 0.0f, 0.0f };   // double voices read slightly late
        const int64_t a = nearestMark (centre - offsets[v] * sr);
        int Tg = (int) std::lround (Tc);
        Tg = std::min (Tg, (int) (t - a));            // never read past the newest sample
        if (Tg < 4) return;
        const int64_t c = (int64_t) std::llround (centre);
        auto& o = ola[(size_t) v];
        const float g = gainSm[(size_t) v];
        const float inv = (float) kPiD / (float) Tg;
        for (int k = -Tg + 1; k < Tg; ++k)
        {
            const float w = 0.5f * (1.0f + std::cos (inv * (float) k));
            o[(size_t) ((c + k) & mask)] += in[(size_t) ((a + k) & mask)] * w * g;
        }
    }

    // --------------------------------------------------------------- effects
    void effects (float* L_, float* R_, int n)
    {
        const float drive = prm[DIST_DRIVE], dmix = prm[DIST_MIX];
        if (dmix > 0.001f && drive > 0.001f)
        {
            const float g = std::exp2 (drive * 5.0f);                  // 1x .. 32x
            const float fc = 800.0f * std::exp2 (prm[DIST_TONE] * 4.0f); // 0.8 .. 12.8 kHz
            const float c = 1.0f - std::exp (-2.0f * (float) kPiD * fc / sr);
            const float norm = 1.0f / ae8::fastTanh (g);
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (L_[i] + R_[i]);
                float y = ae8::fastTanh (m * g) * norm;
                distLp += (y - distLp) * c;
                const float wet = distLp * 0.8f;
                L_[i] = L_[i] * (1 - dmix) + wet * dmix;
                R_[i] = R_[i] * (1 - dmix) + wet * dmix;
            }
        }
        const float dTime = (float) (delayBeats ((int) prm[DLY_TIME]) * 60.0 / std::clamp (tempo, 40.0, 300.0));
        delay.process (L_, R_, n, std::min (dTime, 1.5f), prm[DLY_FB], prm[DLY_MIX]);
        reverb.process (L_, R_, n, prm[REV_SIZE], prm[REV_MIX]);
        const float og = std::pow (10.0f, prm[OUTPUT] / 20.0f);
        for (int i = 0; i < n; ++i) { outGainSm += (og - outGainSm) * 0.001f; L_[i] *= outGainSm; R_[i] *= outGainSm; }
    }

    std::array<float, NUM_PARAMS> prm {};
    float sr = 48000.0f;
    int Tmax = 600, L = 1700, size = 0;
    int64_t mask = 0, t = 0;
    std::vector<float> in, lp;
    std::array<std::vector<float>, kVoices> ola;
    std::deque<int64_t> marks;
    std::array<double, kVoices> synth {};
    std::array<float, kVoices> ratioT { 1, 1, 1, 1, 1 }, ratioSm { 1, 1, 1, 1, 1 }, gainSm {};

    // detector
    int D = 4; float dsr = 12000.0f;
    Biquad aa1, aa2;
    std::vector<float> det, yin;
    int64_t dn = 0; int dPhase = 0, hopCount = 0, W = 256, tauMax = 150, tauMin = 12;
    float T0 = 240.0f, curMidi = 60.0f, mSlow = 60.0f, mHist[3] { 60, 60, 60 };
    bool voiced = false; float voicedAmt = 0;
    int target = -1; float corr = 0;

    float lpS = 0, lpC = 0.1f, distLp = 0, outGainSm = 1;
    double tempo = 120.0;
    ae8::PingPongDelay delay; ae8::Reverb reverb;
};
} // namespace ae8tune
