	.text
	.file	"sortf.c"
	.section	.text.kernel_body,"ax",@progbits
	.globl	kernel_body             # -- Begin function kernel_body
	.p2align	2
	.type	kernel_body,@function
kernel_body:                            # @kernel_body
# %bb.0:                                # %entry
	addi	sp, sp, -16
	sw	a0, 12(sp)
	lw	a3, 0(a1)
	lw	a7, 4(a1)
	lw	t0, 8(a1)
	slli	a2, a0, 2
	add	a2, a3, a2
	flw	ft0, 0(a2)
	mv	a4, zero
	addi	a2, zero, 1
	blt	t0, a2, .LBB0_6
# %bb.1:                                # %for.body.preheader
	mv	a5, zero
	csrr	a6, tmask
	j	.LBB0_3
.LBB0_2:                                # %join_stub
                                        #   in Loop: Header=BB0_3 Depth=1
	vx_join	
	add	a4, a4, a2
	addi	a5, a5, 1
	addi	a3, a3, 4
	beq	t0, a5, .LBB0_5
.LBB0_3:                                # %for.body
                                        # =>This Inner Loop Header: Depth=1
	flw	ft1, 0(a3)
	flt.s	a1, ft1, ft0
	vx_split	a1
	addi	a2, zero, 1
	bnez	a1, .LBB0_2
# %bb.4:                                # %lor.rhs
                                        #   in Loop: Header=BB0_3 Depth=1
	feq.s	a1, ft1, ft0
	slt	a2, a5, a0
	and	a2, a2, a1
	j	.LBB0_2
.LBB0_5:                                # %loop_exit_stub
	vx_tmc	a6
.LBB0_6:                                # %for.cond.cleanup
	slli	a0, a4, 2
	add	a0, a7, a0
	fsw	ft0, 0(a0)
	addi	sp, sp, 16
	ret
.Lfunc_end0:
	.size	kernel_body, .Lfunc_end0-kernel_body
                                        # -- End function
	.section	.sdata,"aw",@progbits
	.p2align	2               # -- Begin function main
.LCPI1_0:
	.word	805306368               # float 4.65661287E-10
	.section	.text.main,"ax",@progbits
	.globl	main
	.p2align	2
	.type	main,@function
main:                                   # @main
# %bb.0:                                # %if.end.15
	addi	sp, sp, -32
	sw	ra, 28(sp)
	sw	s0, 24(sp)
	sw	s1, 20(sp)
	sw	s2, 16(sp)
	fsw	fs0, 12(sp)
	call	rand
	lui	a1, %hi(.LCPI1_0)
	addi	a1, a1, %lo(.LCPI1_0)
	flw	fs0, 0(a1)
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	lui	s0, %hi(bufIn)
	fsw	ft0, %lo(bufIn)(s0)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	addi	s1, s0, %lo(bufIn)
	fsw	ft0, 4(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 8(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 12(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 16(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 20(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 24(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 28(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 32(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 36(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 40(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 44(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 48(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 52(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 56(s1)
	call	rand
	fcvt.s.w	ft0, a0
	fmul.s	ft0, ft0, fs0
	fsw	ft0, 60(s1)
	lui	a0, 818229
	addi	a0, a0, -128
	lui	s2, %hi(bufOut)
	sw	a0, %lo(bufOut)(s2)
	addi	s0, s2, %lo(bufOut)
	sw	a0, 4(s0)
	sw	a0, 8(s0)
	sw	a0, 12(s0)
	sw	a0, 16(s0)
	sw	a0, 20(s0)
	sw	a0, 24(s0)
	sw	a0, 28(s0)
	sw	a0, 32(s0)
	sw	a0, 36(s0)
	sw	a0, 40(s0)
	sw	a0, 44(s0)
	sw	a0, 48(s0)
	sw	a0, 52(s0)
	sw	a0, 56(s0)
	sw	a0, 60(s0)
	sw	s1, 0(sp)
	sw	s0, 4(sp)
	addi	a0, zero, 16
	sw	a0, 8(sp)
	lui	a0, %hi(kernel_body)
	addi	a1, a0, %lo(kernel_body)
	mv	a2, sp
	addi	a0, zero, 16
	call	vx_spawn_tasks
	lui	a0, %hi(.L.str.2)
	addi	a0, a0, %lo(.L.str.2)
	call	vx_printf
	flw	fa0, %lo(bufOut)(s2)
	call	__extendsfdf2
	mv	a2, a0
	lui	a0, %hi(.L.str.4)
	addi	s2, a0, %lo(.L.str.4)
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	lui	a0, %hi(.L.str.3)
	addi	s1, a0, %lo(.L.str.3)
	mv	a0, s1
	call	vx_printf
	flw	fa0, 4(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 8(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 12(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 16(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 20(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 24(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 28(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 32(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 36(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 40(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 44(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 48(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 52(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 56(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	mv	a0, s1
	call	vx_printf
	flw	fa0, 60(s0)
	call	__extendsfdf2
	mv	a2, a0
	mv	a0, s2
	mv	a3, a1
	call	vx_printf
	lui	a0, %hi(.L.str.5)
	addi	a0, a0, %lo(.L.str.5)
	call	vx_printf
	mv	a0, zero
	flw	fs0, 12(sp)
	lw	s2, 16(sp)
	lw	s1, 20(sp)
	lw	s0, 24(sp)
	lw	ra, 28(sp)
	addi	sp, sp, 32
	ret
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
                                        # -- End function
	.type	bufIn,@object           # @bufIn
	.comm	bufIn,64,4
	.type	bufOut,@object          # @bufOut
	.comm	bufOut,64,4
	.type	.L.str.2,@object        # @.str.2
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.2:
	.asciz	"bufOut=["
	.size	.L.str.2, 9

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	", "
	.size	.L.str.3, 3

	.type	.L.str.4,@object        # @.str.4
.L.str.4:
	.asciz	"%.6f"
	.size	.L.str.4, 5

	.type	.L.str.5,@object        # @.str.5
.L.str.5:
	.asciz	"]\n"
	.size	.L.str.5, 3

	.ident	"clang version 10.0.1 (https://github.com/llvm/llvm-project.git 10ecc4d02fa47986338a1bae3d9403f9e2b0a9eb)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym kernel_body
	.addrsig_sym bufIn
	.addrsig_sym bufOut
