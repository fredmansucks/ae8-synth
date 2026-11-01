#pragma once
// AE-8 TUNE — parameter definitions
#include "../Source/ParamDefs.h"   // ae8::ParamDef / Kind
#include <vector>

namespace ae8tune
{
using ae8::ParamDef; using ae8::Float; using ae8::Int; using ae8::Choice; using ae8::Bool;

enum P
{
    KEY, SCALE, SPEED, AMOUNT, HUMANIZE, SHIFT,
    DOUBLE, HARMONY, HARM_INT,
    DIST_DRIVE, DIST_TONE, DIST_MIX,
    DLY_TIME, DLY_FB, DLY_MIX,
    REV_SIZE, REV_MIX,
    MIX, OUTPUT,
    NUM_PARAMS
};

enum Scale { S_CHROMATIC, S_MAJOR, S_MINOR, S_HARM_MINOR, S_PENT_MAJOR, S_PENT_MINOR, S_BLUES };
enum Harm  { H_3RD_UP, H_5TH_UP, H_3RD_5TH, H_OCT_DOWN, H_3RD_DOWN };

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> d =
    {
        { "key",       "Key",          Choice, 0, 11, 9, 0, { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, "" },
        { "scale",     "Scale",        Choice, 0, 6, 2, 0, { "Chromatic", "Major", "Minor", "Harm Minor", "Pent Major", "Pent Minor", "Blues" }, "" },
        { "speed",     "Retune Speed", Float, 0, 400, 20, 60, {}, "ms" },
        { "amount",    "Amount",       Float, 0, 1, 1, 0, {}, "" },
        { "humanize",  "Humanize",     Float, 0, 1, 0.2f, 0, {}, "" },
        { "shift",     "Pitch Shift",  Int,  -12, 12, 0, 0, {}, "st" },

        { "double",    "Double",       Float, 0, 1, 0, 0, {}, "" },
        { "harmony",   "Harmony",      Float, 0, 1, 0, 0, {}, "" },
        { "harmInt",   "Harmony Interval", Choice, 0, 4, 0, 0, { "+3rd", "+5th", "+3rd+5th", "-Oct", "-3rd" }, "" },

        { "distDrive", "Distortion Drive", Float, 0, 1, 0, 0, {}, "" },
        { "distTone",  "Distortion Tone",  Float, 0, 1, 0.6f, 0, {}, "" },
        { "distMix",   "Distortion Mix",   Float, 0, 1, 0, 0, {}, "" },

        { "dlyTime",   "Delay Time",   Choice, 0, 6, 3, 0, { "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2" }, "" },
        { "dlyFb",     "Delay Feedback", Float, 0, 0.9f, 0.3f, 0, {}, "" },
        { "dlyMix",    "Delay Mix",    Float, 0, 1, 0, 0, {}, "" },

        { "revSize",   "Reverb Size",  Float, 0, 1, 0.5f, 0, {}, "" },
        { "revMix",    "Reverb Mix",   Float, 0, 1, 0, 0, {}, "" },

        { "mix",       "Tune Mix",     Float, 0, 1, 1, 0, {}, "" },
        { "output",    "Output",       Float, -24, 12, 0, 0, {}, "dB" },
    };
    return d;
}

inline double delayBeats (int i)
{
    static const double b[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 1.0, 1.5, 2.0 };
    return b[i < 0 ? 0 : (i > 6 ? 6 : i)];
}
} // namespace ae8tune
