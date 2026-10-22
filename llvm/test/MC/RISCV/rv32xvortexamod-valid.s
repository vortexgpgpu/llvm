# XVortexAMOD: the .D atomics on RV32 name even-aligned register pairs.
# Without it they stay RV64-only.
# RUN: llvm-mc %s -triple=riscv32 -mattr=+xvortexamod -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+xvortexamod < %s \
# RUN:     | llvm-objdump --mattr=+xvortexamod -M no-aliases -d -r - \
# RUN:     | FileCheck --check-prefix=CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+xvortexamod \
# RUN:     --riscv-add-build-attributes < %s \
# RUN:     | llvm-objdump -M no-aliases -d -r - \
# RUN:     | FileCheck --check-prefix=CHECK-ASM-AND-OBJ %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+a -show-encoding %s 2>&1 \
# RUN:        | FileCheck %s --check-prefix=CHECK-ERROR

# CHECK-ASM-AND-OBJ: amoswap.d a0, a2, (a1)
# CHECK-ASM: encoding: [0x2f,0xb5,0xc5,0x08]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoswap.d a0, a2, (a1)
# CHECK-ASM-AND-OBJ: amoadd.d t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0x01]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoadd.d t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amoadd.d.aq t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0x05]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoadd.d.aq t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amoadd.d.rl t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0x03]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoadd.d.rl t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amoadd.d.aqrl t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0x07]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoadd.d.aqrl t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amoxor.d a0, a2, (a1)
# CHECK-ASM: encoding: [0x2f,0xb5,0xc5,0x20]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoxor.d a0, a2, (a1)
# CHECK-ASM-AND-OBJ: amoand.d t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0x61]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoand.d t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amoor.d a0, a2, (a1)
# CHECK-ASM: encoding: [0x2f,0xb5,0xc5,0x40]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoor.d a0, a2, (a1)
# CHECK-ASM-AND-OBJ: amomin.d t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0x81]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amomin.d t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amomax.d a0, a2, (a1)
# CHECK-ASM: encoding: [0x2f,0xb5,0xc5,0xa0]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amomax.d a0, a2, (a1)
# CHECK-ASM-AND-OBJ: amominu.d t1, a6, (s0)
# CHECK-ASM: encoding: [0x2f,0x33,0x04,0xc1]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amominu.d t1, a6, (s0)
# CHECK-ASM-AND-OBJ: amomaxu.d a0, a2, (a1)
# CHECK-ASM: encoding: [0x2f,0xb5,0xc5,0xe0]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amomaxu.d a0, a2, (a1)
# CHECK-ASM-AND-OBJ: amoor.d zero, zero, (a0)
# CHECK-ASM: encoding: [0x2f,0x30,0x05,0x40]
# CHECK-ERROR: instruction requires the following: RV64I Base Instruction Set{{$}}
amoor.d zero, zero, (a0)
