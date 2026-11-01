#pragma once
// AE-8 Analog Poly — parameter definitions (shared by the DSP engine, presets and the plugin)
#include <vector>

namespace ae8
{
enum P
{
    OSC1_WAVE, OSC1_OCT, OSC1_LEVEL,
    OSC2_WAVE, OSC2_OCT, OSC2_SEMI, OSC2_DETUNE, OSC2_LEVEL, OSC2_SYNC,
    XMOD, PW, PWM_LFO, ENV_PITCH,
    SUB_LEVEL, NOISE_LEVEL, HPF,
    FLT_TYPE, CUTOFF, RESO, FLT_ENV, FLT_KEY, FLT_LFO, FLT_VEL, DRIVE,
    F_A, F_D, F_S, F_R,
    A_A, A_D, A_S, A_R, AMP_VEL,
    LFO_WAVE, LFO_RATE, LFO_DELAY, LFO_PITCH, MW_VIB,
    MODE, UNI_DETUNE, GLIDE, BEND, DRIFT, SPREAD,
    CHORUS, DLY_TIME, DLY_FB, DLY_MIX, REV_SIZE, REV_MIX, VOLUME,
    NUM_PARAMS
};

enum Kind { Float, Int, Choice, Bool };

struct ParamDef
{
    const char* id;
    const char* name;
    Kind kind;
    float min, max, def;
    float skewCentre;               // > 0 : value at the middle of the knob
    std::vector<const char*> choices;
    const char* unit;
};

// choice indices
enum Wave    { W_SAW, W_PULSE, W_TRI, W_SQUARE, W_SINE };
enum FType   { F_LP24, F_LP12, F_BP, F_HP };
enum LWave   { L_TRI, L_SINE, L_SAW, L_SQUARE, L_SH };
enum VMode   { M_POLY, M_MONO, M_UNISON };
enum ChMode  { C_OFF, C_I, C_II, C_I_II };

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<const char*> waves { "Saw", "Pulse", "Triangle", "Square", "Sine" };
    static const std::vector<ParamDef> d =
    {
        { "osc1Wave",   "Osc1 Wave",   Choice, 0, 4, 0, 0, waves, "" },
        { "osc1Oct",    "Osc1 Octave", Int,   -2, 2, 0, 0, {}, "" },
        { "osc1Level",  "Osc1 Level",  Float,  0, 1, 1, 0, {}, "" },

        { "osc2Wave",   "Osc2 Wave",   Choice, 0, 4, 0, 0, waves, "" },
        { "osc2Oct",    "Osc2 Octave", Int,   -2, 2, 0, 0, {}, "" },
        { "osc2Semi",   "Osc2 Semi",   Int,  -12, 12, 0, 0, {}, "st" },
        { "osc2Detune", "Osc2 Detune", Float, -50, 50, 0, 0, {}, "ct" },
        { "osc2Level",  "Osc2 Level",  Float,  0, 1, 0, 0, {}, "" },
        { "osc2Sync",   "Osc2 Sync",   Bool,   0, 1, 0, 0, {}, "" },

        { "xmod",       "Cross Mod",   Float,  0, 1, 0, 0, {}, "" },
        { "pw",         "Pulse Width", Float,  0.05f, 0.95f, 0.5f, 0, {}, "" },
        { "pwmLfo",     "PWM (LFO)",   Float,  0, 1, 0, 0, {}, "" },
        { "envPitch",   "Env>Osc2 Pitch", Float, -1, 1, 0, 0, {}, "" },

        { "subLevel",   "Sub Osc",     Float,  0, 1, 0, 0, {}, "" },
        { "noiseLevel", "Noise",       Float,  0, 1, 0, 0, {}, "" },
        { "hpf",        "HPF",         Float, 20, 2000, 20, 150, {}, "Hz" },

        { "fltType",    "Filter Type", Choice, 0, 3, 0, 0, { "LP 24", "LP 12", "BP 12", "HP 12" }, "" },
        { "cutoff",     "Cutoff",      Float, 20, 20000, 8000, 800, {}, "Hz" },
        { "reso",       "Resonance",   Float,  0, 1, 0.1f, 0, {}, "" },
        { "fltEnv",     "Filter Env",  Float, -1, 1, 0, 0, {}, "" },
        { "fltKey",     "Key Track",   Float,  0, 1, 0.5f, 0, {}, "" },
        { "fltLfo",     "Filter LFO",  Float,  0, 1, 0, 0, {}, "" },
        { "fltVel",     "Filter Vel",  Float,  0, 1, 0.2f, 0, {}, "" },
        { "drive",      "Drive",       Float,  0, 1, 0.2f, 0, {}, "" },

        { "fA",         "F.Env Attack",  Float, 0.001f, 10, 0.005f, 0.4f, {}, "s" },
        { "fD",         "F.Env Decay",   Float, 0.001f, 10, 0.5f,   0.6f, {}, "s" },
        { "fS",         "F.Env Sustain", Float, 0, 1, 0.5f, 0, {}, "" },
        { "fR",         "F.Env Release", Float, 0.001f, 10, 0.4f,   0.6f, {}, "s" },

        { "aA",         "Amp Attack",  Float, 0.001f, 10, 0.005f, 0.4f, {}, "s" },
        { "aD",         "Amp Decay",   Float, 0.001f, 10, 0.5f,   0.6f, {}, "s" },
        { "aS",         "Amp Sustain", Float, 0, 1, 1, 0, {}, "" },
        { "aR",         "Amp Release", Float, 0.001f, 10, 0.3f,   0.6f, {}, "s" },
        { "ampVel",     "Amp Vel",     Float, 0, 1, 0.5f, 0, {}, "" },

        { "lfoWave",    "LFO Wave",    Choice, 0, 4, 0, 0, { "Triangle", "Sine", "Saw", "Square", "S&H" }, "" },
        { "lfoRate",    "LFO Rate",    Float, 0.05f, 30, 5, 3, {}, "Hz" },
        { "lfoDelay",   "LFO Delay",   Float, 0, 5, 0, 0.8f, {}, "s" },
        { "lfoPitch",   "LFO>Pitch",   Float, 0, 1, 0, 0.25f, {}, "" },
        { "mwVib",      "ModWhl Vib",  Float, 0, 1, 0.5f, 0, {}, "" },

        { "mode",       "Voice Mode",  Choice, 0, 2, 0, 0, { "Poly", "Mono", "Unison" }, "" },
        { "uniDetune",  "Uni Detune",  Float, 0, 1, 0.3f, 0, {}, "" },
        { "glide",      "Glide",       Float, 0, 2, 0, 0.25f, {}, "s" },
        { "bend",       "Bend Range",  Int,   0, 12, 2, 0, {}, "st" },
        { "drift",      "Analog Drift",Float, 0, 1, 0.3f, 0, {}, "" },
        { "spread",     "Stereo Spread", Float, 0, 1, 0.4f, 0, {}, "" },

        { "chorus",     "Chorus",      Choice, 0, 3, 0, 0, { "Off", "I", "II", "I+II" }, "" },
        { "dlyTime",    "Delay Time",  Float, 0.01f, 1.5f, 0.375f, 0.35f, {}, "s" },
        { "dlyFb",      "Delay Fdbk",  Float, 0, 0.95f, 0.3f, 0, {}, "" },
        { "dlyMix",     "Delay Mix",   Float, 0, 1, 0, 0, {}, "" },
        { "revSize",    "Reverb Size", Float, 0, 1, 0.6f, 0, {}, "" },
        { "revMix",     "Reverb Mix",  Float, 0, 1, 0.1f, 0, {}, "" },
        { "volume",     "Volume",      Float, -36, 6, -6, 0, {}, "dB" },
    };
    return d;
}
} // namespace ae8
