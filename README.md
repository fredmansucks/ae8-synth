# AE-8 Analog Poly

A 12-voice virtual-analog polysynth VST3 for Ableton Live, built around the sound of the classic 70s/80s polys: Roland Jupiter-8 and Juno-60/106, Sequential Prophet-5, Oberheim OB-X, Minimoog, Yamaha CS-80, SH-101 and friends. It ships with 50 factory presets.

## Install in Ableton (Windows)

1. Copy the `AE-8 Analog Poly.vst3` folder into `C:\Program Files\Common Files\VST3\`.
2. In Ableton go to **Options → Preferences → Plug-Ins**, turn on **Use VST3 Plug-In System Folders** and click **Rescan**.
3. It shows up under **Plug-Ins → VST3 → AE Audio → AE-8 Analog Poly**. Drag it onto a MIDI track.
4. Pick presets from the **PROGRAM** menu at the top of the plugin, or with the `<` `>` buttons.

`AE-8 Analog Poly.exe` is a standalone version you can play without Ableton (Options → audio/MIDI settings).

## What's inside

| Section | Controls |
|---|---|
| **Oscillator 1** | Saw / Pulse / Triangle / Square / Sine, octave ±2, level |
| **Oscillator 2** | Same waves, octave, semitone ±12, fine detune ±50 cents, level, **hard sync** to Osc 1 |
| **Modulation** | Cross-mod (Osc 2 → Osc 1 FM, Jupiter style), pulse width, PWM from LFO, filter env → Osc 2 pitch (sync sweeps) |
| **Mixer** | Juno-style sub oscillator (square, 1 oct down), white noise, high-pass filter |
| **Filter** | LP 24 dB ladder (Moog/Jupiter/Juno style, self-oscillates at max reso), LP/BP/HP 12 dB state-variable (Oberheim SEM style); cutoff, resonance, bipolar env amount, key tracking, LFO amount, velocity, drive |
| **Filter envelope** | ADSR with analog-style curves |
| **Amp envelope** | ADSR + velocity sensitivity |
| **LFO** | Triangle / Sine / Saw / Square / Sample & Hold, rate 0.05–30 Hz, delay (fade-in per note), vibrato depth, mod-wheel vibrato |
| **Voice** | Poly (12 voices) / Mono (legato, last-note priority) / Unison (6 stacked voices), unison detune, glide, pitch-bend range, analog drift, stereo spread |
| **Effects** | Juno-style BBD chorus (I, II, I+II), ping-pong tape-ish delay, reverb, master volume |

Responds to velocity, pitch bend, mod wheel (CC1) and sustain pedal (CC64). Every knob can be automated and MIDI-mapped in Ableton. Double-click a knob to reset it.

## Factory presets

Name tags: JP = Jupiter-8, JU = Juno, P5 = Prophet-5, OB = Oberheim, MM = Minimoog, CS = CS-80, 101 = SH-101, OD = Odyssey, PS = Polysix. These are tributes to those instruments' sounds and are not affiliated with their makers.

01. PAD JU Warm Strings Pad
02. PAD JU PWM Dream
03. PAD JP Galaxy Sweep
04. PAD OB Big Poly Pad
05. PAD P5 Soft Choir
06. PAD CS Blade Swell
07. PAD JU Glass Air
08. PAD JP Dark Matter
09. PAD PS Ensemble Wash
10. PAD OB Sunrise
11. BRS JP Brass Stack
12. BRS OB Jump Brass
13. BRS CS Bladerunner Brass
14. BRS P5 Synth Horns
15. BRS JU Funky Stabs
16. BRS JP Fanfare Fifths
17. STR JU Solina-ish Ensemble
18. STR OB Lush Strings
19. STR JP Slow Strings
20. STR P5 Chamber
21. STR JU Octave Strings
22. KEY JU Classic Poly
23. KEY JP Bell Sync
24. KEY P5 Poly Keys
25. KEY OB Pluck Poly
26. KEY JU Electric Piano
27. KEY CS Velvet Keys
28. KEY PS Organ Drawbar
29. KEY JP Clav Funk
30. LD MM Classic Lead
31. LD JP Sync Scream
32. LD 101 Square Lead
33. LD OD Whistle Lead
34. LD JP Unison Monster
35. LD P5 Poly Mod Lead
36. LD MM Fat Portamento
37. LD JU Chirpy Lead
38. BS MM Moog Bass
39. BS 101 Acid Bass
40. BS JU Sub Bass
41. BS OB Poly Bass
42. BS JP Sync Bass
43. BS P5 Pluck Bass
44. BS 101 Rubber Bass
45. BS MM Growl Bass
46. SEQ JP Arp Pluck
47. SEQ JU Bubble Arp
48. FX OD Laser Zap
49. FX JP Sample & Hold
50. FX CS Wind & Noise

## Building it yourself

The repo includes a GitHub Actions workflow (`.github/workflows/build-windows.yml`). Push the folder to a GitHub repo and the **Actions** tab builds the VST3 + standalone and offers them as a download.

To build locally on Windows instead, install **Visual Studio 2022 Community** (with "Desktop development with C++") and **Git**, then in this folder run:

```
git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build -A x64
cmake --build build --config Release
```

The plugin ends up in `build\AE8_artefacts\Release\VST3\`.

## Code layout

- `Source/SynthEngine.h`: all of the DSP (oscillators, filters, envelopes, voices, effects). It is plain C++ with no framework.
- `Source/ParamDefs.h`: the parameter list (ranges and defaults).
- `Source/Presets.h`: the 50 presets, each one written as changes from the INIT patch. Add your own here.
- `Source/PluginProcessor.*` and `Source/PluginEditor.*`: the JUCE plugin wrapper and the interface.
- `tests/render_presets.cpp`: renders every preset to a WAV file offline (`g++ -std=c++17 -O2 tests/render_presets.cpp`).

Built with JUCE 8 (AGPLv3 / commercial license). Fine for personal use. If you want to sell it, check JUCE's licensing first.
