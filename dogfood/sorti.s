	.text
	.file	"sorti.c"
	.section	.text.kernel_body,"ax",@progbits
	.globl	kernel_body             # -- Begin function kernel_body
	.p2align	2
	.type	kernel_body,@function
kernel_body:                            # @kernel_body
# %bb.0:                                # %entry
	addi	sp, sp, -16
	sw	a0, 12(sp)
	lw	a3, 0(a1)
	lw	a6, 4(a1)
	lw	a7, 8(a1)
	slli	a1, a0, 2
	add	a1, a3, a1
	lw	t0, 0(a1)
	mv	a5, zero
	addi	a2, zero, 1
	blt	a7, a2, .LBB0_3
# %bb.1:                                # %for.body.preheader
	mv	a2, zero
.LBB0_2:                                # %for.body
                                        # =>This Inner Loop Header: Depth=1
	lw	a4, 0(a3)
	slt	t1, a4, t0
	xor	a4, a4, t0
	seqz	a4, a4
	slt	a1, a2, a0
	and	a1, a1, a4
	or	a1, t1, a1
	add	a5, a5, a1
	addi	a2, a2, 1
	addi	a3, a3, 4
	bne	a7, a2, .LBB0_2
.LBB0_3:                                # %for.cond.cleanup
	slli	a0, a5, 2
	add	a0, a6, a0
	sw	t0, 0(a0)
	addi	sp, sp, 16
	ret
.Lfunc_end0:
	.size	kernel_body, .Lfunc_end0-kernel_body
                                        # -- End function
	.section	.text.main,"ax",@progbits
	.globl	main                    # -- Begin function main
	.p2align	2
	.type	main,@function
main:                                   # @main
# %bb.0:                                # %if.end23.15
	addi	sp, sp, -48
	sw	ra, 44(sp)
	sw	s0, 40(sp)
	sw	s1, 36(sp)
	sw	s2, 32(sp)
	sw	s3, 28(sp)
	sw	s4, 24(sp)
	call	rand
	lui	a1, 67109
	addi	s0, a1, -557
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	addi	s3, zero, 1000
	mul	a1, a1, s3
	sub	a0, a0, a1
	lui	s2, %hi(bufIn)
	sw	a0, %lo(bufIn)(s2)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	addi	s1, s2, %lo(bufIn)
	sw	a0, 4(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 8(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 12(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 16(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 20(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 24(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 28(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 32(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 36(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 40(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 44(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 48(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 52(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 56(s1)
	call	rand
	mulh	a1, a0, s0
	srli	a2, a1, 31
	srai	a1, a1, 6
	add	a1, a1, a2
	mul	a1, a1, s3
	sub	a0, a0, a1
	sw	a0, 60(s1)
	lui	a0, %hi(.L.str.2)
	addi	a0, a0, %lo(.L.str.2)
	call	vx_printf
	lw	a1, %lo(bufIn)(s2)
	lui	a0, %hi(.L.str.4)
	addi	s0, a0, %lo(.L.str.4)
	mv	a0, s0
	call	vx_printf
	lui	a0, %hi(.L.str.3)
	addi	s3, a0, %lo(.L.str.3)
	mv	a0, s3
	call	vx_printf
	lw	a1, 4(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 8(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 12(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 16(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 20(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 24(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 28(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 32(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 36(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 40(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 44(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 48(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 52(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 56(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 60(s1)
	mv	a0, s0
	call	vx_printf
	lui	a0, %hi(.L.str.5)
	addi	s2, a0, %lo(.L.str.5)
	mv	a0, s2
	call	vx_printf
	sw	s1, 8(sp)
	lui	s4, %hi(bufOut)
	addi	s1, s4, %lo(bufOut)
	sw	s1, 12(sp)
	addi	a0, zero, 16
	sw	a0, 16(sp)
	lui	a0, %hi(kernel_body)
	addi	a1, a0, %lo(kernel_body)
	addi	a2, sp, 8
	addi	a0, zero, 16
	call	vx_spawn_tasks
	#APP
	fence iorw, iorw
	#NO_APP
	lui	a0, %hi(.L.str.6)
	addi	a0, a0, %lo(.L.str.6)
	call	vx_printf
	lw	a1, %lo(bufOut)(s4)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 4(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 8(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 12(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 16(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 20(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 24(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 28(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 32(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 36(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 40(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 44(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 48(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 52(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 56(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s3
	call	vx_printf
	lw	a1, 60(s1)
	mv	a0, s0
	call	vx_printf
	mv	a0, s2
	call	vx_printf
	mv	a0, zero
	lw	s4, 24(sp)
	lw	s3, 28(sp)
	lw	s2, 32(sp)
	lw	s1, 36(sp)
	lw	s0, 40(sp)
	lw	ra, 44(sp)
	addi	sp, sp, 48
	ret
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
                                        # -- End function
	.type	bufIn,@object           # @bufIn
	.comm	bufIn,64,4
	.type	.L.str.2,@object        # @.str.2
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.2:
	.asciz	"bufIn=["
	.size	.L.str.2, 8

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	", "
	.size	.L.str.3, 3

	.type	.L.str.4,@object        # @.str.4
.L.str.4:
	.asciz	"%d"
	.size	.L.str.4, 3

	.type	.L.str.5,@object        # @.str.5
.L.str.5:
	.asciz	"]\n"
	.size	.L.str.5, 3

	.type	bufOut,@object          # @bufOut
	.comm	bufOut,64,4
	.type	.L.str.6,@object        # @.str.6
.L.str.6:
	.asciz	"bufOut=["
	.size	.L.str.6, 9

	.ident	"clang version 10.0.1 (https://github.com/llvm/llvm-project.git 5007a8eeff505b498fff15646ee0763fc1a21fc8)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym kernel_body
	.addrsig_sym bufIn
	.addrsig_sym bufOut
