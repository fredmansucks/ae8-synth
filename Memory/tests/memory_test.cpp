#include "../MemoryEngine.h"
#include <cassert>
#include <cmath>
#include <vector>
int main() {
 ae8memory::Engine e; e.prepare(48000);
 std::vector<float> left(1024),right(1024);
 for(int i=0;i<1024;++i) left[i]=right[i]=std::sin(i*0.03f)*0.6f;
 auto original=left;
 e.set(11025,8,4000,0.3f,1.0f);
 float* chans[]={left.data(),right.data()};
 e.process(chans,2,1024);
 float difference=0;
 for(int i=0;i<1024;++i){assert(std::isfinite(left[i]));assert(std::fabs(left[i]-right[i])<0.00001f);difference+=std::fabs(left[i]-original[i]);}
 assert(difference>1.0f);
 e.prepare(48000); e.set(11025,8,4000,0.3f,0.0f);
 left=original;right=original;e.process(chans,2,1024);
 for(int i=0;i<1024;++i) assert(std::fabs(left[i]-original[i])<0.00001f);
}
