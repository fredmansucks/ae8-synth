#pragma once
// AE-8 FLIP: MIDI-pad rearrangement presets for original CHOP slice engine.
// Maps a 16-pad bank to a repeatable slice permutation; no source-sample mutation.
#include <algorithm>
namespace ae8flip {
inline int mapPad(int note, int mode, int count=16) {
  if (note<36||note>=68) return note;
  const int bank=(note-36)/16, pad=(note-36)%16;
  const int n=std::clamp(count,1,16);
  int i=pad%n;
  switch(mode) {
    case 1: i=n-1-i; break;                  // reverse phrase
    case 2: i=(i+4)%n; break;                // displaced
    case 3: i=(i*5)%n; break;                // odd rhythmic permutation
    case 4: i=(i/2+((i%2)?n/2:0))%n; break; // half-time feel
    case 5: i=(i+(i/4)*3)%n; break;          // stutter/skip
    case 6: i=(i*3+1)%n; break;              // alternate start points
    default: break;
  }
  return 36+bank*16+i;
}
} // namespace ae8flip
