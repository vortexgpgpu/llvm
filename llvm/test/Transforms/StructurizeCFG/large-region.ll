; RUN: opt -S -passes=structurizecfg %s | FileCheck %s
;
; One region of 161 blocks: every "then" arm may leave for %exit, so no smaller
; single-entry single-exit region forms inside it. The region is structurized
; whatever its size; only a function the Vortex divergence pipeline marks
; "vortex-divergence-skipped" is left alone. Structurized, %exit is entered
; from a single flow block.

; CHECK-LABEL: define void @big(
; CHECK: {{^}}exit:{{.*}}; preds = %Flow{{[0-9]+}}{{$}}

define void @big(i32 %n, ptr %p) {
entry:
  br label %b0
b0:
  %c0 = icmp sgt i32 %n, 0
  br i1 %c0, label %t0, label %e0
t0:
  store volatile i32 0, ptr %p
  %d0 = icmp eq i32 %n, 3
  br i1 %d0, label %exit, label %j0
e0:
  store volatile i32 1000, ptr %p
  br label %j0
j0:
  br label %b1
b1:
  %c1 = icmp sgt i32 %n, 1
  br i1 %c1, label %t1, label %e1
t1:
  store volatile i32 1, ptr %p
  %d1 = icmp eq i32 %n, 10
  br i1 %d1, label %exit, label %j1
e1:
  store volatile i32 1001, ptr %p
  br label %j1
j1:
  br label %b2
b2:
  %c2 = icmp sgt i32 %n, 2
  br i1 %c2, label %t2, label %e2
t2:
  store volatile i32 2, ptr %p
  %d2 = icmp eq i32 %n, 17
  br i1 %d2, label %exit, label %j2
e2:
  store volatile i32 1002, ptr %p
  br label %j2
j2:
  br label %b3
b3:
  %c3 = icmp sgt i32 %n, 3
  br i1 %c3, label %t3, label %e3
t3:
  store volatile i32 3, ptr %p
  %d3 = icmp eq i32 %n, 24
  br i1 %d3, label %exit, label %j3
e3:
  store volatile i32 1003, ptr %p
  br label %j3
j3:
  br label %b4
b4:
  %c4 = icmp sgt i32 %n, 4
  br i1 %c4, label %t4, label %e4
t4:
  store volatile i32 4, ptr %p
  %d4 = icmp eq i32 %n, 31
  br i1 %d4, label %exit, label %j4
e4:
  store volatile i32 1004, ptr %p
  br label %j4
j4:
  br label %b5
b5:
  %c5 = icmp sgt i32 %n, 5
  br i1 %c5, label %t5, label %e5
t5:
  store volatile i32 5, ptr %p
  %d5 = icmp eq i32 %n, 38
  br i1 %d5, label %exit, label %j5
e5:
  store volatile i32 1005, ptr %p
  br label %j5
j5:
  br label %b6
b6:
  %c6 = icmp sgt i32 %n, 6
  br i1 %c6, label %t6, label %e6
t6:
  store volatile i32 6, ptr %p
  %d6 = icmp eq i32 %n, 45
  br i1 %d6, label %exit, label %j6
e6:
  store volatile i32 1006, ptr %p
  br label %j6
j6:
  br label %b7
b7:
  %c7 = icmp sgt i32 %n, 7
  br i1 %c7, label %t7, label %e7
t7:
  store volatile i32 7, ptr %p
  %d7 = icmp eq i32 %n, 52
  br i1 %d7, label %exit, label %j7
e7:
  store volatile i32 1007, ptr %p
  br label %j7
j7:
  br label %b8
b8:
  %c8 = icmp sgt i32 %n, 8
  br i1 %c8, label %t8, label %e8
t8:
  store volatile i32 8, ptr %p
  %d8 = icmp eq i32 %n, 59
  br i1 %d8, label %exit, label %j8
e8:
  store volatile i32 1008, ptr %p
  br label %j8
j8:
  br label %b9
b9:
  %c9 = icmp sgt i32 %n, 9
  br i1 %c9, label %t9, label %e9
t9:
  store volatile i32 9, ptr %p
  %d9 = icmp eq i32 %n, 66
  br i1 %d9, label %exit, label %j9
e9:
  store volatile i32 1009, ptr %p
  br label %j9
j9:
  br label %b10
b10:
  %c10 = icmp sgt i32 %n, 10
  br i1 %c10, label %t10, label %e10
t10:
  store volatile i32 10, ptr %p
  %d10 = icmp eq i32 %n, 73
  br i1 %d10, label %exit, label %j10
e10:
  store volatile i32 1010, ptr %p
  br label %j10
j10:
  br label %b11
b11:
  %c11 = icmp sgt i32 %n, 11
  br i1 %c11, label %t11, label %e11
t11:
  store volatile i32 11, ptr %p
  %d11 = icmp eq i32 %n, 80
  br i1 %d11, label %exit, label %j11
e11:
  store volatile i32 1011, ptr %p
  br label %j11
j11:
  br label %b12
b12:
  %c12 = icmp sgt i32 %n, 12
  br i1 %c12, label %t12, label %e12
t12:
  store volatile i32 12, ptr %p
  %d12 = icmp eq i32 %n, 87
  br i1 %d12, label %exit, label %j12
e12:
  store volatile i32 1012, ptr %p
  br label %j12
j12:
  br label %b13
b13:
  %c13 = icmp sgt i32 %n, 13
  br i1 %c13, label %t13, label %e13
t13:
  store volatile i32 13, ptr %p
  %d13 = icmp eq i32 %n, 94
  br i1 %d13, label %exit, label %j13
e13:
  store volatile i32 1013, ptr %p
  br label %j13
j13:
  br label %b14
b14:
  %c14 = icmp sgt i32 %n, 14
  br i1 %c14, label %t14, label %e14
t14:
  store volatile i32 14, ptr %p
  %d14 = icmp eq i32 %n, 101
  br i1 %d14, label %exit, label %j14
e14:
  store volatile i32 1014, ptr %p
  br label %j14
j14:
  br label %b15
b15:
  %c15 = icmp sgt i32 %n, 15
  br i1 %c15, label %t15, label %e15
t15:
  store volatile i32 15, ptr %p
  %d15 = icmp eq i32 %n, 108
  br i1 %d15, label %exit, label %j15
e15:
  store volatile i32 1015, ptr %p
  br label %j15
j15:
  br label %b16
b16:
  %c16 = icmp sgt i32 %n, 16
  br i1 %c16, label %t16, label %e16
t16:
  store volatile i32 16, ptr %p
  %d16 = icmp eq i32 %n, 115
  br i1 %d16, label %exit, label %j16
e16:
  store volatile i32 1016, ptr %p
  br label %j16
j16:
  br label %b17
b17:
  %c17 = icmp sgt i32 %n, 17
  br i1 %c17, label %t17, label %e17
t17:
  store volatile i32 17, ptr %p
  %d17 = icmp eq i32 %n, 122
  br i1 %d17, label %exit, label %j17
e17:
  store volatile i32 1017, ptr %p
  br label %j17
j17:
  br label %b18
b18:
  %c18 = icmp sgt i32 %n, 18
  br i1 %c18, label %t18, label %e18
t18:
  store volatile i32 18, ptr %p
  %d18 = icmp eq i32 %n, 129
  br i1 %d18, label %exit, label %j18
e18:
  store volatile i32 1018, ptr %p
  br label %j18
j18:
  br label %b19
b19:
  %c19 = icmp sgt i32 %n, 19
  br i1 %c19, label %t19, label %e19
t19:
  store volatile i32 19, ptr %p
  %d19 = icmp eq i32 %n, 136
  br i1 %d19, label %exit, label %j19
e19:
  store volatile i32 1019, ptr %p
  br label %j19
j19:
  br label %b20
b20:
  %c20 = icmp sgt i32 %n, 20
  br i1 %c20, label %t20, label %e20
t20:
  store volatile i32 20, ptr %p
  %d20 = icmp eq i32 %n, 143
  br i1 %d20, label %exit, label %j20
e20:
  store volatile i32 1020, ptr %p
  br label %j20
j20:
  br label %b21
b21:
  %c21 = icmp sgt i32 %n, 21
  br i1 %c21, label %t21, label %e21
t21:
  store volatile i32 21, ptr %p
  %d21 = icmp eq i32 %n, 150
  br i1 %d21, label %exit, label %j21
e21:
  store volatile i32 1021, ptr %p
  br label %j21
j21:
  br label %b22
b22:
  %c22 = icmp sgt i32 %n, 22
  br i1 %c22, label %t22, label %e22
t22:
  store volatile i32 22, ptr %p
  %d22 = icmp eq i32 %n, 157
  br i1 %d22, label %exit, label %j22
e22:
  store volatile i32 1022, ptr %p
  br label %j22
j22:
  br label %b23
b23:
  %c23 = icmp sgt i32 %n, 23
  br i1 %c23, label %t23, label %e23
t23:
  store volatile i32 23, ptr %p
  %d23 = icmp eq i32 %n, 164
  br i1 %d23, label %exit, label %j23
e23:
  store volatile i32 1023, ptr %p
  br label %j23
j23:
  br label %b24
b24:
  %c24 = icmp sgt i32 %n, 24
  br i1 %c24, label %t24, label %e24
t24:
  store volatile i32 24, ptr %p
  %d24 = icmp eq i32 %n, 171
  br i1 %d24, label %exit, label %j24
e24:
  store volatile i32 1024, ptr %p
  br label %j24
j24:
  br label %b25
b25:
  %c25 = icmp sgt i32 %n, 25
  br i1 %c25, label %t25, label %e25
t25:
  store volatile i32 25, ptr %p
  %d25 = icmp eq i32 %n, 178
  br i1 %d25, label %exit, label %j25
e25:
  store volatile i32 1025, ptr %p
  br label %j25
j25:
  br label %b26
b26:
  %c26 = icmp sgt i32 %n, 26
  br i1 %c26, label %t26, label %e26
t26:
  store volatile i32 26, ptr %p
  %d26 = icmp eq i32 %n, 185
  br i1 %d26, label %exit, label %j26
e26:
  store volatile i32 1026, ptr %p
  br label %j26
j26:
  br label %b27
b27:
  %c27 = icmp sgt i32 %n, 27
  br i1 %c27, label %t27, label %e27
t27:
  store volatile i32 27, ptr %p
  %d27 = icmp eq i32 %n, 192
  br i1 %d27, label %exit, label %j27
e27:
  store volatile i32 1027, ptr %p
  br label %j27
j27:
  br label %b28
b28:
  %c28 = icmp sgt i32 %n, 28
  br i1 %c28, label %t28, label %e28
t28:
  store volatile i32 28, ptr %p
  %d28 = icmp eq i32 %n, 199
  br i1 %d28, label %exit, label %j28
e28:
  store volatile i32 1028, ptr %p
  br label %j28
j28:
  br label %b29
b29:
  %c29 = icmp sgt i32 %n, 29
  br i1 %c29, label %t29, label %e29
t29:
  store volatile i32 29, ptr %p
  %d29 = icmp eq i32 %n, 206
  br i1 %d29, label %exit, label %j29
e29:
  store volatile i32 1029, ptr %p
  br label %j29
j29:
  br label %b30
b30:
  %c30 = icmp sgt i32 %n, 30
  br i1 %c30, label %t30, label %e30
t30:
  store volatile i32 30, ptr %p
  %d30 = icmp eq i32 %n, 213
  br i1 %d30, label %exit, label %j30
e30:
  store volatile i32 1030, ptr %p
  br label %j30
j30:
  br label %b31
b31:
  %c31 = icmp sgt i32 %n, 31
  br i1 %c31, label %t31, label %e31
t31:
  store volatile i32 31, ptr %p
  %d31 = icmp eq i32 %n, 220
  br i1 %d31, label %exit, label %j31
e31:
  store volatile i32 1031, ptr %p
  br label %j31
j31:
  br label %b32
b32:
  %c32 = icmp sgt i32 %n, 32
  br i1 %c32, label %t32, label %e32
t32:
  store volatile i32 32, ptr %p
  %d32 = icmp eq i32 %n, 227
  br i1 %d32, label %exit, label %j32
e32:
  store volatile i32 1032, ptr %p
  br label %j32
j32:
  br label %b33
b33:
  %c33 = icmp sgt i32 %n, 33
  br i1 %c33, label %t33, label %e33
t33:
  store volatile i32 33, ptr %p
  %d33 = icmp eq i32 %n, 234
  br i1 %d33, label %exit, label %j33
e33:
  store volatile i32 1033, ptr %p
  br label %j33
j33:
  br label %b34
b34:
  %c34 = icmp sgt i32 %n, 34
  br i1 %c34, label %t34, label %e34
t34:
  store volatile i32 34, ptr %p
  %d34 = icmp eq i32 %n, 241
  br i1 %d34, label %exit, label %j34
e34:
  store volatile i32 1034, ptr %p
  br label %j34
j34:
  br label %b35
b35:
  %c35 = icmp sgt i32 %n, 35
  br i1 %c35, label %t35, label %e35
t35:
  store volatile i32 35, ptr %p
  %d35 = icmp eq i32 %n, 248
  br i1 %d35, label %exit, label %j35
e35:
  store volatile i32 1035, ptr %p
  br label %j35
j35:
  br label %b36
b36:
  %c36 = icmp sgt i32 %n, 36
  br i1 %c36, label %t36, label %e36
t36:
  store volatile i32 36, ptr %p
  %d36 = icmp eq i32 %n, 255
  br i1 %d36, label %exit, label %j36
e36:
  store volatile i32 1036, ptr %p
  br label %j36
j36:
  br label %b37
b37:
  %c37 = icmp sgt i32 %n, 37
  br i1 %c37, label %t37, label %e37
t37:
  store volatile i32 37, ptr %p
  %d37 = icmp eq i32 %n, 262
  br i1 %d37, label %exit, label %j37
e37:
  store volatile i32 1037, ptr %p
  br label %j37
j37:
  br label %b38
b38:
  %c38 = icmp sgt i32 %n, 38
  br i1 %c38, label %t38, label %e38
t38:
  store volatile i32 38, ptr %p
  %d38 = icmp eq i32 %n, 269
  br i1 %d38, label %exit, label %j38
e38:
  store volatile i32 1038, ptr %p
  br label %j38
j38:
  br label %b39
b39:
  %c39 = icmp sgt i32 %n, 39
  br i1 %c39, label %t39, label %e39
t39:
  store volatile i32 39, ptr %p
  %d39 = icmp eq i32 %n, 276
  br i1 %d39, label %exit, label %j39
e39:
  store volatile i32 1039, ptr %p
  br label %j39
j39:
  br label %exit
exit:
  ret void
}
