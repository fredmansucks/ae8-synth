#include "../ChopEngine.h"
#include "../ChopAnalysis.h"
#include "testloop.h"
#include <cstdio>
using namespace ae8chop;
static double freqOf(const std::vector<float>& x,int a,int b,double sr){int zc=0;for(int i=a+1;i<b;++i) if(x[i-1]<=0&&x[i]>0) ++zc; return zc*sr/(b-a);}
static int lastNonZero(const std::vector<float>& x){for(int i=(int)x.size()-1;i>=0;--i) if(std::fabs(x[i])>1e-3) return i; return -1;}
struct Rig { ChopEngine e; double sr=48000; std::vector<float> L,R;
  Rig(std::shared_ptr<Sample> s){ e.prepare(sr); std::atomic_store(&e.sample, std::shared_ptr<const Sample>(s)); }
  void run(int samples, std::vector<NoteEvent> ev={}, double bpm=120, bool playing=false, double ppq=0){ L.assign(samples,0); R.assign(samples,0);
    std::vector<float> l(256),r(256); for(int p=0;p<samples;p+=256){ std::vector<NoteEvent> blk; for(auto& x:ev) if(x.offset>=p&&x.offset<p+256){auto y=x;y.offset-=p;blk.push_back(y);}
      e.process(l.data(),r.data(),256,blk,bpm,playing?ppq+p/(60.0/bpm*sr):-1,playing); for(int i=0;i<256&&p+i<samples;++i){L[p+i]=l[i];R[p+i]=r[i]; if(!std::isfinite(l[i])){printf("NaN!\n");exit(1);}}}}
};
int main(){
  // 1 s 440 Hz sine sample, one slice
  auto sine=std::make_shared<Sample>(); sine->sr=44100; for(int i=0;i<44100;++i){float v=0.5f*std::sin(2*M_PI*440*i/44100.0); sine->L.push_back(v); sine->R.push_back(v);}
  { Rig g(sine); g.e.starts[0]=0; g.e.numSlices=1; g.run(96000,{{0,36,1,true}});
    printf("classic 0st : f=%.1f len=%.3fs\n",freqOf(g.L,4800,24000,48000),lastNonZero(g.L)/48000.0); }
  { Rig g(sine); g.e.starts[0]=0; g.e.numSlices=1; g.e.slice[0].v[S_PITCH]=12; g.run(96000,{{0,36,1,true}});
    printf("classic +12 : f=%.1f len=%.3fs (expect 880, 0.5)\n",freqOf(g.L,4800,20000,48000),lastNonZero(g.L)/48000.0); }
  { Rig g(sine); g.e.starts[0]=0; g.e.numSlices=1; g.e.slice[0].v[S_PITCH]=12; g.e.setParam(WARP,1); g.run(96000,{{0,36,1,true}});
    printf("warp +12    : f=%.1f len=%.3fs (expect 880, 1.0)\n",freqOf(g.L,4800,40000,48000),lastNonZero(g.L)/48000.0); }
  { Rig g(sine); g.e.starts[0]=0; g.e.numSlices=1; g.e.setParam(BPM_SYNC,1); g.e.sampleBpm=100; g.run(96000,{{0,36,1,true}},150);
    printf("sync 100->150: f=%.1f len=%.3fs (expect 440, 0.667)\n",freqOf(g.L,4800,28000,48000),lastNonZero(g.L)/48000.0); }
  { Rig g(sine); g.e.starts[0]=0; g.e.numSlices=1; g.e.slice[0].v[S_STRETCH]=2; g.run(120000,{{0,36,1,true}});
    printf("stretch 200%%: f=%.1f len=%.3fs (expect 440, 2.0)\n",freqOf(g.L,4800,80000,48000),lastNonZero(g.L)/48000.0); }
  // real loop: slices, mono choke, repeat
  auto loop=std::make_shared<Sample>(); loop->sr=44100; loop->L=makeLoop(44100); loop->R=loop->L;
  auto A=analyse(loop->L,44100); auto sl=transientSlices(A,0.5f,32);
  { Rig g(loop); for(size_t i=0;i<sl.size();++i) g.e.starts[i]=sl[i]; g.e.numSlices=(int)sl.size(); g.e.sampleBpm=A.bpm;
    g.run(48000,{{0,36,1,true},{12000,37,1,true},{24000,38,0.5f,true}});
    double pk=0; for(float v:g.L) pk=std::max(pk,(double)std::fabs(v)); printf("loop: %zu slices bpm %.1f, peak %.3f\n",sl.size(),A.bpm,pk); }
  { Rig g(loop); for(size_t i=0;i<sl.size();++i) g.e.starts[i]=sl[i]; g.e.numSlices=(int)sl.size();
    g.e.setParam(REPEAT,1); g.e.setParam(REPEAT_RATE,2); // 1/16 at 120 = 6000 samples
    g.run(48000,{{100,36,1,true},{47000,36,0,false}},120,true,0.0);
    printf("  (free-run) "); { Rig h(loop); for(size_t i=0;i<sl.size();++i) h.e.starts[i]=sl[i]; h.e.numSlices=(int)sl.size(); h.e.setParam(REPEAT,1); h.e.setParam(REPEAT_RATE,2); h.e.setParam(SWING,0.66f);
      h.run(48000,{{100,36,1,true},{47000,36,0,false}},120,false); printf("%d triggers with swing\n",h.e.triggerCount.load()); }
    int hits=0; double prev=0; for(int i=1;i<48000;++i){ double e=std::fabs(g.L[i]); if(e>0.3 && prev<0.05) ++hits; prev=prev*0.999+e*0.001; }
    printf("repeat 1/16 over 1s @120: %d triggers (expect 8-9)\n",g.e.triggerCount.load()); (void)hits; }
  { Rig g(loop); for(size_t i=0;i<sl.size();++i) g.e.starts[i]=sl[i]; g.e.numSlices=(int)sl.size();
    g.e.setParam(BITS,6); g.e.setParam(RATE,8000); g.e.setParam(DRIVE,0.6f); g.e.setParam(CUTOFF,1500); g.e.setParam(RES,0.8f);
    for(int i=0;i<(int)sl.size();++i){ g.e.slice[i].v[S_REVERSE]=1; g.e.slice[i].v[S_LOOP]=1; }
    std::vector<NoteEvent> ev; for(int k=0;k<20;++k){ ev.push_back({k*4000,36+k%16,0.8f,true}); ev.push_back({k*4000+3000,36+k%16,0,false}); }
    g.run(96000,ev); double pk=0; for(float v:g.L) pk=std::max(pk,(double)std::fabs(v)); printf("stress reverse+loop+color peak %.3f OK\n",pk); }
}
