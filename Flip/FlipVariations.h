#pragma once
// AE-8 FLIP: MIDI-pad rearrangement presets for original CHOP slice engine.
// Maps a 16-pad bank to a repeatable slice permutation; no source-sample mutation.
#include <algorithm>
#include <numeric>
namespace ae8flip {
// a step that visits every pad once for this slice count
inline int coprimeStep(int n, int preferred) {
  for (int m : { preferred, 3, 5, 7, 11, 13 }) if (std::gcd(m, n) == 1) return m % n == 0 ? 1 : m;
  return 1;
}
// count = number of slices that exist (whole sample); pads past the last slice are left alone.
inline int mapPad(int note, int mode, int count=32) {
  if (note<36||note>=68) return note;
  const int bank=(note-36)/16, pad=(note-36)%16;
  const int n=std::clamp(count-bank*16,1,16);
  if (pad>=n) return note;
  int i=pad;
  switch(mode) {
    case 1: i=n-1-i; break;                  // reverse phrase
    case 2: i=(i+4)%n; break;                // displaced
    case 3: i=(i*coprimeStep(n,5))%n; break; // odd rhythmic permutation
    case 4: i=(i/2+((i%2)?n/2:0))%n; break; // half-time feel
    case 5: i=(i+(i/4)*3)%n; break;          // stutter/skip
    case 6: i=(i*coprimeStep(n,3)+1)%n; break; // alternate start points
    default: break;
  }
  return 36+bank*16+i;
}
} // namespace ae8flip
