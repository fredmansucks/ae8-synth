#pragma once
// AE-8 Analog Poly — DSP engine (framework-free C++17)
//
// 12-voice virtual-analog polysynth modelled loosely on classic 70s/80s polys:
//   2 PolyBLEP VCOs (+ hard sync, cross-mod, env->pitch), sub osc, noise
//   Juno-style HPF, 24 dB ladder (Moog/Jupiter style) or 12 dB SVF (Oberheim style)
//   analog-curve ADSRs, global LFO with per-voice delay, glide, mono/unison modes,
//   per-voice analog drift, BBD-style stereo chorus, ping-pong delay, plate-ish reverb.

#include "ParamDefs.h"
#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace ae8
{
static constexpr float kPi = 3.14159265358979f;
static constexpr float kTwoPi = 6.28318530717959f;

inline float fastTanh (float x)
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

struct Rng
{
    uint32_t s = 0x12345678u;
    void seed (uint32_t v) { s = v ? v : 1u; }
    float next()        // 0..1
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (s >> 8) * (1.0f / 16777216.0f);
    }
    float bipolar() { return next() * 2.0f - 1.0f; }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt;          return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float oscSample (int wave, float p, float dt, float pw)
{
    switch (wave)
    {
        case W_SAW:    return 2.0f * p - 1.0f - polyBlep (p, dt);
        case W_PULSE:
        case W_SQUARE:
        {
            const float w = (wave == W_SQUARE) ? 0.5f : pw;
            float v = p < w ? 1.0f : -1.0f;
            v += polyBlep (p, dt);
            float p2 = p + 1.0f - w; if (p2 >= 1.0f) p2 -= 1.0f;
            v -= polyBlep (p2, dt);
            return v;
        }
        case W_TRI:    return 2.0f * std::fabs (2.0f * p - 1.0f) - 1.0f;
        case W_SINE:   return std::sin (kTwoPi * p);
        default:       return 0.0f;
    }
}

//==============================================================================
struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float v = 0.0f;
    float attC = 0, decC = 0, relC = 0, sus = 1;

    void set (float sr, float a, float d, float s, float r)
    {
        attC = std::exp (-1.466f / (std::max (a, 0.0005f) * sr));
        decC = std::exp (-4.6f   / (std::max (d, 0.001f)  * sr));
        relC = std::exp (-4.6f   / (std::max (r, 0.001f)  * sr));
        sus = s;
    }
    void gateOn()  { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    void reset()   { stage = Idle; v = 0.0f; }

    float next()
    {
        switch (stage)
        {
            case Attack:
                v = 1.3f + (v - 1.3f) * attC;
                if (v >= 1.0f) { v = 1.0f; stage = Decay; }
                break;
            case Decay:
                v = sus + (v - sus) * decC;
                if (std::fabs (v - sus) < 1e-4f) { v = sus; stage = Sustain; }
                break;
            case Sustain: v = sus; break;
            case Release:
                v *= relC;
                if (v < 1e-5f) { v = 0.0f; stage = Idle; }
                break;
            case Idle: default: v = 0.0f; break;
        }
        return v;
    }
};

//==============================================================================
struct Voice
{
    bool active = false, gate = false, sustained = false;
    int note = 60;
    float vel = 1.0f;
    uint64_t age = 0;

    float pitch = 60.0f, targetPitch = 60.0f;   // semitones (glided)
    float uniOffset = 0.0f;                    // cents
    float pan = 0.0f;                          // -1..1

    float p1 = 0, p2 = 0, pSub = 0;
    Envelope fEnv, aEnv;

    // filters
    float lad[4] {};
    float ic1 = 0, ic2 = 0;
    float hpS = 0;
    float gCur = 0.1f;

    // drift / lfo delay
    float driftStatic = 0, driftStatic2 = 0, driftSlow = 0, driftTarget = 0;
    float timeOn = 0.0f;

    Rng rng;

    void clearFilters() { lad[0] = lad[1] = lad[2] = lad[3] = 0; ic1 = ic2 = 0; hpS = 0; }
};

//==============================================================================
struct Chorus
{
    std::vector<float> buf; int w = 0; float phase = 0; float lpL = 0, lpR = 0; float sr = 44100;
    void prepare (float s) { sr = s; buf.assign ((size_t) (s * 0.05f) + 4, 0.0f); w = 0; }
    float read (float delaySamples) const
    {
        float rp = (float) w - delaySamples;
        const int n = (int) buf.size();
        while (rp < 0) rp += (float) n;
        const int i0 = (int) rp; const float fr = rp - (float) i0;
        const float a = buf[(size_t) (i0 % n)], b = buf[(size_t) ((i0 + 1) % n)];
        return a + (b - a) * fr;
    }
    void process (float* L, float* R, int n, int mode)
    {
        if (mode == C_OFF || buf.empty()) return;
        float rate = 0.513f, centre = 0.0035f, depth = 0.0017f;
        if (mode == C_II)   { rate = 0.863f; }
        if (mode == C_I_II) { rate = 9.75f; depth = 0.0003f; centre = 0.0033f; }
        const float inc = rate / sr;
        const float lpC = 1.0f - std::exp (-kTwoPi * 7500.0f / sr);
        for (int i = 0; i < n; ++i)
        {
            const float in = 0.5f * (L[i] + R[i]);
            buf[(size_t) w] = in;
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            const float tri = 4.0f * std::fabs (phase - 0.5f) - 1.0f;
            const float dl = (centre + depth * tri) * sr;
            const float dr = (centre - depth * tri) * sr;
            lpL += (read (dl) - lpL) * lpC;
            lpR += (read (dr) - lpR) * lpC;
            L[i] = 0.75f * L[i] + 0.75f * lpL;
            R[i] = 0.75f * R[i] + 0.75f * lpR;
            if (++w >= (int) buf.size()) w = 0;
        }
    }
};

struct PingPongDelay
{
    std::vector<float> bl, br; int w = 0; float sr = 44100; float tSm = 0.375f; float lpL = 0, lpR = 0;
    void prepare (float s) { sr = s; bl.assign ((size_t) (s * 1.6f) + 4, 0.0f); br = bl; w = 0; }
    static float rd (const std::vector<float>& b, int w, float d)
    {
        const int n = (int) b.size();
        float rp = (float) w - d; while (rp < 0) rp += (float) n;
        const int i0 = (int) rp; const float fr = rp - (float) i0;
        return b[(size_t) (i0 % n)] + (b[(size_t) ((i0 + 1) % n)] - b[(size_t) (i0 % n)]) * fr;
    }
    void process (float* L, float* R, int n, float time, float fb, float mix)
    {
        if (bl.empty()) return;
        const float smC = 1.0f - std::exp (-1.0f / (0.08f * sr));
        const float lpC = 1.0f - std::exp (-kTwoPi * 4500.0f / sr);
        const bool silentSend = mix <= 0.0001f;
        for (int i = 0; i < n; ++i)
        {
            tSm += (time - tSm) * smC;
            const float d = std::clamp (tSm * sr, 1.0f, (float) bl.size() - 2.0f);
            const float yl = rd (bl, w, d), yr = rd (br, w, d);
            lpL += (yl - lpL) * lpC; lpR += (yr - lpR) * lpC;
            const float in = silentSend ? 0.0f : 0.5f * (L[i] + R[i]);
            bl[(size_t) w] = in + fb * lpR;
            br[(size_t) w] = fb * lpL;
            L[i] += mix * yl; R[i] += mix * yr;
            if (++w >= (int) bl.size()) w = 0;
        }
    }
};

struct Reverb   // Freeverb topology
{
    struct Comb { std::vector<float> b; int i = 0; float st = 0;
        float run (float x, float fb, float damp) { float y = b[(size_t) i]; st = y * (1 - damp) + st * damp; b[(size_t) i] = x + st * fb; if (++i >= (int) b.size()) i = 0; return y; } };
    struct AP { std::vector<float> b; int i = 0;
        float run (float x) { float bo = b[(size_t) i]; float y = -x + bo; b[(size_t) i] = x + bo * 0.5f; if (++i >= (int) b.size()) i = 0; return y; } };
    Comb cl[8], cr[8]; AP al[4], ar[4];
    void prepare (float sr)
    {
        const int ct[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        const int at[4] = { 556, 441, 341, 225 };
        const float k = sr / 44100.0f;
        for (int j = 0; j < 8; ++j) { cl[j].b.assign ((size_t) (ct[j] * k) + 1, 0.0f); cr[j].b.assign ((size_t) ((ct[j] + 23) * k) + 1, 0.0f); cl[j].i = cr[j].i = 0; cl[j].st = cr[j].st = 0; }
        for (int j = 0; j < 4; ++j) { al[j].b.assign ((size_t) (at[j] * k) + 1, 0.0f); ar[j].b.assign ((size_t) ((at[j] + 23) * k) + 1, 0.0f); al[j].i = ar[j].i = 0; }
    }
    void process (float* L, float* R, int n, float size, float mix)
    {
        if (mix <= 0.0001f) return;
        const float fb = 0.70f + size * 0.28f, damp = 0.35f - size * 0.15f;
        for (int s = 0; s < n; ++s)
        {
            const float in = (L[s] + R[s]) * 0.015f;
            float ol = 0, orr = 0;
            for (int j = 0; j < 8; ++j) { ol += cl[j].run (in, fb, damp); orr += cr[j].run (in, fb, damp); }
            for (int j = 0; j < 4; ++j) { ol = al[j].run (ol); orr = ar[j].run (orr); }
            L[s] += mix * ol * 1.2f; R[s] += mix * orr * 1.2f;
        }
    }
};

//==============================================================================
class Engine
{
public:
    static constexpr int kVoices = 12;
    static constexpr int kUnison = 6;
    static constexpr int kCtrl = 16;           // control-rate interval (samples)

    Engine()
    {
        for (int i = 0; i < NUM_PARAMS; ++i) prm[(size_t) i] = paramDefs()[(size_t) i].def;
        for (int i = 0; i < kVoices; ++i)
        {
            voices[(size_t) i].rng.seed (0x9E3779B9u * (uint32_t) (i + 1));
            voices[(size_t) i].driftStatic  = voices[(size_t) i].rng.bipolar();
            voices[(size_t) i].driftStatic2 = voices[(size_t) i].rng.bipolar();
        }
    }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        chorus.prepare (sr); delay.prepare (sr); reverb.prepare (sr);
        allNotesOff (true);
    }

    void setParam (int idx, float v) { prm[(size_t) idx] = v; }
    float getParam (int idx) const    { return prm[(size_t) idx]; }

    // ---- MIDI -------------------------------------------------------------
    void noteOn (int note, float velocity)
    {
        const int mode = imode();
        if (mode == M_POLY)
        {
            Voice& v = findVoice();
            startVoice (v, note, velocity, lastPitch, true);
            v.uniOffset = 0.0f;
            v.pan = 0.0f;
            lastPitch = (float) note;
        }
        else
        {
            held.erase (std::remove (held.begin(), held.end(), note), held.end());
            held.push_back (note);
            const bool legato = monoGate;
            monoGate = true;
            const int n = (mode == M_UNISON) ? kUnison : 1;
            for (int i = 0; i < kVoices; ++i)
            {
                Voice& v = voices[(size_t) i];
                if (i >= n) { if (v.active && i >= n) v.aEnv.gateOff(), v.fEnv.gateOff(), v.gate = false; continue; }
                if (n > 1)
                {
                    const float t = (float) i / (float) (n - 1) * 2.0f - 1.0f;   // -1..1
                    v.uniOffset = t;
                    v.pan = t;
                }
                else { v.uniOffset = 0.0f; v.pan = 0.0f; }

                if (legato && v.active)
                {
                    v.note = note; v.targetPitch = (float) note; v.vel = velocity;
                    if (prm[GLIDE] <= 0.0005f) v.pitch = v.targetPitch;
                }
                else
                {
                    startVoice (v, note, velocity, v.active ? v.pitch : lastPitch, true);
                }
                v.sustained = false;
            }
            lastPitch = (float) note;
        }
    }

    void noteOff (int note)
    {
        const int mode = imode();
        if (mode == M_POLY)
        {
            for (auto& v : voices)
                if (v.active && v.gate && v.note == note)
                {
                    if (sustainPedal) v.sustained = true;
                    else release (v);
                }
        }
        else
        {
            held.erase (std::remove (held.begin(), held.end(), note), held.end());
            if (! held.empty())
            {
                const int nn = held.back();
                for (auto& v : voices) if (v.active && v.gate) { v.note = nn; v.targetPitch = (float) nn; if (prm[GLIDE] <= 0.0005f) v.pitch = v.targetPitch; }
                lastPitch = (float) nn;
            }
            else if (sustainPedal) { for (auto& v : voices) if (v.active && v.gate) v.sustained = true; }
            else { monoGate = false; for (auto& v : voices) if (v.active) release (v); }
        }
    }

    void setSustain (bool on)
    {
        sustainPedal = on;
        if (! on)
        {
            bool any = false;
            for (auto& v : voices) if (v.active && v.sustained) { release (v); any = true; }
            if (any && held.empty()) monoGate = false;
        }
    }
    void pitchBend (float b)  { bendVal = b; }      // -1..1
    void modWheel (float m)   { mwVal = m; }        // 0..1

    void allNotesOff (bool hard = false)
    {
        held.clear(); monoGate = false;
        for (auto& v : voices)
        {
            if (hard) { v.active = false; v.gate = false; v.aEnv.reset(); v.fEnv.reset(); v.clearFilters(); }
            else release (v);
        }
    }

    // ---- audio --------------------------------------------------------------
    void render (float* L, float* R, int n)
    {
        std::fill (L, L + n, 0.0f);
        std::fill (R, R + n, 0.0f);

        int done = 0;
        while (done < n)
        {
            const int len = std::min (kCtrl, n - done);
            updateLfo (len);
            for (auto& v : voices)
                if (v.active) renderVoice (v, L + done, R + done, len);
            done += len;
        }

        chorus.process (L, R, n, (int) prm[CHORUS]);
        delay.process (L, R, n, prm[DLY_TIME], prm[DLY_FB], prm[DLY_MIX]);
        reverb.process (L, R, n, prm[REV_SIZE], prm[REV_MIX]);

        const float target = std::pow (10.0f, prm[VOLUME] / 20.0f);
        const float c = 1.0f - std::exp (-1.0f / (0.02f * sr));
        for (int i = 0; i < n; ++i)
        {
            volSm += (target - volSm) * c;
            L[i] *= volSm; R[i] *= volSm;
        }
    }

    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.active ? 1 : 0; return c; }

private:
    int imode() const { return std::clamp ((int) std::lround (prm[MODE]), 0, 2); }

    Voice& findVoice()
    {
        for (auto& v : voices) if (! v.active) return v;
        Voice* best = nullptr;
        for (auto& v : voices)    // quietest released voice
            if (! v.gate && (best == nullptr || v.aEnv.v < best->aEnv.v)) best = &v;
        if (best) return *best;
        best = &voices[0];
        for (auto& v : voices) if (v.age < best->age) best = &v;    // oldest
        return *best;
    }

    void startVoice (Voice& v, int note, float vel, float fromPitch, bool trigger)
    {
        const bool wasActive = v.active;
        v.active = true; v.gate = true; v.sustained = false;
        v.note = note; v.vel = vel; v.age = ++ageCounter;
        v.targetPitch = (float) note;
        v.pitch = (prm[GLIDE] > 0.0005f) ? fromPitch : v.targetPitch;
        v.timeOn = 0.0f;
        if (! wasActive)
        {
            v.p1 = v.rng.next(); v.p2 = v.rng.next(); v.pSub = v.p1 * 0.5f;
            v.clearFilters();
            v.aEnv.v = 0.0f; v.fEnv.v = 0.0f;
        }
        if (trigger) { v.aEnv.gateOn(); v.fEnv.gateOn(); }
    }

    void release (Voice& v)
    {
        v.gate = false; v.sustained = false;
        v.aEnv.gateOff(); v.fEnv.gateOff();
    }

    void updateLfo (int len)
    {
        const float rate = prm[LFO_RATE];
        lfoPhase += rate * (float) len / sr;
        bool wrapped = false;
        while (lfoPhase >= 1.0f) { lfoPhase -= 1.0f; wrapped = true; }
        if (wrapped) shValue = lfoRng.bipolar();
        const float p = lfoPhase;
        switch ((int) prm[LFO_WAVE])
        {
            case L_TRI:    lfoVal = 1.0f - 4.0f * std::fabs (p - 0.5f) ; lfoVal = -lfoVal; break;
            case L_SINE:   lfoVal = std::sin (kTwoPi * p); break;
            case L_SAW:    lfoVal = 1.0f - 2.0f * p; break;
            case L_SQUARE: lfoVal = p < 0.5f ? 1.0f : -1.0f; break;
            default:       lfoVal = shValue; break;
        }
    }

    void renderVoice (Voice& v, float* L, float* R, int len)
    {
        const float dtSec = (float) len / sr;
        v.timeOn += dtSec;

        // --- envelopes params
        v.fEnv.set (sr, prm[F_A], prm[F_D], prm[F_S], prm[F_R]);
        v.aEnv.set (sr, prm[A_A], prm[A_D], prm[A_S], prm[A_R]);

        // --- glide
        if (prm[GLIDE] > 0.0005f)
        {
            const float c = 1.0f - std::exp (-dtSec / (prm[GLIDE] * 0.35f));
            v.pitch += (v.targetPitch - v.pitch) * c;
        }
        else v.pitch = v.targetPitch;

        // --- drift (slow random walk)
        const float drift = prm[DRIFT];
        if (v.rng.next() < dtSec * 2.0f) v.driftTarget = v.rng.bipolar();
        v.driftSlow += (v.driftTarget - v.driftSlow) * std::min (1.0f, dtSec * 1.5f);
        const float driftCents = drift * (v.driftStatic * 5.0f + v.driftSlow * 4.0f);

        // --- LFO with per-voice delay (fade-in)
        const float dly = prm[LFO_DELAY];
        const float fade = dly <= 0.001f ? 1.0f : std::clamp (v.timeOn / dly, 0.0f, 1.0f);
        const float lfo = lfoVal * fade;

        const float lp = prm[LFO_PITCH];
        const float vibSemis = lfo * (12.0f * lp * lp) + lfoVal * mwVal * prm[MW_VIB] * 0.6f;

        const float uniCents = v.uniOffset * prm[UNI_DETUNE] * 22.0f;
        const float basePitch = v.pitch + bendVal * prm[BEND] + (driftCents + uniCents) / 100.0f + vibSemis;

        const float fEnvNow = v.fEnv.v;
        const float p1 = basePitch + 12.0f * std::round (prm[OSC1_OCT]);
        const float p2 = basePitch + 12.0f * std::round (prm[OSC2_OCT]) + std::round (prm[OSC2_SEMI]) + prm[OSC2_DETUNE] / 100.0f
                         + prm[ENV_PITCH] * fEnvNow * 36.0f;
        const float nyq = 0.45f;
        const float dt1 = std::min (nyq, 440.0f * std::exp2 ((p1 - 69.0f) / 12.0f) / sr);
        float dt2 = std::min (nyq, 440.0f * std::exp2 ((p2 - 69.0f) / 12.0f) / sr);
        const float ratio = fmRatios()[std::clamp ((int) prm[FM_RATIO], 0, 15)];
        if (ratio > 0.0f)   // DX-style: Osc 2 locked to a ratio of Osc 1 (fine detune still applies)
            dt2 = std::min (nyq, dt1 * ratio * std::exp2 ((prm[OSC2_DETUNE] / 100.0f + prm[ENV_PITCH] * fEnvNow * 36.0f) / 12.0f));
        const float fmBase = prm[FM_AMT] * prm[FM_AMT] * 9.0f * (1.0f - prm[FM_VEL] + prm[FM_VEL] * v.vel);
        const float fmEnvAmt = prm[FM_ENV];

        const float pw = std::clamp (prm[PW] + lfo * prm[PWM_LFO] * 0.42f, 0.04f, 0.96f);

        // --- filter cutoff (in octaves)
        const float keyOct = prm[FLT_KEY] * (v.pitch - 60.0f) / 12.0f;
        float oct = std::log2 (prm[CUTOFF])
                  + prm[FLT_ENV] * fEnvNow * 7.0f
                  + keyOct
                  + prm[FLT_LFO] * lfo * 3.0f
                  + prm[FLT_VEL] * (v.vel - 0.75f) * 3.0f
                  + drift * v.driftStatic2 * 0.12f;
        const float fc = std::clamp (std::exp2 (oct), 20.0f, std::min (20000.0f, sr * 0.45f));
        const float gTarget = std::tan (kPi * fc / sr);
        const float gInc = (gTarget - v.gCur) / (float) len;

        const float hpG = std::tan (kPi * std::min (prm[HPF], sr * 0.4f) / sr);
        const float hpA = hpG / (1.0f + hpG);

        const int w1 = (int) prm[OSC1_WAVE], w2 = (int) prm[OSC2_WAVE];
        const float l1 = prm[OSC1_LEVEL], l2 = prm[OSC2_LEVEL], ls = prm[SUB_LEVEL], ln = prm[NOISE_LEVEL];
        const bool sync = prm[OSC2_SYNC] > 0.5f;
        const float xmod = prm[XMOD];
        const int ftype = (int) prm[FLT_TYPE];
        const float res = prm[RESO];
        const float drv = 1.0f + prm[DRIVE] * 3.0f;
        const float kLad = res * 4.1f;
        const float kSvf = 2.0f - 1.96f * res;
        const float velGain = 1.0f - prm[AMP_VEL] + prm[AMP_VEL] * v.vel;
        const float sp = prm[SPREAD];
        const float panPos = std::clamp (v.pan * sp + (std::fabs (v.uniOffset) < 1e-6f ? sp * 0.35f * v.driftStatic : 0.0f), -1.0f, 1.0f);
        const float gl = std::sqrt (0.5f * (1.0f - panPos)), gr = std::sqrt (0.5f * (1.0f + panPos));

        for (int i = 0; i < len; ++i)
        {
            const float fe = v.fEnv.next();
            const float ae = v.aEnv.next();

            // osc 2 first (it modulates osc 1)
            const float o2 = oscSample (w2, v.p2, dt2, pw);
            const float d1 = std::clamp (dt1 * (1.0f + xmod * 2.5f * o2), 0.0f, nyq);

            float ph1 = v.p1;
            if (fmBase > 0.0f)   // phase modulation of Osc 1 by Osc 2
            {
                const float idx = fmBase * (1.0f - fmEnvAmt + fmEnvAmt * fe);
                ph1 += idx * o2 * (1.0f / kTwoPi);
                ph1 -= std::floor (ph1);
            }
            const float o1 = oscSample (w1, ph1, std::max (d1, 1e-6f), pw);
            float sub = v.pSub < 0.5f ? 1.0f : -1.0f;
            sub += polyBlep (v.pSub, dt1 * 0.5f);
            { float q = v.pSub + 0.5f; if (q >= 1.0f) q -= 1.0f; sub -= polyBlep (q, dt1 * 0.5f); }

            v.p1 += d1;
            v.pSub += d1 * 0.5f; if (v.pSub >= 1.0f) v.pSub -= 1.0f;
            v.p2 += dt2;
            if (v.p1 >= 1.0f)
            {
                v.p1 -= 1.0f;
                if (sync) v.p2 = v.p1 * (dt2 / std::max (d1, 1e-6f));
            }
            while (v.p2 >= 1.0f) v.p2 -= 1.0f;

            float x = (o1 * l1 + o2 * l2 + sub * ls + v.rng.bipolar() * ln) * 0.5f;

            // HPF (one-pole TPT)
            {
                const float vv = (x - v.hpS) * hpA;
                const float lpOut = vv + v.hpS;
                v.hpS = lpOut + vv;
                x -= lpOut;
            }

            v.gCur += gInc;
            const float g = v.gCur;
            float y;
            if (ftype == F_LP24)
            {
                const float G = g / (1.0f + g);
                const float inv = 1.0f / (1.0f + g);
                const float b1 = v.lad[0] * inv, b2 = v.lad[1] * inv, b3 = v.lad[2] * inv, b4 = v.lad[3] * inv;
                const float G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
                const float S = G3 * b1 + G2 * b2 + G * b3 + b4;
                float u = (x * drv - kLad * S) / (1.0f + kLad * G4);
                u = fastTanh (u);
                float in = u;
                for (int s = 0; s < 4; ++s)
                {
                    const float vv = (in - v.lad[s]) * G;
                    const float o = vv + v.lad[s];
                    v.lad[s] = o + vv;
                    in = o;
                }
                y = in * (1.0f + kLad * 0.3f) / std::max (1.0f, drv * 0.7f);
            }
            else
            {
                const float k = kSvf;
                const float a1 = 1.0f / (1.0f + g * (g + k));
                const float a2 = g * a1, a3 = g * a2;
                const float v0 = fastTanh (x * drv * 0.8f);
                const float v3 = v0 - v.ic2;
                const float v1 = a1 * v.ic1 + a2 * v3;
                const float v2 = v.ic2 + a2 * v.ic1 + a3 * v3;
                v.ic1 = 2.0f * v1 - v.ic1;
                v.ic2 = 2.0f * v2 - v.ic2;
                v.ic1 = fastTanh (v.ic1 * 0.5f) * 2.0f;    // soft-limit resonance
                if (ftype == F_LP12)      y = v2;
                else if (ftype == F_BP)   y = v1 * 1.4f;
                else                      y = v0 - k * v1 - v2;
                y /= std::max (1.0f, drv * 0.6f);
            }

            const float out = y * ae * velGain * 0.55f;
            L[i] += out * gl;
            R[i] += out * gr;
        }

        if (v.aEnv.stage == Envelope::Idle) { v.active = false; v.gate = false; }
        if (! std::isfinite (v.lad[0] + v.lad[3] + v.ic1 + v.ic2)) v.clearFilters();
    }

    float sr = 44100.0f;
    std::array<float, NUM_PARAMS> prm {};
    std::array<Voice, kVoices> voices;
    std::vector<int> held;
    bool monoGate = false, sustainPedal = false;
    float lastPitch = 60.0f;
    uint64_t ageCounter = 0;
    float bendVal = 0.0f, mwVal = 0.0f;
    float lfoPhase = 0.0f, lfoVal = 0.0f, shValue = 0.0f;
    Rng lfoRng;
    float volSm = 0.5f;

    Chorus chorus; PingPongDelay delay; Reverb reverb;
};
} // namespace ae8
