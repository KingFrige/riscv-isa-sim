// vfredsum: vd[0] =  sum( vs2[*] , vs1[0] )
bool is_propagate = true;
VI_VFP_VV_LOOP_UNORDER_REDUCTION
({
  result = f16_add(val_0, val_1);
},
{
  result = f32_add(val_0, val_1);
},
{
  result = f64_add(val_0, val_1);
})
