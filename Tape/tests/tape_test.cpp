#include "../TapeEngine.h"
#include <functional>
#include <cstdio>
using namespace ae8tape;
static double freqOf(const std::vector<float>& x, int a, int b, double sr){ int zc=0; for(int i=a+1;i<b;++i) if(x[i-1]<=0&&x[i]>0) ++zc; return zc*sr/(b-a); }
static double rms(const std::vector<float>& x,int a,int b){double s=0;for(int i=a;i<b;++i)s+=x[i]*x[i];return std::sqrt(s/(b-a));}
int main(){
  const double sr=48000; int ok=1;
  auto run=[&](TapeEngine& e,int secs,std::vector<float>& out,double f=1000,std::function<void(int)> hook=nullptr){
    int N=(int)(secs*sr); out.assign(N,0); std::vector<float> in(512),l(512),r(512);
    for(int p=0;p<N;p+=512){ if(hook) hook(p); int n=std::min(512,N-p); for(int i=0;i<n;++i) in[i]=0.3f*std::sin(2*kPi*f*(p+i)/sr);
      e.process(in.data(),in.data(),l.data(),r.data(),n); for(int i=0;i<n;++i){ out[p+i]=l[i]; if(!std::isfinite(l[i])||!std::isfinite(r[i])) ok=0; } } };
  { TapeEngine e; e.prepare(sr); e.setParam(WOW,0); e.setParam(FLUTTER,0); e.setParam(HISS,0); e.setParam(WEAR,0); e.setParam(SATURATE,0);
    std::vector<float> o; run(e,2,o); printf("speed1: f=%.1f rms=%.3f latency=%d\n",freqOf(o,48000,96000,sr),rms(o,48000,96000),e.latencySamples());
    // impulse latency
    TapeEngine e2; e2.prepare(sr); e2.setParam(WOW,0);e2.setParam(FLUTTER,0);e2.setParam(HISS,0);e2.setParam(WEAR,0);e2.setParam(SATURATE,0);
    std::vector<float> in(8192,0),l(8192),r(8192); in[100]=1; e2.process(in.data(),in.data(),l.data(),r.data(),8192);
    int pk=0; for(int i=0;i<8192;++i) if(std::fabs(l[i])>std::fabs(l[pk])) pk=i; printf("impulse peak at %d (expect ~%d)\n",pk,100+e2.latencySamples()); }
  { TapeEngine e; e.prepare(sr); e.setParam(WOW,0); e.setParam(FLUTTER,0); e.setParam(HISS,0); e.setParam(SPEED,0.5f);
    std::vector<float> o; run(e,4,o); printf("speed0.5: f=%.1f (expect ~500)\n",freqOf(o,96000,192000,sr)); }
  { TapeEngine e; e.prepare(sr); std::vector<float> o; run(e,6,o,1000,[&](int p){ e.setParam(STOP, p>sr && p<3*sr ? 1.f:0.f); });
    printf("stop: rms before=%.3f during=%.4f after=%.3f\n",rms(o,24000,48000),rms(o,120000,140000),rms(o,240000,288000));
    printf("after-resync f=%.1f\n",freqOf(o,240000,288000,sr)); }
  { TapeEngine e; e.prepare(sr); e.setParam(SPEED,0.2f); std::vector<float> o; run(e,40,o); printf("long slow: rms end=%.3f\n",rms(o,(int)(38*sr),(int)(40*sr))); }
  { TapeEngine e; e.prepare(sr); for(int i=0;i<NUM_PARAMS;++i) e.setParam(i, paramDefs()[i].max); std::vector<float> o; run(e,3,o); printf("all max rms=%.3f\n",rms(o,48000,144000)); }
  printf(ok?"NO NaN\n":"NaN FOUND\n"); return ok?0:1; }
