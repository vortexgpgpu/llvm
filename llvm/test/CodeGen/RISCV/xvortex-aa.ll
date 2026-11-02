; RUN: opt -mtriple=riscv32 -mattr=+xvortex -passes=aa-eval \
; RUN:   -aa-pipeline=riscv-vortex-aa -print-all-alias-modref-info -disable-output \
; RUN:   < %s 2>&1 | FileCheck %s --check-prefix=ALIAS
; RUN: opt -mtriple=riscv32 -mattr=+xvortex -aa-pipeline=default -passes=licm \
; RUN:   -S < %s | FileCheck %s --check-prefix=VORTEX
; RUN: opt -mtriple=riscv32 -aa-pipeline=default -passes=licm -S < %s \
; RUN:   | FileCheck %s --check-prefix=NOVORTEX

; Global (1) and local (3) memory never alias; generic (0) may alias either.

; ALIAS-LABEL: Function: test_alias
; ALIAS: MayAlias: i8* %gen, i8 addrspace(1)* %global
; ALIAS: MayAlias: i8* %gen, i8 addrspace(3)* %local
; ALIAS: NoAlias: i8 addrspace(1)* %global, i8 addrspace(3)* %local
; ALIAS: MayAlias: i8* %gen, i8* %gen.local
; ALIAS: NoAlias: i8* %gen.local, i8 addrspace(1)* %global
; ALIAS: MayAlias: i8* %gen.local, i8 addrspace(3)* %local
; ALIAS: MayAlias: i8* %gen, i8 addrspace(5)* %other
; ALIAS: MayAlias: i8 addrspace(1)* %global, i8 addrspace(5)* %other
; ALIAS: MayAlias: i8 addrspace(3)* %local, i8 addrspace(5)* %other
; ALIAS: MayAlias: i8* %gen.local, i8 addrspace(5)* %other
define i8 @test_alias(ptr %gen, ptr addrspace(1) %global, ptr addrspace(3) %local, ptr addrspace(5) %other) {
  %gen.local = addrspacecast ptr addrspace(3) %local to ptr
  %v1 = load i8, ptr %gen
  %v2 = load i8, ptr addrspace(1) %global
  %v3 = load i8, ptr addrspace(3) %local
  %v4 = load i8, ptr %gen.local
  %v5 = load i8, ptr addrspace(5) %other
  %r1 = add i8 %v1, %v2
  %r2 = add i8 %r1, %v3
  %r3 = add i8 %r2, %v4
  %r4 = add i8 %r3, %v5
  ret i8 %r4
}

; The default AA pipeline includes the Vortex AA: the local load is invariant
; across the global stores and LICM hoists it.
; VORTEX-LABEL: define void @hoist(
; VORTEX: entry:
; VORTEX-NEXT: %v = load i32, ptr addrspace(3) %l
; NOVORTEX-LABEL: define void @hoist(
; NOVORTEX: loop:
; NOVORTEX-NEXT: %i = phi
; NOVORTEX-NEXT: %v = load i32, ptr addrspace(3) %l
define void @hoist(ptr addrspace(1) %g, ptr addrspace(3) %l, i32 %n) {
entry:
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %v = load i32, ptr addrspace(3) %l
  %p = getelementptr i32, ptr addrspace(1) %g, i32 %i
  store i32 %v, ptr addrspace(1) %p
  %i.next = add i32 %i, 1
  %c = icmp slt i32 %i.next, %n
  br i1 %c, label %loop, label %exit
exit:
  ret void
}
