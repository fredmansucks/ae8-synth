#pragma once
// AE-8 Analog Poly — factory presets.
// Each preset starts from the INIT values in ParamDefs.h and applies the overrides listed.
// Tags in the names hint at the classic instrument the patch is modelled on:
//   FM = 2-operator FM (DX-style)   ARP = uses the arpeggiator
//   JP = Jupiter-8   JU = Juno-60/106   P5 = Prophet-5   OB = OB-X/OB-Xa
//   MM = Minimoog    CS = CS-80         101 = SH-101     OD = Odyssey   PS = Polysix
#include "ParamDefs.h"
#include <vector>
#include <utility>

namespace ae8
{
struct Preset
{
    const char* name;
    std::vector<std::pair<int, float>> v;
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> p =
    {
        // ------------------------------------------------------------------ PADS
        { "PAD JU Warm Strings Pad", { {OSC1_WAVE,W_SAW},{OSC1_LEVEL,0.9f},{SUB_LEVEL,0.25f},{CUTOFF,2200},{RESO,0.15f},{FLT_ENV,0.1f},
            {F_A,0.8f},{F_D,2},{F_S,0.6f},{F_R,1.5f},{A_A,0.6f},{A_D,1},{A_S,1},{A_R,1.6f},{CHORUS,C_I},{REV_MIX,0.25f},{REV_SIZE,0.7f},{AMP_VEL,0.3f} } },
        { "PAD JU PWM Dream", { {OSC1_WAVE,W_PULSE},{PW,0.5f},{PWM_LFO,0.6f},{LFO_RATE,0.6f},{SUB_LEVEL,0.3f},{CUTOFF,1800},{RESO,0.2f},
            {A_A,0.9f},{A_D,1},{A_S,1},{A_R,2},{CHORUS,C_II},{REV_MIX,0.3f},{REV_SIZE,0.75f} } },
        { "PAD JP Galaxy Sweep", { {OSC2_LEVEL,0.85f},{OSC2_DETUNE,9},{CUTOFF,350},{RESO,0.45f},{FLT_ENV,0.55f},{F_A,3.5f},{F_D,4},{F_S,0.3f},{F_R,3},
            {A_A,1.2f},{A_S,1},{A_R,3},{SPREAD,0.8f},{REV_MIX,0.35f},{REV_SIZE,0.9f},{DLY_MIX,0.15f},{DLY_TIME,0.45f},{DLY_FB,0.45f} } },
        { "PAD OB Big Poly Pad", { {OSC2_LEVEL,1},{OSC2_DETUNE,12},{FLT_TYPE,F_LP12},{CUTOFF,1400},{RESO,0.25f},{FLT_ENV,0.25f},{F_A,0.5f},{F_D,2},{F_S,0.5f},
            {A_A,0.4f},{A_R,1.2f},{DRIFT,0.55f},{SPREAD,0.7f},{REV_MIX,0.2f},{DRIVE,0.35f} } },
        { "PAD P5 Soft Choir", { {OSC1_WAVE,W_PULSE},{PW,0.3f},{OSC2_WAVE,W_TRI},{OSC2_LEVEL,0.7f},{OSC2_OCT,1},{CUTOFF,1100},{RESO,0.1f},{NOISE_LEVEL,0.05f},
            {LFO_RATE,4.5f},{LFO_PITCH,0.06f},{LFO_DELAY,1.2f},{A_A,1.0f},{A_R,1.8f},{REV_MIX,0.35f},{REV_SIZE,0.85f},{CHORUS,C_I} } },
        { "PAD CS Blade Swell", { {OSC2_WAVE,W_PULSE},{OSC2_LEVEL,0.9f},{OSC2_DETUNE,6},{PW,0.35f},{CUTOFF,900},{RESO,0.2f},{FLT_ENV,0.35f},{F_A,2.5f},{F_D,3},{F_S,0.7f},
            {A_A,1.8f},{A_R,2.5f},{LFO_RATE,5.5f},{LFO_PITCH,0.08f},{LFO_DELAY,2},{REV_MIX,0.45f},{REV_SIZE,0.92f},{DRIFT,0.5f},{SPREAD,0.8f} } },
        { "PAD JU Glass Air", { {OSC1_WAVE,W_PULSE},{PW,0.15f},{PWM_LFO,0.25f},{LFO_RATE,0.3f},{HPF,600},{CUTOFF,6000},{RESO,0.35f},{A_A,1.5f},{A_R,2.5f},
            {CHORUS,C_I_II},{REV_MIX,0.4f},{REV_SIZE,0.9f},{DLY_MIX,0.2f},{DLY_TIME,0.5f},{DLY_FB,0.5f} } },
        { "PAD JP Dark Matter", { {OSC1_OCT,-1},{OSC2_LEVEL,0.9f},{OSC2_DETUNE,-8},{SUB_LEVEL,0.4f},{CUTOFF,280},{RESO,0.55f},{FLT_LFO,0.25f},{LFO_RATE,0.15f},
            {LFO_WAVE,L_SINE},{A_A,2.5f},{A_R,3.5f},{REV_MIX,0.4f},{REV_SIZE,0.95f},{DRIVE,0.4f} } },
        { "PAD PS Ensemble Wash", { {OSC2_LEVEL,0.6f},{OSC2_OCT,-1},{CUTOFF,2600},{RESO,0.05f},{A_A,0.7f},{A_R,1.4f},{CHORUS,C_II},{SPREAD,0.6f},{REV_MIX,0.3f} } },
        { "PAD OB Sunrise", { {OSC2_LEVEL,1},{OSC2_DETUNE,10},{OSC2_SEMI,7},{FLT_TYPE,F_LP12},{CUTOFF,500},{RESO,0.3f},{FLT_ENV,0.45f},{F_A,4},{F_D,5},{F_S,0.6f},
            {A_A,2},{A_R,3},{REV_MIX,0.35f},{REV_SIZE,0.85f},{SPREAD,0.7f} } },

        // ----------------------------------------------------------------- BRASS
        { "BRS JP Brass Stack", { {OSC2_LEVEL,0.95f},{OSC2_DETUNE,7},{CUTOFF,600},{RESO,0.15f},{FLT_ENV,0.55f},{F_A,0.08f},{F_D,0.7f},{F_S,0.45f},{F_R,0.5f},
            {A_A,0.04f},{A_D,0.3f},{A_S,0.9f},{A_R,0.4f},{FLT_VEL,0.4f},{DRIFT,0.4f},{REV_MIX,0.15f} } },
        { "BRS OB Jump Brass", { {OSC2_LEVEL,1},{OSC2_DETUNE,9},{FLT_TYPE,F_LP12},{CUTOFF,700},{RESO,0.2f},{FLT_ENV,0.5f},{F_A,0.02f},{F_D,0.5f},{F_S,0.55f},
            {A_A,0.01f},{A_S,1},{A_R,0.25f},{DRIVE,0.4f},{SPREAD,0.6f},{REV_MIX,0.12f} } },
        { "BRS CS Bladerunner Brass", { {OSC2_LEVEL,0.9f},{OSC2_DETUNE,5},{CUTOFF,450},{RESO,0.2f},{FLT_ENV,0.6f},{F_A,0.35f},{F_D,1.5f},{F_S,0.5f},{F_R,1.5f},
            {A_A,0.2f},{A_R,1.6f},{LFO_RATE,5},{LFO_PITCH,0.07f},{LFO_DELAY,0.8f},{REV_MIX,0.4f},{REV_SIZE,0.9f},{DLY_MIX,0.12f},{DLY_TIME,0.6f} } },
        { "BRS P5 Synth Horns", { {OSC1_WAVE,W_PULSE},{PW,0.4f},{OSC2_LEVEL,0.8f},{OSC2_DETUNE,4},{CUTOFF,800},{RESO,0.1f},{FLT_ENV,0.45f},{F_A,0.05f},{F_D,0.4f},{F_S,0.6f},
            {A_A,0.02f},{A_R,0.3f},{FLT_VEL,0.5f},{REV_MIX,0.12f} } },
        { "BRS JU Funky Stabs", { {SUB_LEVEL,0.4f},{CUTOFF,500},{RESO,0.25f},{FLT_ENV,0.5f},{F_D,0.25f},{F_S,0},{F_R,0.2f},{A_D,0.35f},{A_S,0},{A_R,0.2f},
            {CHORUS,C_I},{FLT_VEL,0.5f} } },
        { "BRS JP Fanfare Fifths", { {OSC2_LEVEL,0.8f},{OSC2_SEMI,7},{OSC2_DETUNE,3},{CUTOFF,650},{RESO,0.15f},{FLT_ENV,0.55f},{F_A,0.06f},{F_D,0.8f},{F_S,0.5f},
            {A_A,0.03f},{A_R,0.5f},{REV_MIX,0.2f} } },

        // --------------------------------------------------------------- STRINGS
        { "STR JU Solina-ish Ensemble", { {OSC1_WAVE,W_SAW},{HPF,180},{CUTOFF,4500},{RESO,0},{A_A,0.35f},{A_R,1.2f},{CHORUS,C_I_II},{REV_MIX,0.25f},{AMP_VEL,0.2f} } },
        { "STR OB Lush Strings", { {OSC2_LEVEL,1},{OSC2_DETUNE,14},{FLT_TYPE,F_LP12},{CUTOFF,3000},{RESO,0.1f},{A_A,0.5f},{A_R,1.3f},{LFO_RATE,5},{LFO_PITCH,0.04f},
            {LFO_DELAY,0.7f},{SPREAD,0.8f},{REV_MIX,0.3f} } },
        { "STR JP Slow Strings", { {OSC2_LEVEL,0.8f},{OSC2_OCT,1},{OSC2_DETUNE,5},{CUTOFF,2000},{RESO,0.05f},{A_A,1.2f},{A_R,2},{CHORUS,C_I},{REV_MIX,0.35f},{REV_SIZE,0.8f} } },
        { "STR P5 Chamber", { {OSC1_WAVE,W_PULSE},{PW,0.45f},{PWM_LFO,0.15f},{LFO_RATE,0.8f},{OSC2_LEVEL,0.7f},{OSC2_DETUNE,8},{CUTOFF,2500},{A_A,0.25f},{A_R,0.9f},{REV_MIX,0.25f} } },
        { "STR JU Octave Strings", { {OSC1_WAVE,W_SAW},{SUB_LEVEL,0.5f},{HPF,90},{CUTOFF,3500},{A_A,0.3f},{A_R,1},{CHORUS,C_II},{REV_MIX,0.2f} } },

        // ------------------------------------------------------------ KEYS / POLY
        { "KEY JU Classic Poly", { {OSC1_WAVE,W_PULSE},{PW,0.4f},{SUB_LEVEL,0.3f},{CUTOFF,1200},{RESO,0.2f},{FLT_ENV,0.35f},{F_D,0.6f},{F_S,0.3f},{A_D,1.2f},{A_S,0.6f},
            {A_R,0.4f},{CHORUS,C_I},{FLT_VEL,0.4f} } },
        { "KEY JP Bell Sync", { {OSC2_LEVEL,1},{OSC1_LEVEL,0},{OSC2_SYNC,1},{OSC2_OCT,1},{OSC2_SEMI,5},{ENV_PITCH,0.25f},{CUTOFF,4000},{F_D,1.5f},{F_S,0},
            {A_D,2},{A_S,0},{A_R,1.5f},{REV_MIX,0.3f},{DLY_MIX,0.15f} } },
        { "KEY P5 Poly Keys", { {OSC1_WAVE,W_SAW},{OSC2_WAVE,W_PULSE},{OSC2_LEVEL,0.6f},{PW,0.3f},{CUTOFF,900},{RESO,0.15f},{FLT_ENV,0.4f},{F_D,0.8f},{F_S,0.2f},
            {A_D,1.5f},{A_S,0.4f},{A_R,0.5f},{FLT_VEL,0.5f},{REV_MIX,0.15f} } },
        { "KEY OB Pluck Poly", { {OSC2_LEVEL,1},{OSC2_DETUNE,8},{FLT_TYPE,F_LP12},{CUTOFF,300},{RESO,0.3f},{FLT_ENV,0.6f},{F_D,0.3f},{F_S,0},{A_D,0.8f},{A_S,0},{A_R,0.5f},
            {DLY_MIX,0.2f},{DLY_TIME,0.33f},{REV_MIX,0.15f} } },
        { "KEY JU Electric Piano", { {OSC1_WAVE,W_SQUARE},{OSC1_LEVEL,0.7f},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0.8f},{OSC2_OCT,1},{CUTOFF,1500},{FLT_ENV,0.35f},{F_D,0.5f},{F_S,0},
            {A_D,2.5f},{A_S,0},{A_R,0.4f},{CHORUS,C_I},{AMP_VEL,0.7f},{FLT_VEL,0.6f} } },
        { "KEY CS Velvet Keys", { {OSC2_WAVE,W_PULSE},{OSC2_LEVEL,0.6f},{OSC2_DETUNE,4},{CUTOFF,700},{RESO,0.15f},{FLT_ENV,0.3f},{FLT_VEL,0.8f},{A_D,3},{A_S,0.3f},{A_R,0.9f},
            {LFO_RATE,5},{LFO_PITCH,0.05f},{LFO_DELAY,0.6f},{REV_MIX,0.3f} } },
        { "KEY PS Organ Drawbar", { {OSC1_WAVE,W_SQUARE},{OSC2_WAVE,W_SQUARE},{OSC2_OCT,1},{OSC2_LEVEL,0.6f},{SUB_LEVEL,0.6f},{CUTOFF,5000},{RESO,0},{A_A,0.003f},{A_R,0.06f},
            {AMP_VEL,0},{CHORUS,C_I_II},{FLT_VEL,0} } },
        { "KEY JP Clav Funk", { {OSC1_WAVE,W_PULSE},{PW,0.2f},{HPF,250},{FLT_TYPE,F_BP},{CUTOFF,1800},{RESO,0.4f},{FLT_ENV,0.35f},{F_D,0.15f},{F_S,0},{A_D,0.4f},{A_S,0},{A_R,0.1f},
            {AMP_VEL,0.8f} } },

        // ----------------------------------------------------------------- LEADS
        { "LD MM Classic Lead", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC2_LEVEL,0.9f},{OSC2_DETUNE,6},{SUB_LEVEL,0.3f},{CUTOFF,900},{RESO,0.35f},{FLT_ENV,0.45f},{F_D,0.6f},{F_S,0.4f},
            {GLIDE,0.08f},{DRIVE,0.5f},{LFO_PITCH,0},{MW_VIB,0.8f},{DLY_MIX,0.15f} } },
        { "LD JP Sync Scream", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_LEVEL,0},{OSC2_LEVEL,1},{OSC2_SYNC,1},{ENV_PITCH,0.45f},{F_D,0.8f},{F_S,0.2f},{CUTOFF,3500},{RESO,0.2f},
            {GLIDE,0.05f},{DLY_MIX,0.25f},{DLY_FB,0.4f},{REV_MIX,0.15f} } },
        { "LD 101 Square Lead", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_WAVE,W_SQUARE},{SUB_LEVEL,0.2f},{CUTOFF,1500},{RESO,0.3f},{FLT_ENV,0.3f},{F_D,0.3f},{F_S,0.4f},{GLIDE,0.06f},
            {LFO_RATE,5.5f},{LFO_PITCH,0.05f},{LFO_DELAY,0.5f},{DLY_MIX,0.2f} } },
        { "LD OD Whistle Lead", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_WAVE,W_TRI},{OSC2_WAVE,W_SINE},{OSC2_OCT,1},{OSC2_LEVEL,0.5f},{CUTOFF,5000},{RESO,0.6f},{GLIDE,0.12f},
            {LFO_RATE,5.8f},{LFO_PITCH,0.07f},{LFO_DELAY,0.4f},{A_A,0.05f},{REV_MIX,0.3f},{DLY_MIX,0.25f} } },
        { "LD JP Unison Monster", { {MODE,M_UNISON},{UNI_DETUNE,0.45f},{OSC2_LEVEL,0.9f},{OSC2_DETUNE,5},{CUTOFF,1200},{RESO,0.25f},{FLT_ENV,0.4f},{F_D,0.7f},{F_S,0.5f},
            {GLIDE,0.07f},{SPREAD,0.9f},{DRIVE,0.4f},{VOLUME,-9},{REV_MIX,0.15f} } },
        { "LD P5 Poly Mod Lead", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC2_WAVE,W_TRI},{OSC2_LEVEL,0.6f},{OSC2_SEMI,7},{XMOD,0.35f},{CUTOFF,1800},{RESO,0.3f},{FLT_ENV,0.35f},{F_D,0.5f},
            {F_S,0.3f},{GLIDE,0.04f},{DLY_MIX,0.2f} } },
        { "LD MM Fat Portamento", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{OSC2_LEVEL,1},{OSC2_DETUNE,-7},{SUB_LEVEL,0.4f},{CUTOFF,700},{RESO,0.4f},{FLT_ENV,0.5f},{F_D,0.9f},
            {F_S,0.5f},{GLIDE,0.25f},{DRIVE,0.7f} } },
        { "LD JU Chirpy Lead", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_WAVE,W_PULSE},{PW,0.25f},{CUTOFF,1000},{RESO,0.55f},{FLT_ENV,0.5f},{F_D,0.2f},{F_S,0.3f},{CHORUS,C_I},
            {DLY_MIX,0.3f},{DLY_TIME,0.3f},{DLY_FB,0.45f} } },

        // ------------------------------------------------------------------ BASS
        { "BS MM Moog Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{OSC2_OCT,-1},{OSC2_LEVEL,1},{OSC2_DETUNE,5},{SUB_LEVEL,0.3f},{CUTOFF,250},{RESO,0.3f},{FLT_ENV,0.5f},
            {F_D,0.35f},{F_S,0.2f},{A_D,0.6f},{A_S,0.8f},{A_R,0.15f},{DRIVE,0.6f},{REV_MIX,0},{SPREAD,0} } },
        { "BS 101 Acid Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{CUTOFF,300},{RESO,0.75f},{FLT_ENV,0.6f},{F_D,0.22f},{F_S,0},{A_S,1},{A_R,0.08f},{GLIDE,0.06f},{DRIVE,0.6f},
            {REV_MIX,0},{SPREAD,0},{FLT_VEL,0.6f} } },
        { "BS JU Sub Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{OSC1_WAVE,W_PULSE},{PW,0.35f},{SUB_LEVEL,1},{CUTOFF,400},{RESO,0.15f},{FLT_ENV,0.2f},{F_D,0.3f},{A_R,0.1f},
            {REV_MIX,0},{SPREAD,0} } },
        { "BS OB Poly Bass", { {OSC1_OCT,-1},{OSC2_OCT,-1},{OSC2_LEVEL,1},{OSC2_DETUNE,9},{FLT_TYPE,F_LP12},{CUTOFF,350},{RESO,0.25f},{FLT_ENV,0.5f},{F_D,0.3f},{F_S,0.15f},
            {A_R,0.15f},{DRIVE,0.5f},{REV_MIX,0},{SPREAD,0.3f} } },
        { "BS JP Sync Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{OSC2_OCT,-1},{OSC2_LEVEL,1},{OSC1_LEVEL,0.5f},{OSC2_SYNC,1},{ENV_PITCH,0.3f},{F_D,0.25f},{F_S,0},{CUTOFF,800},
            {RESO,0.2f},{FLT_ENV,0.3f},{A_R,0.1f},{REV_MIX,0},{SPREAD,0} } },
        { "BS P5 Pluck Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{OSC2_WAVE,W_PULSE},{OSC2_LEVEL,0.7f},{OSC2_OCT,-1},{PW,0.3f},{CUTOFF,200},{RESO,0.35f},{FLT_ENV,0.55f},
            {F_D,0.18f},{F_S,0},{A_D,0.5f},{A_S,0.5f},{A_R,0.1f},{REV_MIX,0},{SPREAD,0} } },
        { "BS 101 Rubber Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-1},{OSC1_WAVE,W_SQUARE},{SUB_LEVEL,0.6f},{CUTOFF,220},{RESO,0.45f},{FLT_ENV,0.45f},{F_D,0.3f},{F_S,0.1f},
            {GLIDE,0.04f},{A_R,0.1f},{REV_MIX,0},{SPREAD,0} } },
        { "BS MM Growl Bass", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_OCT,-2},{OSC2_OCT,-1},{OSC2_LEVEL,0.8f},{OSC2_DETUNE,-6},{CUTOFF,300},{RESO,0.4f},{FLT_ENV,0.3f},{FLT_LFO,0.2f},
            {LFO_RATE,6},{F_D,0.5f},{F_S,0.4f},{DRIVE,0.9f},{A_R,0.15f},{REV_MIX,0},{SPREAD,0} } },

        // ------------------------------------------------------------- FX / SEQ
        { "SEQ JP Arp Pluck", { {CUTOFF,500},{RESO,0.4f},{FLT_ENV,0.55f},{F_D,0.18f},{F_S,0},{A_D,0.3f},{A_S,0},{A_R,0.2f},{DLY_MIX,0.3f},{DLY_TIME,0.375f},{DLY_FB,0.5f},
            {REV_MIX,0.2f},{FLT_VEL,0.5f} } },
        { "SEQ JU Bubble Arp", { {OSC1_WAVE,W_PULSE},{PW,0.3f},{CUTOFF,700},{RESO,0.6f},{FLT_ENV,0.5f},{F_D,0.12f},{F_S,0},{A_D,0.25f},{A_S,0},{A_R,0.15f},{CHORUS,C_II},
            {DLY_MIX,0.25f},{DLY_TIME,0.25f},{DLY_FB,0.4f} } },
        { "FX OD Laser Zap", { {MODE,M_MONO},{VOLUME,-0.5f},{OSC1_WAVE,W_SQUARE},{OSC2_LEVEL,0},{CUTOFF,3000},{RESO,0.7f},{FLT_ENV,0.4f},{F_D,0.3f},{F_S,0},{A_D,0.4f},{A_S,0},
            {LFO_WAVE,L_SAW},{LFO_RATE,12},{LFO_PITCH,0.5f},{DLY_MIX,0.3f},{DLY_FB,0.5f} } },
        { "FX JP Sample & Hold", { {OSC2_LEVEL,0.7f},{OSC2_SEMI,7},{CUTOFF,900},{RESO,0.7f},{FLT_LFO,0.6f},{LFO_WAVE,L_SH},{LFO_RATE,8},{A_A,0.01f},{A_R,0.6f},
            {DLY_MIX,0.3f},{DLY_TIME,0.5f},{DLY_FB,0.5f},{REV_MIX,0.25f} } },
        { "FX CS Wind & Noise", { {OSC1_LEVEL,0},{NOISE_LEVEL,1},{FLT_TYPE,F_BP},{CUTOFF,900},{RESO,0.6f},{FLT_LFO,0.5f},{LFO_WAVE,L_SINE},{LFO_RATE,0.12f},{A_A,2},
            {A_R,3},{REV_MIX,0.5f},{REV_SIZE,0.95f},{SPREAD,0.8f},{FLT_KEY,1} } },

        // --------------------------------------------------- FM (DX-style, 2-op)
        // Osc 1 = carrier, Osc 2 = modulator. The filter envelope is the modulator envelope (FM Env).
        { "FM Tine E.Piano", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,2},{FM_AMT,0.55f},{FM_ENV,0.9f},{FM_VEL,0.75f},{F_A,0.001f},{F_D,0.9f},{F_S,0.12f},{F_R,0.5f},
            {A_A,0.002f},{A_D,3.5f},{A_S,0},{A_R,0.6f},{AMP_VEL,0.7f},{CHORUS,C_I},{REV_MIX,0.12f} } },
        { "FM Bell Piano", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,15},{FM_AMT,0.38f},{FM_ENV,1},{FM_VEL,0.8f},{F_A,0.001f},{F_D,0.35f},{F_S,0},{F_R,0.3f},
            {A_A,0.002f},{A_D,2.8f},{A_S,0},{A_R,0.6f},{AMP_VEL,0.7f},{CHORUS,C_II},{REV_MIX,0.15f} } },
        { "FM Solid Bass", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{MODE,M_MONO},{VOLUME,-1},{OSC1_OCT,-1},{FM_RATIO,2},{FM_AMT,0.7f},{FM_ENV,0.85f},{F_A,0.001f},{F_D,0.25f},{F_S,0.2f},{F_R,0.1f},
            {A_A,0.002f},{A_D,0.8f},{A_S,0.6f},{A_R,0.1f},{REV_MIX,0},{SPREAD,0},{DRIVE,0.25f} } },
        { "FM Slap Bass", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{MODE,M_MONO},{VOLUME,-1},{OSC1_OCT,-1},{FM_RATIO,3},{FM_AMT,0.8f},{FM_ENV,1},{FM_VEL,0.6f},{F_A,0.001f},{F_D,0.14f},{F_S,0.1f},{F_R,0.1f},
            {A_A,0.001f},{A_D,0.5f},{A_S,0.35f},{A_R,0.08f},{REV_MIX,0},{SPREAD,0} } },
        { "FM Glass Bells", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{OSC2_OCT,1},{OSC2_SEMI,5},{FM_AMT,0.55f},{FM_ENV,0.8f},{F_A,0.001f},{F_D,2.5f},{F_S,0},{F_R,2},
            {A_A,0.002f},{A_D,4.5f},{A_S,0},{A_R,2.2f},{REV_MIX,0.35f},{REV_SIZE,0.8f},{DLY_MIX,0.12f} } },
        { "FM Tubular Bells", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{OSC2_OCT,1},{OSC2_SEMI,2},{OSC2_DETUNE,14},{FM_AMT,0.65f},{FM_ENV,0.7f},{F_A,0.001f},{F_D,3},{F_S,0},{F_R,3},
            {A_A,0.002f},{A_D,6},{A_S,0},{A_R,3},{REV_MIX,0.3f},{REV_SIZE,0.85f} } },
        { "FM Marimba", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,5},{FM_AMT,0.5f},{FM_ENV,1},{FM_VEL,0.6f},{F_A,0.001f},{F_D,0.07f},{F_S,0},{F_R,0.05f},
            {A_A,0.001f},{A_D,0.55f},{A_S,0},{A_R,0.35f},{AMP_VEL,0.8f},{REV_MIX,0.2f} } },
        { "FM Brass Section", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,2},{FM_AMT,0.6f},{FM_ENV,0.85f},{FM_VEL,0.5f},{F_A,0.09f},{F_D,0.6f},{F_S,0.55f},{F_R,0.3f},
            {A_A,0.05f},{A_D,0.4f},{A_S,0.9f},{A_R,0.3f},{LFO_RATE,5.5f},{LFO_PITCH,0.05f},{LFO_DELAY,0.7f},{OSC2_DETUNE,4},{REV_MIX,0.15f} } },
        { "FM Digital Clav", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,4},{FM_AMT,0.6f},{FM_ENV,0.9f},{FM_VEL,0.7f},{F_A,0.001f},{F_D,0.2f},{F_S,0.1f},{F_R,0.1f},
            {A_A,0.001f},{A_D,0.9f},{A_S,0},{A_R,0.12f},{HPF,220},{AMP_VEL,0.8f} } },
        { "FM Reed Organ", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,3},{FM_AMT,0.35f},{FM_ENV,0},{F_S,1},{A_A,0.01f},{A_S,1},{A_R,0.1f},{AMP_VEL,0},
            {LFO_RATE,6},{LFO_PITCH,0.03f},{CHORUS,C_I_II} } },
        { "FM Crystal Pad", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{OSC2_OCT,2},{OSC2_DETUNE,7},{FM_AMT,0.35f},{FM_ENV,0.3f},{F_A,2},{F_D,4},{F_S,0.5f},{F_R,3},
            {A_A,1.2f},{A_D,2},{A_S,1},{A_R,2.5f},{CHORUS,C_II},{REV_MIX,0.4f},{REV_SIZE,0.9f},{DLY_MIX,0.15f},{DLY_TIME,0.5f},{SPREAD,0.7f} } },
        { "FM Log Drum", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,2},{FM_AMT,0.6f},{FM_ENV,1},{ENV_PITCH,0.05f},{F_A,0.001f},{F_D,0.06f},{F_S,0},{F_R,0.05f},
            {A_A,0.001f},{A_D,0.35f},{A_S,0},{A_R,0.25f},{AMP_VEL,0.8f},{REV_MIX,0.15f} } },
        { "FM Koto Pluck", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,4},{OSC2_DETUNE,3},{FM_AMT,0.5f},{FM_ENV,1},{F_A,0.001f},{F_D,0.1f},{F_S,0},{F_R,0.1f},
            {A_A,0.001f},{A_D,1.2f},{A_S,0},{A_R,0.4f},{LFO_RATE,5},{LFO_PITCH,0.03f},{LFO_DELAY,0.5f},{REV_MIX,0.25f},{DLY_MIX,0.12f} } },

        // ------------------------------------------------------------ ARP
        { "ARP FM Bell Sequence", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{OSC2_OCT,1},{OSC2_SEMI,5},{FM_AMT,0.5f},{FM_ENV,0.9f},{F_A,0.001f},{F_D,0.4f},{F_S,0},{F_R,0.3f},
            {A_A,0.001f},{A_D,0.9f},{A_S,0},{A_R,0.5f},{ARP_ON,1},{VOLUME,-1},{ARP_MODE,A_UPDOWN},{ARP_RATE,3},{ARP_OCT,2},{ARP_GATE,0.5f},
            {DLY_MIX,0.3f},{DLY_TIME,0.375f},{DLY_FB,0.45f},{REV_MIX,0.25f} } },
        { "ARP JU Pulse Arp", { {OSC1_WAVE,W_PULSE},{PW,0.35f},{SUB_LEVEL,0.3f},{CUTOFF,700},{RESO,0.45f},{FLT_ENV,0.5f},{F_D,0.15f},{F_S,0},{A_D,0.3f},{A_S,0.2f},{A_R,0.15f},
            {CHORUS,C_I},{ARP_ON,1},{VOLUME,-1},{ARP_MODE,A_UP},{ARP_RATE,3},{ARP_OCT,2},{ARP_GATE,0.55f},{DLY_MIX,0.25f},{DLY_TIME,0.5625f},{DLY_FB,0.4f} } },
        { "ARP OB Octave Bass", { {OSC1_OCT,-1},{OSC2_OCT,-1},{OSC2_LEVEL,1},{OSC2_DETUNE,8},{FLT_TYPE,F_LP12},{CUTOFF,400},{RESO,0.3f},{FLT_ENV,0.5f},{F_D,0.2f},{F_S,0.1f},
            {A_R,0.1f},{DRIVE,0.5f},{REV_MIX,0},{SPREAD,0.2f},{ARP_ON,1},{VOLUME,-1},{ARP_MODE,A_PLAYED},{ARP_RATE,3},{ARP_OCT,2},{ARP_GATE,0.7f} } },
        { "ARP FM Random Data", { {OSC1_WAVE,W_SINE},{OSC2_WAVE,W_SINE},{OSC2_LEVEL,0},{CUTOFF,20000},{RESO,0},{FLT_KEY,0},{FLT_VEL,0},{DRIVE,0},{DRIFT,0.08f},{HPF,20},{FM_RATIO,7},{FM_AMT,0.45f},{FM_ENV,1},{F_A,0.001f},{F_D,0.12f},{F_S,0},{A_D,0.25f},{A_S,0},{A_R,0.2f},
            {ARP_ON,1},{VOLUME,-1},{ARP_MODE,A_RANDOM},{ARP_RATE,5},{ARP_OCT,3},{ARP_GATE,0.4f},{DLY_MIX,0.3f},{DLY_TIME,0.1875f},{DLY_FB,0.5f},{SPREAD,0.8f} } },
    };
    return p;
}
} // namespace ae8
