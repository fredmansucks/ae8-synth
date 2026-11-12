#include "../FlipVariations.h"
#include <cassert>
int main() {
 using ae8flip::mapPad;
 assert(mapPad(36,0)==36);
 assert(mapPad(36,1)==51);
 assert(mapPad(36,2)==40);
 assert(mapPad(38,3)==46);
 assert(mapPad(60,0)==60);
 assert(mapPad(60,1)==59);
 assert(mapPad(35,1)==35);
 // slice-count aware: with 8 slices nothing maps past slice 8, pads 9-16 stay put
 for(int mode=0;mode<7;++mode)for(int n=36;n<44;++n){ const int v=mapPad(n,mode,8); assert(v>=36 && v<44); }
 for(int mode=0;mode<7;++mode) assert(mapPad(46,mode,8)==46);
 // permutation modes visit every slice once for 10 slices
 for(int mode : {1,2,3,6}){ bool seen[10]={}; for(int n=36;n<46;++n) seen[mapPad(n,mode,10)-36]=true; for(bool b:seen) assert(b); }
 for(int mode=0;mode<7;++mode)for(int n=36;n<68;++n){
  const int v=mapPad(n,mode);
  assert(v>=36 && v<68);
  assert((v-36)/16==(n-36)/16);
 }
}
