# RUN: llvm-mc %s -triple=riscv32 -mattr=+f,+xvortexldstwide -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+f,+xvortexldstwide < %s \
# RUN:     | llvm-objdump --mattr=+f,+xvortexldstwide --no-print-imm-hex -d - \
# RUN:     | FileCheck --check-prefix=CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+f,+xvortexldstwide < %s \
# RUN:     | llvm-objdump --mattr=+f -d - \
# RUN:     | FileCheck --check-prefix=CHECK-UNKNOWN %s

# custom-3: vx_flw.d (I-type, funct3 3), vx_flw.q (funct3 4),
# vx_fsw.d (S-type, funct3 5), vx_fsw.q (funct3 6).

# CHECK-ASM-AND-OBJ: vx_flw.d ft0, 0(a0)
# CHECK-ASM: encoding: [0x7b,0x30,0x05,0x00]
# CHECK-UNKNOWN: 0005307b <unknown>
vx_flw.d ft0, 0(a0)
# CHECK-ASM-AND-OBJ: vx_flw.d fa0, 8(sp)
# CHECK-ASM: encoding: [0x7b,0x35,0x81,0x00]
vx_flw.d fa0, 8(sp)
# CHECK-ASM-AND-OBJ: vx_flw.d ft10, -2048(s0)
# CHECK-ASM: encoding: [0x7b,0x3f,0x04,0x80]
vx_flw.d f30, -2048(s0)
# CHECK-ASM-AND-OBJ: vx_flw.d ft2, 2040(a1)
# CHECK-ASM: encoding: [0x7b,0xb1,0x85,0x7f]
vx_flw.d f2, 2040(a1)

# CHECK-ASM-AND-OBJ: vx_flw.q ft0, 0(a0)
# CHECK-ASM: encoding: [0x7b,0x40,0x05,0x00]
# CHECK-UNKNOWN: 0005407b <unknown>
vx_flw.q ft0, 0(a0)
# CHECK-ASM-AND-OBJ: vx_flw.q fa2, -16(a1)
# CHECK-ASM: encoding: [0x7b,0xc6,0x05,0xff]
vx_flw.q fa2, -16(a1)
# CHECK-ASM-AND-OBJ: vx_flw.q fs8, 2032(t0)
# CHECK-ASM: encoding: [0x7b,0xcc,0x02,0x7f]
vx_flw.q fs8, 2032(t0)
# CHECK-ASM-AND-OBJ: vx_flw.q ft8, 64(a5)
# CHECK-ASM: encoding: [0x7b,0xce,0x07,0x04]
vx_flw.q f28, 64(a5)

# CHECK-ASM-AND-OBJ: vx_fsw.d ft0, 0(a0)
# CHECK-ASM: encoding: [0x7b,0x50,0x05,0x00]
# CHECK-UNKNOWN: 0005507b <unknown>
vx_fsw.d ft0, 0(a0)
# CHECK-ASM-AND-OBJ: vx_fsw.d fa0, 8(sp)
# CHECK-ASM: encoding: [0x7b,0x54,0xa1,0x00]
vx_fsw.d fa0, 8(sp)
# CHECK-ASM-AND-OBJ: vx_fsw.d ft10, -2048(s0)
# CHECK-ASM: encoding: [0x7b,0x50,0xe4,0x81]
vx_fsw.d f30, -2048(s0)
# CHECK-ASM-AND-OBJ: vx_fsw.d ft2, 2040(a1)
# CHECK-ASM: encoding: [0x7b,0xdc,0x25,0x7e]
vx_fsw.d f2, 2040(a1)

# CHECK-ASM-AND-OBJ: vx_fsw.q ft0, 0(a0)
# CHECK-ASM: encoding: [0x7b,0x60,0x05,0x00]
# CHECK-UNKNOWN: 0005607b <unknown>
vx_fsw.q ft0, 0(a0)
# CHECK-ASM-AND-OBJ: vx_fsw.q fa2, -16(a1)
# CHECK-ASM: encoding: [0x7b,0xe8,0xc5,0xfe]
vx_fsw.q fa2, -16(a1)
# CHECK-ASM-AND-OBJ: vx_fsw.q fs8, 2032(t0)
# CHECK-ASM: encoding: [0x7b,0xe8,0x82,0x7f]
vx_fsw.q fs8, 2032(t0)
# CHECK-ASM-AND-OBJ: vx_fsw.q ft8, 64(a5)
# CHECK-ASM: encoding: [0x7b,0xe0,0xc7,0x05]
vx_fsw.q f28, 64(a5)
