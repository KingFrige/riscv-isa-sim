// vwadaccu.vv vd, vs2, vs1
#include "vmatmul_common.h"

require_extension(EXT_ZVMATMUL);
require(P.VU.vsew == e8);
require(P.VU.vl == 64);
require(P.VU.vlmul == 1.0);

VI_VV_LOOP
({
  VQDOT(vs1, vs2, int8_t, int8_t);
  vd = (vd + result) & 0xffffffff;
})


