#!/bin/bash

WORKSPACE=$PWD

RISCV=$WORKSPACE/xpack-riscv-none-elf-gcc-12.2.0-1
if [[ -v RISCV ]]; then
  echo "RISCV=${RISCV}"
else
  echo "please set RISCV"
  exit
fi

openocd_dir="${WORKSPACE}/riscv-openocd"
openocd_bin="${WORKSPACE}/riscv-openocd/src/openocd"

if [[ ! -d "${openocd_dir}" ]]; then
  # Download OpenOCD
  cd ${WORKSPACE}
  git clone --recurse-submodules https://github.com/riscv/riscv-openocd.git
fi

if [[ ! -f "${openocd_bin}" ]]; then
  #sudo xargs apt-get install -y < .github/workflows/apt-packages.txt

  # Build OpenOCD
  cd ${WORKSPACE}
  cd riscv-openocd
  ./bootstrap
  ./configure
  make -j"$(nproc 2> /dev/null || sysctl -n hw.ncpu)"
fi

gcc_path="$RISCV/bin/riscv-none-elf-gcc"
if [[ ! -f "${gcc_path}" ]]; then
  cd ${WORKSPACE}
  toolchain_tar="${WORKSPACE}/xpack-riscv-none-elf-gcc-12.2.0-1-linux-x64.tar.gz"
  if [[ ! -f "${toolchain_tar}" ]]; then
    wget --progress=dot:giga https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v12.2.0-1/xpack-riscv-none-elf-gcc-12.2.0-1-linux-x64.tar.gz
  fi
  tar zxf xpack-riscv-none-elf-gcc-12.2.0-1-linux-x64.tar.gz
fi

# Build Spike
cd ${WORKSPACE}
ci-tests/build-spike

# Download Tests
if [[ ! -d "riscv-tests/" ]]; then
  cd ${WORKSPACE}
  git clone --recurse-submodules https://github.com/riscv-software-src/riscv-tests.git
fi

cd riscv-tests
git checkout e06a435c1e545def71e833031356372f0828f165

# Run Tests
cd ${WORKSPACE}
cd riscv-tests/debug
./gdbserver.py targets/RISC-V/spike32.py --print-failures \
  --gcc $RISCV/bin/riscv-none-elf-gcc \
  --gdb $RISCV/bin/riscv-none-elf-gdb \
  --sim_cmd $WORKSPACE/install/bin/spike \
  --server_cmd $WORKSPACE/riscv-openocd/src/openocd

./gdbserver.py targets/RISC-V/spike64-2.py --print-failures \
  --gcc ${RISCV}/bin/riscv-none-elf-gcc \
  --gdb ${RISCV}/bin/riscv-none-elf-gdb \
  --sim_cmd $WORKSPACE/install/bin/spike \
  --server_cmd $WORKSPACE/riscv-openocd/src/openocd
