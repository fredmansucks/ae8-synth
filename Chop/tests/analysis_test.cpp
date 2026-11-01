#include "../ChopAnalysis.h"
#include "testloop.h"
#include <cstdio>
using namespace ae8chop;
int main(){ const double sr=44100; for (double bpm : {92.0, 140.0, 75.0}) { auto x=makeLoop(sr,bpm); auto A=analyse(x,sr);
 printf("true %.0f -> bpm %.1f key %s onsets %zu\n",bpm,A.bpm,keyName(A.keyRoot,A.keyMinor),A.onsets.size());
 for(float s:{0.2f,0.6f,0.9f}) printf("  sens %.1f -> %zu slices\n",s,transientSlices(A,s,32).size());
 auto f=findSlices(A,(int64_t)x.size(),16); printf("  find %zu; first: ",f.size()); for(int i=0;i<5;i++) printf("%.3f ",f[i]/sr*bpm/60); printf("(beats)\n"); }
 printf("shift Fmin->Amin %d, Fmin->Cmaj %d\n", keyShift(5,true,9,true), keyShift(5,true,0,false)); }
