# RUN: not llvm-mc -triple=riscv32 -mattr=+f,+xvortex < %s 2>&1 \
# RUN:     | FileCheck %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+f < %s 2>&1 \
# RUN:     | FileCheck --check-prefix=CHECK-NOVX %s
# RUN: not llvm-mc -triple=riscv64 -mattr=+f,+xvortex < %s 2>&1 \
# RUN:     | FileCheck --check-prefix=CHECK-RV64 %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+d,+xvortex < %s 2>&1 \
# RUN:     | FileCheck --check-prefix=CHECK-D %s

# vx_flds.d needs an even fd, vx_flds.q an fd that is a multiple of 4.
vx_flds.d ft1, 0(a0)
# CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction
vx_flds.d fa1, 8(a0)
# CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction
vx_flds.q ft2, 0(a0)
# CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction
vx_flds.q fa0, 16(a0)
# CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction
vx_flds.q f31, 0(a0)
# CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction

# The destination is an FP register group, the offset a signed 12-bit value.
vx_flds.q a0, 0(a1)
# CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction
vx_flds.q ft0, 2048(a0)
# CHECK: :[[@LINE-1]]:16: error: operand must be a symbol with %lo/%pcrel_lo/%tprel_lo modifier or an integer in the range [-2048, 2047]

# CHECK-NOVX: :[[@LINE+3]]:1: error: instruction requires the following: 'XVortex' (Vortex ISA Extension)
# CHECK-RV64: :[[@LINE+2]]:1: error: instruction requires the following: RV32I Base Instruction Set
# CHECK-D: :[[@LINE+1]]:1: error: instruction requires the following: FLEN 32 ('D' disabled)
vx_flds.q ft0, 0(a0)
# CHECK-D: :[[@LINE+1]]:1: error: instruction requires the following: FLEN 32 ('D' disabled)
vx_flds.d ft0, 0(a0)
