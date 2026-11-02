# RUN: llvm-mc %s -triple=riscv32 -mattr=+f,+xvortex -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+f,+xvortex < %s \
# RUN:     | llvm-objdump --mattr=+f,+xvortex --no-print-imm-hex -d - \
# RUN:     | FileCheck --check-prefix=CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+f,+xvortex < %s \
# RUN:     | llvm-objdump --mattr=+f -d - \
# RUN:     | FileCheck --check-prefix=CHECK-UNKNOWN %s

# vx_flds.d (funct3 3) and vx_flds.q (funct3 4): custom-3, I-type.

# CHECK-ASM-AND-OBJ: vx_flds.d ft0, 0(a0)
# CHECK-ASM: encoding: [0x7b,0x30,0x05,0x00]
# CHECK-UNKNOWN: 0005307b <unknown>
vx_flds.d ft0, 0(a0)
# CHECK-ASM-AND-OBJ: vx_flds.d fa0, 8(sp)
# CHECK-ASM: encoding: [0x7b,0x35,0x81,0x00]
vx_flds.d fa0, 8(sp)
# CHECK-ASM-AND-OBJ: vx_flds.d ft10, -2048(s0)
# CHECK-ASM: encoding: [0x7b,0x3f,0x04,0x80]
vx_flds.d f30, -2048(s0)

# CHECK-ASM-AND-OBJ: vx_flds.q ft0, 0(a0)
# CHECK-ASM: encoding: [0x7b,0x40,0x05,0x00]
vx_flds.q ft0, 0(a0)
# CHECK-ASM-AND-OBJ: vx_flds.q fa2, -16(a1)
# CHECK-ASM: encoding: [0x7b,0xc6,0x05,0xff]
vx_flds.q fa2, -16(a1)
# CHECK-ASM-AND-OBJ: vx_flds.q fs8, 2047(t0)
# CHECK-ASM: encoding: [0x7b,0xcc,0xf2,0x7f]
vx_flds.q fs8, 2047(t0)
# CHECK-ASM-AND-OBJ: vx_flds.q ft8, 64(a5)
# CHECK-ASM: encoding: [0x7b,0xce,0x07,0x04]
vx_flds.q f28, 64(a5)
