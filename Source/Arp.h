#pragma once
// AE-8 POLY — tempo-synced arpeggiator (framework-free)
#include "ParamDefs.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ae8
{
struct ArpEvent { int pos; int note; float vel; bool on; };

class Arpeggiator
{
public:
    // ---- settings (set every block)
    int mode = A_UP, octaves = 1, rate = 3;
    float gate = 0.6f;
    bool latch = false;

    static double stepBeats (int r)
    {
        switch (r)
        {
            case 0: return 1.0;           // 1/4
            case 1: return 0.5;           // 1/8
            case 2: return 1.0 / 3.0;     // 1/8T
            case 3: return 0.25;          // 1/16
            case 4: return 1.0 / 6.0;     // 1/16T
            case 5: return 0.125;         // 1/32
            case 6: return 0.75;          // 1/8 dotted
            default: return 0.25;
        }
    }

    void reset() { held.clear(); latched.clear(); physicalDown = 0; step = 0; playing = -1; pendingOff = false; wasEmpty = true; startNow = false; }

    // keyboard input (from the host / on-screen keyboard)
    void noteOn (int note, float vel)
    {
        if (latch && physicalDown == 0) latched.clear();   // new chord after full release replaces the latch
        ++physicalDown;
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const Key& k) { return k.note == note; }), held.end());
        held.push_back ({ note, vel });
        if (latch) { if (std::none_of (latched.begin(), latched.end(), [note] (const Key& k) { return k.note == note; })) latched.push_back ({ note, vel }); }
        if (wasEmpty) { step = 0; startNow = true; }
        wasEmpty = false;
    }
    void noteOff (int note)
    {
        physicalDown = std::max (0, physicalDown - 1);
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const Key& k) { return k.note == note; }), held.end());
        if (activeKeys().empty()) wasEmpty = true;
    }

    // Advance the arp across samples [s0, s1) of the current block.
    // beatAtBlockStart/beatsPerSample describe the timeline (host-synced or free-running).
    void advance (int s0, int s1, double beat0, double bps, bool hostPlaying, std::vector<ArpEvent>& out)
    {
        const double sb = stepBeats (rate);
        const auto keys = activeKeys();
        auto beatAt = [&] (double s) { return beat0 + s * bps; };

        if (keys.empty())
        {
            if (playing >= 0) { out.push_back ({ s0, playing, 0.0f, false }); playing = -1; pendingOff = false; }
            return;
        }

        if (startNow)
        {
            startNow = false;
            if (! hostPlaying) nextStep = beatAt (s0);                    // free-run: play immediately
            else nextStep = std::ceil (beatAt (s0) / sb - 1e-9) * sb;     // synced: wait for the grid
        }
        if (! hostPlaying && nextStep < beatAt (s0) - sb * 4) nextStep = beatAt (s0);  // timeline jumped back

        if (hostPlaying && nextStep > beatAt (s0) + sb * 4) nextStep = std::ceil (beatAt (s0) / sb - 1e-9) * sb;

        for (;;)
        {
            // gate-off before the next step?
            if (pendingOff && offBeat < nextStep && offBeat < beatAt (s1))
            {
                const int pos = std::clamp ((int) std::floor ((offBeat - beat0) / bps), s0, s1 - 1);
                out.push_back ({ pos, playing, 0.0f, false });
                playing = -1; pendingOff = false;
                continue;
            }
            if (nextStep >= beatAt (s1)) break;
            const int pos = std::clamp ((int) std::floor ((nextStep - beat0) / bps), s0, s1 - 1);
            if (playing >= 0) { out.push_back ({ pos, playing, 0.0f, false }); playing = -1; pendingOff = false; }

            const auto pattern = buildPattern (keys);
            if (! pattern.empty())
            {
                const Key k = pattern[(size_t) (pickIndex ((int) pattern.size()))];
                out.push_back ({ pos, k.note, k.vel, true });
                playing = k.note;
                offBeat = nextStep + sb * std::clamp ((double) gate, 0.05, 1.0) * 0.98;
                pendingOff = true;
            }
            ++step;
            nextStep += sb;
        }
    }

    void syncLatch() { latched = held; }

    void allOff (int pos, std::vector<ArpEvent>& out)
    {
        if (playing >= 0) out.push_back ({ pos, playing, 0.0f, false });
        playing = -1; pendingOff = false;
    }

private:
    struct Key { int note; float vel; };

    std::vector<Key> activeKeys() const { return latch ? latched : held; }

    std::vector<Key> buildPattern (std::vector<Key> keys) const
    {
        if (mode != A_PLAYED) std::sort (keys.begin(), keys.end(), [] (const Key& a, const Key& b) { return a.note < b.note; });
        std::vector<Key> p;
        for (int o = 0; o < std::max (1, octaves); ++o)
            for (auto k : keys) if (k.note + 12 * o <= 127) p.push_back ({ k.note + 12 * o, k.vel });
        if (mode == A_DOWN) std::reverse (p.begin(), p.end());
        if (mode == A_UPDOWN && p.size() > 2)
            for (int i = (int) p.size() - 2; i >= 1; --i) p.push_back (p[(size_t) i]);
        return p;
    }

    int pickIndex (int n)
    {
        if (mode == A_RANDOM)
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            return (int) (rng % (uint32_t) n);
        }
        return (int) (step % n);
    }

    std::vector<Key> held, latched;
    int physicalDown = 0;
    long step = 0;
    int playing = -1;
    bool pendingOff = false, startNow = false, wasEmpty = true;
    double nextStep = 0, offBeat = 0;
    uint32_t rng = 0x2545F491u;
};
} // namespace ae8
