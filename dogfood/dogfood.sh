#!/bin/sh

#LLVM_PREFIX=/opt/llvm-riscv
LLVM_PREFIX=../build_dbg
TEST=hello
#TEST=kernel
#TEST=sorti
#TEST=sortf

RISCV_TOOLCHAIN_PATH=/opt/riscv-gnu-toolchain
SYSROOT=${RISCV_TOOLCHAIN_PATH}/riscv32-unknown-elf

VORTEX_PATH=~/dev/vortex-64b
VORTEX_DRV_PATH=${VORTEX_PATH}/driver
VORTEX_RT_PATH=${VORTEX_PATH}/runtime

DP=${LLVM_PREFIX}/bin/llvm-objdump
CP=${LLVM_PREFIX}/bin/llvm-objcopy

K_LLCFLAGS="-march=riscv32 -target-abi=ilp32f -mcpu=generic-rv32 -mattr=+m,+f,+vortex -float-abi=hard"
K_CFLAGS="--sysroot=${SYSROOT} --gcc-toolchain=${RISCV_TOOLCHAIN_PATH} -march=rv32imf -mabi=ilp32f -Xclang -target-feature -Xclang +vortex -I${VORTEX_PATH}/hw -I${VORTEX_RT_PATH}/include -fno-rtti -fno-exceptions -ffreestanding -nostartfiles -fdata-sections -ffunction-sections"
K_LDFLAGS="-Wl,-Bstatic,-T${VORTEX_RT_PATH}/linker/vx_link32.ld,--gc-sections ${VORTEX_RT_PATH}/libvortexrt.a -lm"

# exit when any command fails
set -e

# list RISCV supported features
specs()
{
    ${LLVM_PREFIX}/bin/llc -march=riscv32 -mattr=help
}

# dump clang AST
clang_ast()
{
    rm -f ${TEST}.ast
    ${LLVM_PREFIX}/bin/clang++ -Xclang -ast-dump -fsyntax-only ${TEST}.c > ${TEST}.ast
}

# dump LLVM IR
llvm_ir()
{
    rm -f ${TEST}.ll .main.dot .main.dot.jpg

    ${LLVM_PREFIX}/bin/clang++ ${K_CFLAGS} -S -emit-llvm ${TEST}.c -o ${TEST}.ll
    
    #${LLVM_PREFIX}/bin/opt -dot-cfg -S -o ${TEST}.ll
    #dot -Tjpg .main.dot > .main.dot.jpg
}

llvm_opt()
{
    rm -f ${TEST}.opt.ll .main.dot .main.opt.dot.jpg
    
    ${LLVM_PREFIX}/bin/opt -stats -time-passes -S ${TEST}.ll -o ${TEST}.opt.ll --lowerswitch --flattencfg --loop-simplify --mergereturn --structurizecfg
    #${LLVM_PREFIX}/bin/opt -stats -time-passes -S ${TEST}.ll -o ${TEST}.opt.ll
    
    ${LLVM_PREFIX}/bin/opt -dot-cfg -S ${TEST}.opt.ll
    dot -Tjpg .main.dot > .main.opt.dot.jpg
}

llc()
{
    rm -f ${TEST}.s
    ${LLVM_PREFIX}/bin/llc -O2 ${K_LLCFLAGS} -filetype=asm ${TEST}.ll -o ${TEST}.s
    #${LLVM_PREFIX}/bin/llc -O2 ${K_LLCFLAGS} --print-after-all -filetype=asm ${TEST}.ll -o ${TEST}.s
    #${LLVM_PREFIX}/bin/llc ${K_LLCFLAGS} -print-machineinstrs -filetype=asm ${TEST}.ll -o ${TEST}.s > build.log 2>&1
}

# compile RISCV
codegen()
{
    rm -f ${TEST}.o
    rm -f ${TEST}.s
    rm -f ${TEST}.hex
    rm -f ${TEST}.elf
    rm -f ${TEST}.dump
    #${LLVM_PREFIX}/bin/clang ${TEST}.c -v --target=riscv32 -march=rv32imf -Xclang -target-feature -Xclang +vortex -S -o ${TEST}.s
    #${LLVM_PREFIX}/bin/clang ${TEST}.c -v -Wl,-Bstatic,-T${VORTEX_RT_PATH}/linker/vx_link.ld -Wl,--gc-sections ${VORTEX_RT_PATH}/libvortexrt.a -lm --sysroot=${SYSROOT} --gcc-toolchain=${RISCV_TOOLCHAIN_PATH} -march=rv32imf -mabi=ilp32f -I${VORTEX_RT_PATH}/include -fno-rtti -fno-exceptions -ffreestanding -nostartfiles -fdata-sections -ffunction-sections --target=riscv32 -march=rv32imf -Xclang -target-feature -Xclang +vortex -o ${TEST}.elf
    #${LLVM_PREFIX}/bin/clang ${K_CFLAGS} ${TEST}.c ${K_LDFLAGS} -o ${TEST}.elf -mllvm -debug-pass=Arguments > build.log 2>&1
    #${LLVM_PREFIX}/bin/clang ${K_CFLAGS} ${TEST}.c -S -o ${TEST}.s 2> /dev/null    
    #${LLVM_PREFIX}/bin/clang ${K_CFLAGS} ${TEST}.c -c -o ${TEST}.o

    ${LLVM_PREFIX}/bin/clang++ -O0 ${K_CFLAGS} ${TEST}.c -c -o ${TEST}.o -mllvm -debug-pass=Arguments > build.log 2>&1

    ${CP} -O ihex ${TEST}.elf ${TEST}.hex
    ${CP} -O binary ${TEST}.elf ${TEST}.bin
    ${DP} -arch=riscv32 -mcpu=generic-rv32 -mattr=+m,+f -mattr=+vortex -D ${TEST}.elf > ${TEST}.dump
}

# running
run()
{
    #${VORTEX_PATH}/sim/simX/simX -a rv32i -c 1 -i ${TEST}.bin > run.log 2>&1
    ${VORTEX_PATH}/sim/rtlsim/rtlsim ${TEST}.bin > run.log 2>&1
}

usage()
{
    echo "usage: dogfood [-specs] [-ast] [-ir] [-opt] [-llc] [-bin] [-run] [-h|--help]"
}

while [ "$1" != "" ]; do
    case $1 in
        -specs ) specs
                ;;
        -ast ) clang_ast
                ;;
        -ir ) llvm_ir
                ;;
        -opt ) llvm_opt
                ;;
        -llc ) llc
                ;;
        -bin ) codegen
                ;;
        -run ) run
                ;;
        -h | --help ) usage
                      exit
                      ;;
        * )           usage
                      exit 1
    esac
    shift
done