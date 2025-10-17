// vfwmatmul.vv vd, vs2, vs1

VI_VFP_BASE;
ZVMATMUL_INIT(2);

ZVMATMUL_LOOP(uint16_t, uint16_t, float32_t, zvfwbdot16bf_dot_acc);
