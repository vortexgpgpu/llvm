; Vortex treats an inline-asm read of a machine CSR or of the warp-wide user
; CSRs (0xCC1-0xCC4) as always uniform. The CSR is resolved from the asm
; operand that holds it: an "i" argument ($N), a number or a CSR name in the
; asm text. Asm that is not a single CSR instruction, or whose CSR is not
; constant, is divergent like any other call.
;
; RUN: opt -mtriple=riscv32 -mattr=+xvortex -passes='print<uniformity>' \
; RUN:   -disable-output %s 2>&1 | FileCheck %s
; RUN: opt -mtriple=riscv64 -mattr=+xvortex -passes='print<uniformity>' \
; RUN:   -disable-output %s 2>&1 | FileCheck %s

; The CSR is an "i" argument.
; CHECK-LABEL: 'csr_arg'
; CHECK: ALL VALUES UNIFORM
define i32 @csr_arg() {
  %r = call i32 asm sideeffect "csrr $0, $1", "=r,i"(i32 3265)
  ret i32 %r
}

; The CSR is named in the asm text: there is no call argument.
; CHECK-LABEL: 'csr_name_user'
; CHECK: DIVERGENT: %r =
define i32 @csr_name_user() {
  call void asm sideeffect "csrw fflags, zero", ""()
  %r = call i32 asm sideeffect "csrr $0, fflags", "=r"()
  ret i32 %r
}

; CHECK-LABEL: 'csr_name_machine'
; CHECK: ALL VALUES UNIFORM
define i32 @csr_name_machine() {
  %r = call i32 asm sideeffect "csrr\09$0, mhartid", "=r"()
  ret i32 %r
}

; The CSR is a number in the asm text (vortexpipe's form).
; CHECK-LABEL: 'csr_number'
; CHECK: DIVERGENT: %tid =
; CHECK-NOT: DIVERGENT: %wid =
define i32 @csr_number() {
  %tid = call i32 asm sideeffect "csrr $0, 3264", "=r"()
  %wid = call i32 asm sideeffect "csrr $0, 0xcc1", "=r"()
  %r = add i32 %tid, %wid
  ret i32 %r
}

; The first argument is the source register, not the CSR.
; CHECK-LABEL: 'csr_name_with_arg'
; CHECK: DIVERGENT: %r =
define i32 @csr_name_with_arg() {
  %r = call i32 asm sideeffect "csrrw $0, fflags, $1", "=r,r"(i32 3265)
  ret i32 %r
}

; The CSR is the second argument.
; CHECK-LABEL: 'csr_second_arg'
; CHECK: ALL VALUES UNIFORM
define i32 @csr_second_arg() {
  %r = call i32 asm sideeffect "csrrs $0, ${2}, $1", "=r,r,i,~{memory}"(i32 0, i32 3860)
  ret i32 %r
}

; Several instructions: the result is not the CSR.
; CHECK-LABEL: 'csr_then_add'
; CHECK: DIVERGENT: %r =
define i32 @csr_then_add(i32 %x) {
  %r = call i32 asm sideeffect "csrr $0, $1; add $0, $0, $2", "=r,i,r"(i32 3265, i32 %x)
  ret i32 %r
}
