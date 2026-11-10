# AE-8 MEMORY — M-01 (development preview)

**Status:** initial functional effect; Windows VST3 + standalone are built by GitHub Actions.
Do not describe this as an accurate emulation of any historical sampler. The current
models are AE-8's own DSP configurations inspired by early digital sampling workflows.

## What is implemented

- Real sample-rate reduction by sample-and-hold (3–48 kHz)
- Signed quantization (4–16 bit)
- Post-conversion bandwidth limiting
- Soft saturation, parallel dry/wet blend and output gain
- Five factory starting programs
- APVTS parameter automation and project save/restore
- AE-8 shared brand system: logo, Archivo and IBM Plex Mono, modular panels,
  flux bar, machined rotary controls, resizable UI

## Using it in Ableton (Windows)

1. Download the successful **AE-8-Plugins-Windows** GitHub Actions artifact.
2. Extract it. Copy `AE-8 MEMORY.vst3` into
   `C:\Program Files\Common Files\VST3\`.
3. In Ableton, rescan VST3 plug-ins and load AE-8 MEMORY on an audio track.
4. Begin with DUST LOOP or 8-BIT BASEMENT, and adjust RATE, BITS, BANDWIDTH and MIX.

## Known limitations / next milestone

- No drag-in sample waveform, sampler memory budget, or sample export yet.
  This first build is an *audio insert effect*, not a sample-playing instrument.
- The editor's digital-readout panel is not an interactive waveform view.
- No measured MPC/SP-style hardware response curves; factory programs are original
  AE-8 colorations, not recreations of specific copyrighted or trademarked devices.
- No macOS CI build, installer, or DAW acceptance tests yet.
- Real-world CPU, latency, automation and mono/stereo testing in Ableton are needed.

## Testing

Compile the framework-free DSP test:

```sh
g++ -std=c++17 -O2 Memory/tests/memory_test.cpp -o memory_test
./memory_test
```

The Windows GitHub Actions workflow also builds `AE8MemoryDSPTest` and runs CTest.
