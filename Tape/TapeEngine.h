#pragma once
// AE-8 TAPE — tape machine engine (framework-free C++17)
//
//  record   : input gain -> bias-dependent pre-emphasis -> asymmetric tape saturation -> de-emphasis
//  NR       : optional compander that pushes hiss down and slightly compresses the top end
//  transport: the signal is "recorded" to a ~32 s tape loop and played back by a head that moves at
//             the Speed setting with motor inertia. Below 100% the audio really slows down (time and
//             pitch together). If the tape falls too far behind, or Speed returns to 100%, the head
//             crossfades back into sync.
//  playback : wow + flutter, head bump, gap loss, wear (dropouts + dulling), hiss, crosstalk, 2-band EQ

#include "TapeParams.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace ae8tape
{
constexpr double kPi = 3.14159265358979323846;

struct BQ
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] {}, z2[2] {};
    void set (double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    void lowShelf (double fs, double f, double db, double S = 0.8)
    {
        const double A = std::pow (10.0, db / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), r = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + r), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - r),
             (A + 1) + (A - 1) * c + r, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - r);
    }
    void highShelf (double fs, double f, double db, double S = 0.8)
    {
        const double A = std::pow (10.0, db / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), r = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + r), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - r),
             (A + 1) - (A - 1) * c + r, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - r);
    }
    void peak (double fs, double f, double db, double q)
    {
        const double A = std::pow (10.0, db / 40.0), w = 2 * kPi * f / fs, al = std::sin (w) / (2 * q), c = std::cos (w);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowpass (double fs, double f, double q = 0.7071)
    {
        f = std::min (f, fs * 0.45);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double fs, double f, double q = 0.7071)
    {
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    float run (float x, int ch)
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }
};

struct Rand { uint32_t s = 0x1234567u; float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (1.0f / 16777216.0f); } float bi() { return next() * 2 - 1; } };

class TapeEngine
{
public:
    // read by the GUI
    std::atomic<float> guiInL { 0 }, guiInR { 0 }, guiOutL { 0 }, guiOutR { 0 };
    std::atomic<float> guiSpeed { 1 }, guiAngleL { 0 }, guiAngleR { 0 }, guiPack { 0 };

    TapeEngine() { for (int i = 0; i < NUM_PARAMS; ++i) prm[(size_t) i] = paramDefs()[(size_t) i].def; }
    void setParam (int i, float v) { prm[(size_t) i] = v; }
    int latencySamples() const { return D0; }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        D0 = (int) std::lround (0.030 * sr);                 // base head distance (reported as latency)
        size = 1; while (size < (int64_t) (34.0 * sr)) size <<= 1;
        mask = size - 1;
        for (auto& t : tape) t.assign ((size_t) size, 0.0f);
        w = (int64_t) D0 + 8;
        headA = (double) (w - D0); headB = headA; fade = 1.0; fading = false;
        maxLag = 24.0 * sr;
        speedSm = 1.0;
        for (auto& v : lpS) v = 0;
        playLP.lowpass (sr, 16000, 0.6);
        bump.peak (sr, 62, 2.5, 0.9);
        bumpDip.peak (sr, 130, -1.0, 1.2);
        xtalkLP.lowpass (sr, 1500);
        hissHP.highpass (sr, 1800);
        hissLP.lowpass (sr, 12000);
        nrHP.highpass (sr, 3000);
        nrHP2.highpass (sr, 3000);
        dcBlock.highpass (sr, 12, 0.6);
        dryLine.assign (2 * ((size_t) D0 + 16), 0.0f); dryW = 0;
        wowPh = 0; flutPh = 0.3; wowRnd = 0; wowRndT = 0; drop = 1; dropT = 1;
        outSm = 1; inSm = 1;
        lastLow = 1e9; lastHigh = 1e9; lastBias = 1e9; lastWear = 1e9;
        envNR[0] = envNR[1] = 0;
    }

    void process (const float* inL, const float* inR, float* outL, float* outR, int n)
    {
        updateFilters();
        const float inG = std::pow (10.0f, prm[INPUT] / 20.0f);
        const float outG = std::pow (10.0f, prm[OUTPUT] / 20.0f);
        const float sat = prm[SATURATE], bias = prm[BIAS];
        const float drive = 1.0f + sat * 7.0f;
        const float asym = 0.08f + 0.1f * std::max (0.0f, -bias);           // under-bias = more even harmonics
        const float norm = 1.0f / std::tanh (drive * 0.9f);
        const bool nr = prm[NR] > 0.5f;
        const float hissAmt = prm[HISS] * prm[HISS] * 0.02f * (nr ? 0.22f : 1.0f);
        const float xt = prm[CROSSTALK] * prm[CROSSTALK] * 0.25f;
        const float wow = prm[WOW], flut = prm[FLUTTER], wear = prm[WEAR];
        const float mix = prm[MIX];
        const double target = prm[STOP] > 0.5f ? 0.0 : (double) std::clamp (prm[SPEED], 0.0f, 1.0f);
        const double kDown = 1.0 - std::exp (-1.0 / (0.45 * sr)), kUp = 1.0 - std::exp (-1.0 / (0.30 * sr));

        float pkIn[2] {}, pkOut[2] {};
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { inL[i] * inG, (inR ? inR[i] : inL[i]) * inG };
            pkIn[0] = std::max (pkIn[0], std::fabs (inL[i])); pkIn[1] = std::max (pkIn[1], std::fabs (inR ? inR[i] : inL[i]));

            // dry, delayed to line up with the tape head at full speed
            float dry[2];
            {
                const size_t L = dryLine.size() / 2;
                for (int c = 0; c < 2; ++c) { dry[c] = dryLine[(size_t) c * L + (size_t) ((dryW + 1) % (int) L)]; dryLine[(size_t) c * L + (size_t) dryW] = c == 0 ? inL[i] : (inR ? inR[i] : inL[i]); }
                dryW = (dryW + 1) % (int) L;
            }

            // ---------------- record path
            for (int c = 0; c < 2; ++c)
            {
                float v = preEmph.run (x[c], c);
                if (nr)   // encoder: lift quiet highs before the tape
                {
                    const float hi = nrHP.run (v, c);
                    envNR[c] += (std::fabs (hi) - envNR[c]) * 0.002f;
                    const float boost = 1.0f + 1.5f / (1.0f + envNR[c] * 40.0f);
                    v += hi * (boost - 1.0f);
                }
                float s = std::tanh (drive * 0.9f * (v + asym * v * v)) * norm;
                v = (1.0f - sat) * v + sat * s;
                v = deEmph.run (v, c);
                tape[(size_t) c][(size_t) (w & mask)] = v;
            }
            ++w;

            // ---------------- transport
            speedSm += (target - speedSm) * (target < speedSm ? kDown : kUp);
            double lagA = (double) w - headA;
            if (! fading)
            {
                const bool tooFar = lagA > maxLag;
                const bool backToSync = target > 0.999 && speedSm > 0.998 && lagA > D0 + 0.004 * sr;
                if (tooFar || backToSync) { headB = (double) (w - D0); fading = true; fade = 0.0; }
            }
            headA += speedSm;
            if (fading) { headB += speedSm; fade += 1.0 / (0.15 * sr); if (fade >= 1.0) { headA = headB; fading = false; fade = 1.0; } }
            if ((double) w - headA < D0 * 0.5) headA = (double) (w - D0);   // never read ahead of the record head

            // wow (slow, wandering) + flutter (fast), as head offsets in samples
            wowPh += (0.55 + 0.25 * wowRnd) / sr; if (wowPh >= 1) wowPh -= 1;
            flutPh += 7.5 / sr; if (flutPh >= 1) flutPh -= 1;
            if (--wowRndT <= 0) { wowRndT = (int) (sr * 0.5); wowRndTarget = rnd.bi(); }
            wowRnd += (wowRndTarget - wowRnd) * (float) (1.0 / (0.6 * sr));
            const double mod = (wow * wow) * 0.0075 * sr * (0.5 + 0.5 * std::sin (2 * kPi * wowPh) + 0.25 * wowRnd)
                             + (flut * flut) * 0.00035 * sr * (0.5 + 0.5 * std::sin (2 * kPi * flutPh) + 0.15 * rnd.bi());
            const double pA = headA - mod, pB = headB - mod;

            // wear: random dropouts
            if (--dropT <= 0)
            {
                dropT = (int) (sr * (0.05 + 0.4 * rnd.next()));
                dropTarget = (rnd.next() < wear * wear * 0.6f) ? 0.35f + 0.5f * rnd.next() : 1.0f;
            }
            drop += (dropTarget - drop) * 0.004f;

            // playback level falls as the tape crawls (playback head output ~ tape speed)
            const float speedGain = (float) std::min (1.0, speedSm * 3.0);

            float y[2];
            for (int c = 0; c < 2; ++c)
            {
                float a = readTape (c, pA);
                if (fading) { const float b = readTape (c, pB); const float g = (float) fade; a = a * std::cos (g * 1.5707963f) + b * std::sin (g * 1.5707963f); }
                y[c] = a;
            }
            for (int c = 0; c < 2; ++c)
            {
                float v = y[c] + xt * xtalkLP.run (y[1 - c], c);
                v = dcBlock.run (v, c);
                v = bumpDip.run (bump.run (v, c), c);
                v = playLP.run (v, c);
                v = wearLP.run (v, c);
                v *= drop * speedGain;
                if (nr)   // decoder: expand the highs back down
                {
                    const float hi = nrHP2.run (v, c);
                    v -= hi * 0.25f * (1.0f / (1.0f + envNR[c] * 40.0f));
                }
                float h = hissLP.run (hissHP.run (rnd.bi(), c), c) * hissAmt * (0.6f + 0.4f * speedGain);
                v += h;
                v = highEQ.run (lowEQ.run (v, c), c);
                y[c] = v;
            }

            outSm += (outG - outSm) * 0.001f;
            outL[i] = (dry[0] * (1.0f - mix) + y[0] * mix) * outSm;
            outR[i] = (dry[1] * (1.0f - mix) + y[1] * mix) * outSm;
            pkOut[0] = std::max (pkOut[0], std::fabs (outL[i])); pkOut[1] = std::max (pkOut[1], std::fabs (outR[i]));
        }

        // ---------------- reels: angle advances with speed; the emptier reel spins faster
        const double rotBase = 1.1 * speedSm * n / sr;            // revolutions this block at hub radius
        pack += speedSm * n / (sr * 240.0);                         // ~4 minutes to wind from left to right
        if (pack >= 1.0) pack -= 1.0;
        const double rL = 0.35 + 0.65 * (1.0 - pack), rR = 0.35 + 0.65 * pack;
        angL = std::fmod (angL + 2 * kPi * rotBase / rL, 2 * kPi);
        angR = std::fmod (angR + 2 * kPi * rotBase / rR, 2 * kPi);
        guiAngleL = (float) angL; guiAngleR = (float) angR; guiPack = (float) pack; guiSpeed = (float) speedSm;
        guiInL = pkIn[0]; guiInR = pkIn[1]; guiOutL = pkOut[0]; guiOutR = pkOut[1];
    }

private:
    float readTape (int c, double pos) const
    {
        const double fl = std::floor (pos);
        const float t = (float) (pos - fl);
        const int64_t i = (int64_t) fl;
        const auto& b = tape[(size_t) c];
        const float y0 = b[(size_t) ((i - 1) & mask)], y1 = b[(size_t) (i & mask)], y2 = b[(size_t) ((i + 1) & mask)], y3 = b[(size_t) ((i + 2) & mask)];
        const float c0 = y1, c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    void updateFilters()
    {
        if (prm[LOW] != lastLow)   { lastLow = prm[LOW];   lowEQ.lowShelf (sr, 110, prm[LOW]); }
        if (prm[HIGH] != lastHigh) { lastHigh = prm[HIGH]; highEQ.highShelf (sr, 7500, prm[HIGH]); }
        if (prm[BIAS] != lastBias)
        {
            lastBias = prm[BIAS];
            // under-bias (negative): brighter, grittier; over-bias (positive): darker, cleaner
            const double pre = 6.0 - 3.0 * lastBias;
            preEmph.highShelf (sr, 3200, pre);
            deEmph.highShelf (sr, 3200, -pre + (-lastBias * 2.0));
        }
        if (prm[WEAR] != lastWear) { lastWear = prm[WEAR]; wearLP.lowpass (sr, 18000.0 * std::pow (0.22, (double) lastWear), 0.6); }
    }

    std::array<float, NUM_PARAMS> prm {};
    double sr = 48000;
    int D0 = 1440;
    int64_t size = 0, mask = 0, w = 0;
    std::array<std::vector<float>, 2> tape;
    double headA = 0, headB = 0, fade = 1, maxLag = 0, speedSm = 1;
    bool fading = false;
    BQ nrHP2, dcBlock, preEmph, deEmph, playLP, bump, bumpDip, xtalkLP, hissHP, hissLP, nrHP, wearLP, lowEQ, highEQ;
    float lpS[2] {};
    std::vector<float> dryLine; int dryW = 0;
    double wowPh = 0, flutPh = 0;
    float wowRnd = 0, wowRndTarget = 0; int wowRndT = 0;
    float drop = 1, dropTarget = 1; int dropT = 1;
    float outSm = 1, inSm = 1, envNR[2] {};
    float lastLow = 1e9f, lastHigh = 1e9f, lastBias = 1e9f, lastWear = 1e9f;
    double pack = 0, angL = 0, angR = 0;
    Rand rnd;
};
} // namespace ae8tape
