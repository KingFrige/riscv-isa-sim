#!/bin/bash

WORKSPAPE=$PWD

module load riscv-toolchain/office-v2025.07.16

rm -rf riscv-tests

git clone git@github.com:riscv-software-src/riscv-tests.git
cd riscv-tests/
git submodule update --init --recursive

mkdir build
autoconf
cd build
../configure
make

cd ${WORKSPAPE}
executable_files=()
while IFS= read -r -d '' file; do
  executable_files+=("$file")
done < <(find "riscv-tests/build/isa" -maxdepth 1 -type f -executable -print0)

error_cnt=0
spike_path="${WORKSPAPE}/install/bin/spike"
for file in "${executable_files[@]}"; do
  echo "spike $file"

  run_status=0
  if [[ "$file" =~ rv32 ]]; then
    ${spike_path} --isa="rv32gcvh_zicbom_zicbop_zicboz_zicond_zicntr_zihpm_zihintntl_zihintpause_zimop_zfa_zfbfmin_zfhmin_zba_zbb_zbc_zbs_zca_zcb_zcmop_smcsrind_smcntrpmf_smepmp_smstateen_sscofpmf_sscsrind_sstc_svinval_svnapot_svpbmt" $file
    run_status=$?
  elif [[ "$file" =~ rv64 ]]; then
    ${spike_path} --isa="rv64gcvh_zicbom_zicbop_zicboz_zicond_zicntr_zihpm_zihintntl_zihintpause_zimop_zfa_zfbfmin_zfhmin_zba_zbb_zbc_zbs_zca_zcb_zcmop_smcsrind_smcntrpmf_smepmp_smstateen_sscofpmf_sscsrind_sstc_svinval_svnapot_svpbmt" $file
    run_status=$?
  else
    continue
  fi

  if [[ run_status -ne 0  ]];then
    echo ""
    echo "Error: $file run Error!"
    echo ""
    ((error_cnt++))
  fi
done

echo ""
echo "run ${#executable_files[@]} tests!"
echo "error case: ${error_cnt}"

if [[ error_cnt -gt 0 ]]; then
  exit 1
else
  exit 0
fi

