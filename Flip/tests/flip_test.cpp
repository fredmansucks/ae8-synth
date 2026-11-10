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
 for(int mode=0;mode<7;++mode)for(int n=36;n<68;++n){
  const int v=mapPad(n,mode);
  assert(v>=36 && v<68);
  assert((v-36)/16==(n-36)/16);
 }
}
