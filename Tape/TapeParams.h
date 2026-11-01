#pragma once
// AE-8 TAPE — parameter definitions
#include "../Source/ParamDefs.h"
#include <vector>

namespace ae8tape
{
using ae8::ParamDef; using ae8::Float; using ae8::Int; using ae8::Choice; using ae8::Bool;

enum P
{
    INPUT, SATURATE, BIAS,
    SPEED, STOP,
    WOW, FLUTTER, WEAR,
    HISS, CROSSTALK, NR,
    LOW, HIGH,
    OUTPUT, MIX,
    NUM_PARAMS
};

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> d =
    {
        { "input",     "Input",       Float, -12, 24, 0, 0, {}, "dB" },
        { "saturate",  "Saturation",  Float, 0, 1, 0.35f, 0, {}, "" },
        { "bias",      "Bias",        Float, -1, 1, 0, 0, {}, "" },
        { "speed",     "Speed",       Float, 0, 1, 1, 0, {}, "%" },
        { "stop",      "Tape Stop",   Bool,  0, 1, 0, 0, {}, "" },
        { "wow",       "Wow",         Float, 0, 1, 0.15f, 0, {}, "" },
        { "flutter",   "Flutter",     Float, 0, 1, 0.15f, 0, {}, "" },
        { "wear",      "Wear",        Float, 0, 1, 0.1f, 0, {}, "" },
        { "hiss",      "Hiss",        Float, 0, 1, 0.15f, 0, {}, "" },
        { "crosstalk", "Crosstalk",   Float, 0, 1, 0.1f, 0, {}, "" },
        { "nr",        "Noise Reduction", Bool, 0, 1, 0, 0, {}, "" },
        { "low",       "Low EQ",      Float, -12, 12, 0, 0, {}, "dB" },
        { "high",      "High EQ",     Float, -12, 12, 0, 0, {}, "dB" },
        { "output",    "Output",      Float, -24, 12, 0, 0, {}, "dB" },
        { "mix",       "Mix",         Float, 0, 1, 1, 0, {}, "" },
    };
    return d;
}
} // namespace ae8tape
