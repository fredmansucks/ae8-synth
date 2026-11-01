#pragma once
// AE-8 CHOP — playback engine (framework-free C++17)
//
//  pads/notes -> voices (per-slice pitch, stretch, envelope, filter, pan, reverse, one-shot / loop, choke)
//  warp     : when time and pitch must differ (BPM sync, stretch, or Warp on), a voice plays through a
//             4-grain overlap-add stretcher; otherwise it is a clean varispeed read (classic repitch)
//  repeat   : note repeat locked to the host grid with swing
//  color    : drive -> bit / rate crush -> state-variable filter

#include "ChopParams.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>

namespace ae8chop
{
struct Sample
{
    std::vector<float> L, R;
    double sr = 44100;
    int64_t size() const { return (int64_t) L.size(); }
};

struct NoteEvent { int offset; int note; float vel; bool on; };

struct SliceSettings
{
    std::array<std::atomic<float>, NUM_SP> v;
    SliceSettings() { reset(); }
    void reset() { for (int i = 0; i < NUM_SP; ++i) v[(size_t) i] = sliceInfo (i).def; }
    float operator[] (int i) const { return v[(size_t) i].load(); }
};

class ChopEngine
{
public:
    // ---- shared state (written by the message thread)
    std::shared_ptr<const Sample> sample;                    // swap with std::atomic_store
    std::array<std::atomic<int64_t>, kMaxSlices> starts {};
    std::atomic<int> numSlices { 0 };
    std::array<SliceSettings, kMaxSlices> slice;
    std::atomic<int> selected { 0 };
    std::atomic<double> sampleBpm { 120.0 };
    std::atomic<int> keyShiftSemis { 0 };                    // computed from detected + target key

    // ---- read by the GUI
    std::atomic<uint32_t> guiPlaying { 0 };                  // bit per slice
    std::atomic<float> guiPlayhead { -1 };                   // 0..1 of the file, last voice
    std::atomic<int> guiLastSlice { -1 };
    std::atomic<int> triggerCount { 0 };

    ChopEngine() { for (int i = 0; i < NUM_PARAMS; ++i) prm[(size_t) i] = paramDefs()[(size_t) i].def; }
    void setParam (int i, float v) { prm[(size_t) i] = v; }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto& v : voices) v.active = false;
        held.clear();
        svf[0] = svf[1] = {};
        crushHold[0] = crushHold[1] = 0; crushPh = 0;
        cutSm = prm[CUTOFF];
        freeRun = 0;
        held.reserve (128); evBuf.reserve (1024);
    }

    void noteOn (int note, float vel) { events.push_back ({ 0, note, vel, true }); }
    void allNotesOff() { for (auto& v : voices) if (v.active) v.release (sr, 8); held.clear(); }

    // ppq < 0 or !playing: repeat free-runs from the first held note
    void process (float* outL, float* outR, int n, const std::vector<NoteEvent>& midi, double hostBpm, double ppq, bool playing)
    {
        auto smp = std::atomic_load (&sample);
        bpm = hostBpm > 0 ? hostBpm : 120.0;
        std::fill (outL, outL + n, 0.0f);
        std::fill (outR, outR + n, 0.0f);

        // merge queued GUI events (pads) with MIDI
        auto& ev = evBuf;
        ev.assign (midi.begin(), midi.end());
        for (auto& e : events) ev.push_back (e);
        events.clear();
        std::stable_sort (ev.begin(), ev.end(), [] (auto& a, auto& b) { return a.offset < b.offset; });

        const bool repeatOn = prm[REPEAT] > 0.5f;
        static const double rates[4] = { 1.0, 0.5, 0.25, 0.125 };   // in quarter notes
        double stepQ = rates[std::clamp ((int) prm[REPEAT_RATE], 0, 3)];
        if (prm[TRIPLET] > 0.5f) stepQ *= 2.0 / 3.0;
        const double samplesPerQ = 60.0 / bpm * sr;
        const double swingDelay = (prm[SWING] - 0.5) * 2.0 * stepQ;   // quarter notes added to odd steps

        size_t ei = 0;
        for (int i = 0; i < n; ++i)
        {
            while (ei < ev.size() && ev[ei].offset <= i)
            {
                const auto& e = ev[ei++];
                if (e.on)
                {
                    held.push_back ({ e.note, e.vel });
                    if (held.size() == 1) freeRun = 0;
                    trigger (smp.get(), e.note, e.vel);
                }
                else
                {
                    held.erase (std::remove_if (held.begin(), held.end(), [&] (auto& h) { return h.first == e.note; }), held.end());
                    for (auto& v : voices) if (v.active && v.note == e.note && ! v.oneshot) v.release (sr, slice[(size_t) v.slice][S_RELEASE]);
                }
            }

            if (repeatOn && ! held.empty())
            {
                // position in quarter notes, from the host grid when playing
                const double q = playing && ppq >= 0 ? ppq + i / samplesPerQ : freeRun / samplesPerQ;
                const double qPrev = playing && ppq >= 0 ? ppq + (i - 1) / samplesPerQ : (freeRun - 1) / samplesPerQ;
                const auto stepAt = [&] (double x) { // index of the last step boundary at or before x
                    const double k = std::floor (x / stepQ);
                    const double b = k * stepQ + (((int64_t) k & 1) ? swingDelay : 0.0);
                    return x >= b ? k : k - 1; };
                if ((playing && ppq >= 0) || freeRun > 0)
                    if (stepAt (q) != stepAt (qPrev))
                        for (auto& h : held) trigger (smp.get(), h.first, h.second, true);
            }
            ++freeRun;
        }

        // render voices (whole block per voice)
        uint32_t playingMask = 0;
        if (smp && smp->size() > 0)
            for (auto& v : voices)
                if (v.active) { render (*smp, v, outL, outR, n); if (v.active) playingMask |= 1u << v.slice; }
        guiPlaying = playingMask;

        color (outL, outR, n);
    }

private:
    struct Voice
    {
        bool active = false, oneshot = true, loop = false, granular = false, releasing = false, warp = false;
        int note = -1, slice = 0, choke = 0;
        int64_t start = 0, end = 0;
        double pos = 0, dir = 1;
        double semis = 0, semisTarget = 0, glideK = 1;
        double timeRate = 1;                     // source samples per output sample (analysis head)
        float gain = 1, panL = 1, panR = 1;
        float env = 0, attackInc = 1, relMul = 0.999f, endFade = 1;
        float lpK = 1, lp[2] {};
        // grains
        struct G { bool on = false, first = false; double src = 0; int age = 0; };
        std::array<G, 4> g;
        int grainLen = 2048, hopCount = 0;
        int64_t age = 0;
        void release (double sr, float ms)
        {
            if (! releasing) { releasing = true; relMul = (float) std::exp (-1.0 / (std::max (1.0f, ms) * 0.001 * sr / 4.6)); }
        }
    };

    void trigger (const Sample* smp, int note, float vel, bool fromRepeat = false)
    {
        if (! smp || smp->size() == 0) return;
        const int ns = numSlices.load();
        if (ns <= 0) return;
        const bool pitchLayout = prm[LAYOUT] > 0.5f;
        int si; double padSemis = 0;
        if (pitchLayout) { si = std::clamp (selected.load(), 0, ns - 1); padSemis = note - 60; }
        else { si = note - 36; if (si < 0 || si >= ns) return; }

        const auto& s = slice[(size_t) si];
        const int voiceMode = (int) prm[VOICE];
        const int choke = (int) s[S_CHOKE];

        // previous pitch for glide (mono)
        double prevSemis = 0; bool havePrev = false;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            if (voiceMode == 0 || (choke > 0 && v.choke == choke) || (fromRepeat && v.note == note))
            {
                if (! v.releasing) { prevSemis = v.semis; havePrev = true; }
                v.release (sr, 6);
            }
        }

        Voice* v = nullptr;
        for (auto& c : voices) if (! c.active) { v = &c; break; }
        if (! v) { v = &voices[0]; for (auto& c : voices) if (c.age > v->age) v = &c; }   // steal oldest
        *v = Voice();
        v->active = true; v->note = note; v->slice = si; v->choke = choke;

        // slice bounds (in source samples)
        const int64_t len = smp->size();
        int64_t a = starts[(size_t) si].load();
        int64_t b = si + 1 < ns ? starts[(size_t) si + 1].load() : len;
        a = std::clamp<int64_t> (a + (int64_t) (s[S_START] * 0.001 * smp->sr), 0, len - 1);
        b = std::clamp<int64_t> (b, a + 16, len);
        b = a + std::max<int64_t> (16, (int64_t) ((double) (b - a) * s[S_LENGTH]));
        v->start = a; v->end = std::min (b, len);
        const bool rev = s[S_REVERSE] > 0.5f;
        v->dir = rev ? -1 : 1;
        v->pos = rev ? (double) v->end - 1 : (double) v->start;

        v->oneshot = s[S_ONESHOT] > 0.5f && voiceMode != 2 && s[S_LOOP] < 0.5f;
        v->loop = s[S_LOOP] > 0.5f;

        // pitch & time
        const bool bpmSync = prm[BPM_SYNC] > 0.5f, warp = prm[WARP] > 0.5f;
        const double keySemis = prm[KEY_SYNC] > 0.5f ? keyShiftSemis.load() : 0;
        v->semisTarget = s[S_PITCH] + s[S_FINE] / 100.0 + keySemis + padSemis;
        const float glideMs = prm[GLIDE];
        v->semis = (voiceMode == 0 && havePrev && glideMs > 0.5f) ? prevSemis : v->semisTarget;
        v->glideK = glideMs > 0.5f ? 1.0 - std::exp (-1.0 / (glideMs * 0.001 * sr)) : 1.0;
        double T = 1.0 / s[S_STRETCH];
        if (bpmSync) T *= bpm / std::max (20.0, sampleBpm.load());
        const double srcPerOut = smp->sr / sr;
        const bool timeIsUnity = std::fabs (T - 1.0) < 1e-4;
        // classic repitch when time is untouched and warp is off; key sync alone also repitches unless warp is on
        v->granular = ! timeIsUnity || (warp && std::fabs (v->semisTarget) > 1e-4) || (warp && v->semis != v->semisTarget);
        v->timeRate = T * srcPerOut;
        v->warp = warp;
        v->grainLen = (int) (0.07 * sr);
        v->hopCount = 0;

        // level, pan, velocity
        const float velAmt = prm[VEL];
        v->gain = std::pow (10.0f, s[S_LEVEL] / 20.0f) * (1.0f - velAmt * (1.0f - vel));
        const float pan = s[S_PAN];
        v->panL = std::cos ((pan + 1) * 0.25f * 3.14159265f) * 1.41421356f;
        v->panR = std::sin ((pan + 1) * 0.25f * 3.14159265f) * 1.41421356f;
        v->panL = std::min (1.0f, v->panL); v->panR = std::min (1.0f, v->panR);
        const float atk = s[S_ATTACK];
        v->attackInc = atk < 0.5f ? 1.0f : (float) (1.0 / (atk * 0.001 * sr));
        v->env = atk < 0.5f ? 1.0f : 0.0f;
        const float fc = s[S_FILTER];
        v->lpK = fc >= 19999 ? 1.0f : (float) (1.0 - std::exp (-2 * 3.14159265 * fc / sr));
        for (auto& o : voices) if (o.active && &o != v) ++o.age;
        v->age = 0;

        guiLastSlice = si;
        ++triggerCount;
    }

    static float rd (const std::vector<float>& b, double p)
    {
        const int64_t n = (int64_t) b.size();
        const int64_t i = (int64_t) std::floor (p);
        if (i < 0 || i + 1 >= n) return (i >= 0 && i < n) ? b[(size_t) i] : 0.0f;
        const float t = (float) (p - (double) i);
        return b[(size_t) i] + (b[(size_t) i + 1] - b[(size_t) i]) * t;
    }

    void render (const Sample& smp, Voice& v, float* outL, float* outR, int n)
    {
        const double srcPerOut = smp.sr / sr;
        const auto& s = slice[(size_t) v.slice];
        const bool loopNow = v.loop;
        const int64_t span = std::max<int64_t> (1, v.end - v.start);
        const double fadeLen = std::min (0.004 * smp.sr, span * 0.25);

        for (int i = 0; i < n; ++i)
        {
            if (v.semis != v.semisTarget) { v.semis += (v.semisTarget - v.semis) * v.glideK; if (std::fabs (v.semis - v.semisTarget) < 1e-3) v.semis = v.semisTarget; }
            const double P = std::pow (2.0, v.semis / 12.0);

            float l, r;
            if (! v.granular)
            {
                l = rd (smp.L, v.pos); r = rd (smp.R, v.pos);
                v.pos += v.dir * P * srcPerOut;
            }
            else
            {
                // 4 Hann grains, hop = len/4; each grain reads at pitch P from where the analysis head was
                if (v.hopCount <= 0)
                {
                    v.hopCount = v.grainLen / 4;
                    const bool first = v.age == 0 && ! v.g[0].on && ! v.g[1].on && ! v.g[2].on && ! v.g[3].on;
                    for (auto& g : v.g) if (! g.on) { g.on = true; g.first = first; g.src = v.pos; g.age = 0; break; }
                }
                --v.hopCount;
                l = r = 0;
                for (auto& g : v.g)
                {
                    if (! g.on) continue;
                    auto hann = [&] (int a) { return a < v.grainLen ? 0.5f - 0.5f * std::cos (2 * 3.14159265f * (float) a / (float) v.grainLen) : 0.0f; };
                    float w = hann (g.age);
                    if (g.first) for (int k = 1; k < 4; ++k) w += hann (g.age + k * v.grainLen / 4);   // stand in for the grains before the note, so the attack is not faded
                    double p = g.src + v.dir * g.age * P * srcPerOut;
                    if (loopNow) { while (p >= v.end) p -= span; while (p < v.start) p += span; }
                    l += rd (smp.L, p) * w; r += rd (smp.R, p) * w;
                    if (++g.age >= v.grainLen) g.on = false;
                }
                l *= 0.5f; r *= 0.5f;
                v.pos += v.dir * v.timeRate * (v.warp ? 1.0 : P);
            }

            // boundary handling
            const double distEnd = v.dir > 0 ? (double) v.end - v.pos : v.pos - (double) v.start;
            if (distEnd <= 0)
            {
                if (loopNow && ! v.releasing) v.pos += v.dir > 0 ? -(double) span : (double) span;
                else { v.active = false; break; }
            }
            float edge = 1.0f;
            if (! loopNow && distEnd < fadeLen) edge = (float) std::max (0.0, distEnd / fadeLen);
            else if (loopNow)
            {
                const double distStart = v.dir > 0 ? v.pos - (double) v.start : (double) v.end - v.pos;
                if (distEnd < fadeLen) edge = (float) (distEnd / fadeLen);
                else if (distStart < fadeLen) edge = (float) (distStart / fadeLen);
                edge = 0.25f + 0.75f * edge;
            }

            if (v.releasing) { v.env *= v.relMul; if (v.env < 1e-4f) { v.active = false; break; } }
            else if (v.env < 1.0f) v.env = std::min (1.0f, v.env + v.attackInc);

            const float g = v.env * v.gain * edge;
            if (v.lpK < 1.0f) { v.lp[0] += (l - v.lp[0]) * v.lpK; v.lp[1] += (r - v.lp[1]) * v.lpK; l = v.lp[0]; r = v.lp[1]; }
            outL[i] += l * g * v.panL;
            outR[i] += r * g * v.panR;
        }
        if (v.active) guiPlayhead = (float) (v.pos / (double) std::max<int64_t> (1, smp.size()));
        ++v.age;
        (void) s;
    }

    void color (float* L, float* R, int n)
    {
        const float drive = prm[DRIVE];
        const float bits = prm[BITS], rate = prm[RATE];
        const bool crushBits = bits < 15.95f, crushRate = rate < 47900.0f;
        const float q = std::pow (2.0f, bits - 1.0f);
        const double rateInc = std::min (1.0, rate / sr);
        const int ft = (int) prm[FTYPE];
        const float res = prm[RES];
        const bool filterOn = ! (ft == 0 && prm[CUTOFF] >= 19900.0f) && ! (ft == 1 && prm[CUTOFF] <= 21.0f);
        const float dg = 1.0f + drive * 8.0f, dn = 1.0f / std::tanh (dg);
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { L[i], R[i] };
            if (drive > 0.001f) for (auto& v : x) v = std::tanh (v * dg) * dn * (1.0f - drive * 0.3f) + v * drive * 0.0f;
            if (crushRate)
            {
                crushPh += rateInc;
                if (crushPh >= 1.0) { crushPh -= 1.0; crushHold[0] = x[0]; crushHold[1] = x[1]; }
                x[0] = crushHold[0]; x[1] = crushHold[1];
            }
            if (crushBits) for (auto& v : x) v = std::round (v * q) / q;
            if (filterOn)
            {
                cutSm += (prm[CUTOFF] - cutSm) * 0.002f;
                const double gC = std::tan (3.14159265 * std::min (cutSm, (float) (sr * 0.45)) / sr);
                const double k = 2.0 - 1.9 * res;
                const double a1 = 1.0 / (1.0 + gC * (gC + k)), a2 = gC * a1, a3 = gC * a2;
                for (int c = 0; c < 2; ++c)
                {
                    auto& st = svf[c];
                    const double v3 = x[c] - st.ic2, v1 = a1 * st.ic1 + a2 * v3, v2 = st.ic2 + a2 * st.ic1 + a3 * v3;
                    st.ic1 = 2 * v1 - st.ic1; st.ic2 = 2 * v2 - st.ic2;
                    x[c] = (float) (ft == 0 ? v2 : ft == 1 ? x[c] - k * v1 - v2 : v1);
                }
            }
            L[i] = x[0]; R[i] = x[1];
        }
    }

    std::array<float, NUM_PARAMS> prm {};
    double sr = 48000, bpm = 120;
    std::array<Voice, 16> voices;
    std::vector<std::pair<int, float>> held;
    std::vector<NoteEvent> events, evBuf;   // events: offline tests only (the plugin feeds pads through MIDI)
    int64_t freeRun = 0;
    struct SVF { double ic1 = 0, ic2 = 0; } svf[2];
    float crushHold[2] {}; double crushPh = 0; float cutSm = 20000;
};
} // namespace ae8chop
