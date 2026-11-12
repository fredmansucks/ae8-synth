#pragma once
#include "../Source/ParamDefs.h"
#include <vector>

namespace ae8chop
{
using ae8::ParamDef; using ae8::Float; using ae8::Int; using ae8::Choice; using ae8::Bool;

enum P
{
    CHOP_MODE, SENS, SLICES, DIVISION,
    BPM_SYNC, KEY_SYNC, TARGET_KEY, WARP,
    LAYOUT, VOICE, REPEAT, REPEAT_RATE, TRIPLET, GLIDE, SWING, VEL,
    CUTOFF, RES, FTYPE, BITS, RATE, DRIVE,
#ifdef AE8_FLIP
    FLIP_MODE,
#endif
    NUM_PARAMS
};

inline const std::vector<const char*>& keyNames24()
{
    static const std::vector<const char*> k = { "C MAJ", "C# MAJ", "D MAJ", "D# MAJ", "E MAJ", "F MAJ", "F# MAJ", "G MAJ", "G# MAJ", "A MAJ", "A# MAJ", "B MAJ",
                                                "C MIN", "C# MIN", "D MIN", "D# MIN", "E MIN", "F MIN", "F# MIN", "G MIN", "G# MIN", "A MIN", "A# MIN", "B MIN" };
    return k;
}

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> d =
    {
        { "chopmode", "Chop Mode", Choice, 0, 3, 0, 0, { "Transient", "Beat", "Equal", "Manual" }, "" },
        { "sens", "Sensitivity", Float, 0, 1, 0.35f, 0, {}, "%" },
        { "slices", "Slices", Int, 2, 32, 16, 0, {}, "" },
        { "division", "Beat Division", Choice, 0, 4, 2, 0, { "1 Bar", "1/2", "1/4", "1/8", "1/16" }, "" },
        { "bpmsync", "BPM Sync", Bool, 0, 1, 0, 0, {}, "" },
        { "keysync", "Key Sync", Bool, 0, 1, 0, 0, {}, "" },
        { "targetkey", "Target Key", Choice, 0, 23, 21, 0, keyNames24(), "" },
        { "warp", "Warp", Bool, 0, 1, 0, 0, {}, "" },
        { "layout", "Pad Layout", Choice, 0, 1, 0, 0, { "Slices", "Pitch" }, "" },
        { "voice", "Voice Mode", Choice, 0, 2, 0, 0, { "Mono", "Poly", "Gate" }, "" },
        { "repeat", "Note Repeat", Bool, 0, 1, 0, 0, {}, "" },
        { "reprate", "Repeat Rate", Choice, 0, 3, 2, 0, { "1/4", "1/8", "1/16", "1/32" }, "" },
        { "triplet", "Triplet", Bool, 0, 1, 0, 0, {}, "" },
        { "glide", "Glide", Float, 0, 500, 0, 80, {}, "ms" },
        { "swing", "Swing", Float, 0.5f, 0.75f, 0.5f, 0, {}, "%" },
        { "vel", "Velocity", Float, 0, 1, 0.7f, 0, {}, "%" },
        { "cutoff", "Cutoff", Float, 20, 20000, 20000, 1000, {}, "Hz" },
        { "res", "Resonance", Float, 0, 1, 0.1f, 0, {}, "%" },
        { "ftype", "Filter Type", Choice, 0, 2, 0, 0, { "LP", "HP", "BP" }, "" },
        { "bits", "Bits", Float, 4, 16, 16, 0, {}, "bits" },
        { "rate", "Rate", Float, 2000, 48000, 48000, 12000, {}, "Hz" },
        { "drive", "Drive", Float, 0, 1, 0, 0, {}, "%" },
#ifdef AE8_FLIP
        { "flipmode", "Flip Mode", Choice, 0, 6, 0, 0, { "Original Chops", "Reverse Phrase", "Displaced", "Rhythmic Invert", "Half Time", "Stutter Skip", "Alternate Starts" }, "" },
#endif
    };
    return d;
}

// per-slice settings (stored in the plugin state, edited from the SLICE panel)
enum SP { S_PITCH, S_FINE, S_STRETCH, S_LEVEL, S_PAN, S_START, S_LENGTH, S_ATTACK, S_RELEASE, S_FILTER, S_REVERSE, S_ONESHOT, S_LOOP, S_CHOKE, NUM_SP };

struct SliceParamInfo { const char* id; float min, max, def; };
inline const SliceParamInfo& sliceInfo (int i)
{
    static const SliceParamInfo s[NUM_SP] =
    {
        { "pitch", -24, 24, 0 }, { "fine", -50, 50, 0 }, { "stretch", 0.5f, 2.0f, 1.0f }, { "level", -36, 6, 0 }, { "pan", -1, 1, 0 },
        { "start", -20, 200, 0 }, { "length", 0.05f, 1.0f, 1.0f }, { "attack", 0, 500, 1 }, { "release", 5, 2000, 60 }, { "filter", 200, 20000, 20000 },
        { "reverse", 0, 1, 0 }, { "oneshot", 0, 1, 1 }, { "loop", 0, 1, 0 }, { "choke", 0, 3, 0 },
    };
    return s[i];
}
constexpr int kMaxSlices = 32;
} // namespace ae8chop
