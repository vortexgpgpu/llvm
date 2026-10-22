# RUN: not llvm-mc -triple riscv32 -mattr=+a,+xvortexamod < %s 2>&1 | FileCheck %s

# rd and rs2 name even-aligned register pairs.
amoadd.d a1, a2, (a0) # CHECK: :[[@LINE]]:10: error: register must be even
amoadd.d a0, a3, (a1) # CHECK: :[[@LINE]]:14: error: register must be even
amoswap.d.aqrl a0, a1, (a2) # CHECK: :[[@LINE]]:20: error: register must be even

# Non-zero offsets are not supported for rs1.
amoor.d a0, a2, 4(a1) # CHECK: :[[@LINE]]:17: error: optional integer offset must be 0

# LR.D and SC.D stay RV64-only.
lr.d a0, (a1) # CHECK: :[[@LINE]]:1: error: instruction requires the following: RV64I Base Instruction Set{{$}}
sc.d a0, a2, (a1) # CHECK: :[[@LINE]]:1: error: instruction requires the following: RV64I Base Instruction Set{{$}}
