; Every Vortex thread has its own stack at the same addresses, so a pointer
; into the stack is uniform while the data behind it differs per thread. A
; branch on such data must be lowered as divergent (vx_split/vx_join), however
; the address reaches the load: through an argument, a returned pointer, an
; aggregate, a byval copy, or a global the address was stored to. An argument
; is uniform only if every call site passes a uniform value, whatever order
; the call sites are analysed in. Uniform data keeps a uniform branch.
;
; RUN: llc -mtriple=riscv32 -mattr=+m,+f,+xvortex,+zicond < %s | FileCheck %s

declare i32 @get_tid()

@g = global i32 0
@gp = global ptr null
@src = global [2 x i32] zeroinitializer
@tab = global [2 x i32] zeroinitializer

; CHECK-LABEL: by_ref:
; CHECK: vx_split
define internal void @by_ref(ptr %p) noinline {
  %q = getelementptr inbounds i8, ptr %p, i32 4
  %v = load i32, ptr %q
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

define internal ptr @pick(ptr %s) noinline {
  %f = getelementptr inbounds i8, ptr %s, i32 4
  ret ptr %f
}

; CHECK-LABEL: by_returned_ptr:
; CHECK: vx_split
define internal void @by_returned_ptr(ptr %p) noinline {
  %q = call ptr @pick(ptr %p)
  %v = load i32, ptr %q
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

; CHECK-LABEL: by_aggregate:
; CHECK: vx_split
define internal void @by_aggregate(ptr %p) noinline {
  %agg = insertvalue {ptr, i32} undef, ptr %p, 0
  %q0 = extractvalue {ptr, i32} %agg, 0
  %q = getelementptr inbounds i8, ptr %q0, i32 4
  %v = load i32, ptr %q
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

; CHECK-LABEL: by_byval:
; CHECK: vx_split
define internal void @by_byval(ptr byval([2 x i32]) %p, i32 %t) noinline {
  store i32 %t, ptr %p
  %v = load i32, ptr %p
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

; CHECK-LABEL: by_escaped_global:
; CHECK: vx_split
define internal void @by_escaped_global() noinline {
  %p = load ptr, ptr @gp
  %v = load i32, ptr %p
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

; Called with a divergent value first and a uniform one second.
; CHECK-LABEL: mixed_call_sites:
; CHECK: vx_split
define internal void @mixed_call_sites(i32 %v) noinline {
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

; Uniform data: a global table, and a constant at every call site.
; CHECK-LABEL: uniform_global:
; CHECK-NOT: vx_split
; CHECK: ret
define internal void @uniform_global(i32 %i) noinline {
  %q = getelementptr inbounds [2 x i32], ptr @tab, i32 0, i32 %i
  %v = load i32, ptr %q
  %c = icmp eq i32 %v, 0
  br i1 %c, label %a, label %b
a:
  store volatile i32 1, ptr @g
  br label %e
b:
  store volatile i32 2, ptr @g
  br label %e
e:
  ret void
}

define void @kernel() "vortex-kernel" {
  %s = alloca [2 x i32]
  %t = call i32 @get_tid()
  %s1 = getelementptr inbounds i8, ptr %s, i32 4
  store i32 %t, ptr %s1
  call void @by_ref(ptr %s)
  call void @by_returned_ptr(ptr %s)
  call void @by_aggregate(ptr %s)
  call void @by_byval(ptr byval([2 x i32]) @src, i32 %t)
  store ptr %s, ptr @gp
  call void @by_escaped_global()
  call void @mixed_call_sites(i32 %t)
  call void @mixed_call_sites(i32 7)
  call void @uniform_global(i32 1)
  call void @uniform_global(i32 1)
  ret void
}
