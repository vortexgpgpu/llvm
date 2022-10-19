	.text
	.file	"dtoa.c"
	.globl	_dtoa_r                 # -- Begin function _dtoa_r
	.p2align	1
	.type	_dtoa_r,@function
_dtoa_r:                                # @_dtoa_r
# %bb.0:                                # %entry
	addi	sp, sp, -480
	sw	ra, 476(sp)
	sw	s0, 472(sp)
	addi	s0, sp, 480
	add	t0, zero, a2
	add	t1, zero, a1
	sw	a0, -16(s0)
	sw	a2, -20(s0)
	sw	a1, -24(s0)
	sw	a3, -28(s0)
	sw	a4, -32(s0)
	sw	a5, -36(s0)
	sw	a6, -40(s0)
	sw	a7, -44(s0)
	mv	a0, zero
	sw	a0, -188(s0)
	lw	a1, -24(s0)
	lw	a2, -20(s0)
	sw	a2, -140(s0)
	sw	a1, -144(s0)
	lw	a1, -16(s0)
	lw	a1, 56(a1)
	beq	a1, a0, .LBB0_2
	j	.LBB0_1
.LBB0_1:                                # %if.then
	lw	a0, -16(s0)
	lw	a1, 60(a0)
	lw	a0, 56(a0)
	sw	a1, 4(a0)
	lw	a0, -16(s0)
	lw	a1, 60(a0)
	addi	a2, zero, 1
	sll	a1, a2, a1
	lw	a0, 56(a0)
	sw	a1, 8(a0)
	lw	a0, -16(s0)
	lw	a1, 56(a0)
	call	_Bfree
	lw	a0, -16(s0)
	mv	a1, zero
	sw	a1, 56(a0)
	j	.LBB0_2
.LBB0_2:                                # %if.end
	lbu	a0, -137(s0)
	andi	a0, a0, 128
	mv	a1, zero
	beq	a0, a1, .LBB0_4
	j	.LBB0_3
.LBB0_3:                                # %if.then9
	lw	a0, -40(s0)
	addi	a1, zero, 1
	sw	a1, 0(a0)
	lw	a0, -140(s0)
	lui	a1, 524288
	addi	a1, a1, -1
	and	a0, a0, a1
	sw	a0, -140(s0)
	j	.LBB0_5
.LBB0_4:                                # %if.else
	lw	a0, -40(s0)
	mv	a1, zero
	sw	a1, 0(a0)
	j	.LBB0_5
.LBB0_5:                                # %if.end13
	lw	a0, -140(s0)
	lui	a1, 524032
	and	a0, a0, a1
	bne	a0, a1, .LBB0_16
	j	.LBB0_6
.LBB0_6:                                # %if.then17
	lw	a0, -36(s0)
	lui	a1, 2
	addi	a1, a1, 1807
	sw	a1, 0(a0)
	lw	a0, -144(s0)
	mv	a1, zero
	add	a2, zero, a1
	sw	a2, -236(s0)
	bne	a0, a1, .LBB0_8
	j	.LBB0_7
.LBB0_7:                                # %land.rhs
	lw	a0, -140(s0)
	lui	a1, 256
	addi	a1, a1, -1
	and	a0, a0, a1
	seqz	a0, a0
	sw	a0, -236(s0)
	j	.LBB0_8
.LBB0_8:                                # %land.end
	lw	a0, -236(s0)
	andi	a0, a0, 1
	lui	a1, %hi(.L.str.1)
	addi	a1, a1, %lo(.L.str.1)
	lui	a2, %hi(.L.str)
	addi	a2, a2, %lo(.L.str)
	mv	a3, zero
	sw	a1, -240(s0)
	sw	a3, -244(s0)
	sw	a2, -248(s0)
	bne	a0, a3, .LBB0_10
# %bb.9:                                # %land.end
	lw	a0, -240(s0)
	sw	a0, -248(s0)
.LBB0_10:                               # %land.end
	lw	a0, -248(s0)
	sw	a0, -212(s0)
	lw	a0, -44(s0)
	lw	a1, -244(s0)
	beq	a0, a1, .LBB0_15
	j	.LBB0_11
.LBB0_11:                               # %if.then26
	lw	a0, -212(s0)
	lbu	a0, 3(a0)
	mv	a1, zero
	beq	a0, a1, .LBB0_13
	j	.LBB0_12
.LBB0_12:                               # %cond.true
	lw	a0, -212(s0)
	addi	a0, a0, 8
	sw	a0, -252(s0)
	j	.LBB0_14
.LBB0_13:                               # %cond.false
	lw	a0, -212(s0)
	addi	a0, a0, 3
	sw	a0, -252(s0)
	j	.LBB0_14
.LBB0_14:                               # %cond.end
	lw	a0, -252(s0)
	lw	a1, -44(s0)
	sw	a0, 0(a1)
	j	.LBB0_15
.LBB0_15:                               # %if.end31
	lw	a0, -212(s0)
	sw	a0, -12(s0)
	j	.LBB0_292
.LBB0_16:                               # %if.end32
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	mv	a2, zero
	sw	a2, -256(s0)
	lw	a3, -256(s0)
	call	__nedf2
	lw	a1, -256(s0)
	bne	a0, a1, .LBB0_20
	j	.LBB0_17
.LBB0_17:                               # %if.then35
	lw	a0, -36(s0)
	addi	a1, zero, 1
	sw	a1, 0(a0)
	lui	a0, %hi(.L.str.2)
	addi	a0, a0, %lo(.L.str.2)
	sw	a0, -212(s0)
	lw	a0, -44(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_19
	j	.LBB0_18
.LBB0_18:                               # %if.then37
	lw	a0, -212(s0)
	addi	a0, a0, 1
	lw	a1, -44(s0)
	sw	a0, 0(a1)
	j	.LBB0_19
.LBB0_19:                               # %if.end39
	lw	a0, -212(s0)
	sw	a0, -12(s0)
	j	.LBB0_292
.LBB0_20:                               # %if.end40
	lw	a0, -16(s0)
	lw	a2, -140(s0)
	lw	a1, -144(s0)
	addi	a3, s0, -60
	addi	a4, s0, -48
	call	__d2b
	sw	a0, -176(s0)
	lw	a0, -140(s0)
	srli	a0, a0, 20
	andi	a0, a0, 2047
	sw	a0, -68(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_22
	j	.LBB0_21
.LBB0_21:                               # %if.then47
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	sw	a1, -148(s0)
	sw	a0, -152(s0)
	lw	a0, -148(s0)
	lui	a1, 256
	addi	a1, a1, -1
	and	a0, a0, a1
	sw	a0, -148(s0)
	lw	a0, -148(s0)
	lui	a1, 261888
	or	a0, a0, a1
	sw	a0, -148(s0)
	lw	a0, -68(s0)
	addi	a0, a0, -1023
	sw	a0, -68(s0)
	mv	a0, zero
	sw	a0, -168(s0)
	j	.LBB0_26
.LBB0_22:                               # %if.else55
	lw	a0, -48(s0)
	lw	a1, -60(s0)
	add	a0, a0, a1
	addi	a0, a0, 1074
	sw	a0, -68(s0)
	lw	a0, -68(s0)
	addi	a1, zero, 33
	blt	a0, a1, .LBB0_24
	j	.LBB0_23
.LBB0_23:                               # %cond.true59
	lw	a0, -140(s0)
	lw	a1, -68(s0)
	addi	a2, zero, 64
	sub	a2, a2, a1
	sll	a0, a0, a2
	lw	a2, -144(s0)
	addi	a1, a1, -32
	srl	a1, a2, a1
	or	a0, a0, a1
	sw	a0, -260(s0)
	j	.LBB0_25
.LBB0_24:                               # %cond.false69
	lw	a0, -144(s0)
	lw	a1, -68(s0)
	addi	a2, zero, 32
	sub	a1, a2, a1
	sll	a0, a0, a1
	sw	a0, -260(s0)
	j	.LBB0_25
.LBB0_25:                               # %cond.end74
	lw	a0, -260(s0)
	sw	a0, -172(s0)
	lw	a0, -172(s0)
	call	__floatunsidf
	sw	a1, -148(s0)
	sw	a0, -152(s0)
	lw	a0, -148(s0)
	lui	a1, 1040640
	add	a0, a0, a1
	sw	a0, -148(s0)
	lw	a0, -68(s0)
	addi	a0, a0, -1075
	sw	a0, -68(s0)
	addi	a0, zero, 1
	sw	a0, -168(s0)
	j	.LBB0_26
.LBB0_26:                               # %if.end82
	lw	a0, -152(s0)
	lw	a1, -148(s0)
	mv	a2, zero
	lui	a3, 786304
	sw	a2, -264(s0)
	call	__adddf3
	lui	a2, 407284
	addi	a2, a2, 865
	lui	a3, 261416
	addi	a3, a3, 1959
	call	__muldf3
	lui	a2, 570893
	addi	a2, a2, -1869
	lui	a3, 261225
	addi	a3, a3, -1496
	call	__adddf3
	lw	a2, -68(s0)
	sw	a0, -268(s0)
	add	a0, zero, a2
	sw	a1, -272(s0)
	call	__floatsidf
	lui	a2, 330232
	addi	a2, a2, -1541
	lui	a3, 261428
	addi	a3, a3, 1043
	call	__muldf3
	lw	a2, -268(s0)
	sw	a0, -276(s0)
	add	a0, zero, a2
	lw	a3, -272(s0)
	sw	a1, -280(s0)
	add	a1, zero, a3
	lw	a2, -276(s0)
	lw	a3, -280(s0)
	call	__adddf3
	sw	a1, -204(s0)
	sw	a0, -208(s0)
	lw	a0, -208(s0)
	lw	a1, -204(s0)
	call	__fixdfsi
	sw	a0, -96(s0)
	lw	a0, -208(s0)
	lw	a1, -204(s0)
	sw	a0, -284(s0)
	sw	a1, -288(s0)
	lw	a2, -264(s0)
	lw	a3, -264(s0)
	call	__ltdf2
	lw	a1, -96(s0)
	sw	a0, -292(s0)
	add	a0, zero, a1
	call	__floatsidf
	addi	a2, zero, -1
	lw	a3, -292(s0)
	sw	a1, -296(s0)
	sw	a0, -300(s0)
	blt	a2, a3, .LBB0_29
	j	.LBB0_27
.LBB0_27:                               # %if.end82
	lw	a0, -284(s0)
	lw	a1, -288(s0)
	lw	a2, -300(s0)
	lw	a3, -296(s0)
	call	__eqdf2
	mv	a1, zero
	beq	a0, a1, .LBB0_29
	j	.LBB0_28
.LBB0_28:                               # %if.then95
	lw	a0, -96(s0)
	addi	a0, a0, -1
	sw	a0, -96(s0)
	j	.LBB0_29
.LBB0_29:                               # %if.end96
	addi	a0, zero, 1
	sw	a0, -104(s0)
	lw	a0, -96(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_34
	j	.LBB0_30
.LBB0_30:                               # %land.lhs.true99
	lw	a0, -96(s0)
	addi	a1, zero, 22
	blt	a1, a0, .LBB0_34
	j	.LBB0_31
.LBB0_31:                               # %if.then102
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -96(s0)
	slli	a2, a2, 3
	lui	a3, %hi(__mprec_tens)
	addi	a3, a3, %lo(__mprec_tens)
	add	a2, a2, a3
	lw	a3, 0(a2)
	lw	a2, 4(a2)
	sw	a2, -304(s0)
	add	a2, zero, a3
	lw	a3, -304(s0)
	call	__ltdf2
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_33
	j	.LBB0_32
.LBB0_32:                               # %if.then107
	lw	a0, -96(s0)
	addi	a0, a0, -1
	sw	a0, -96(s0)
	j	.LBB0_33
.LBB0_33:                               # %if.end109
	mv	a0, zero
	sw	a0, -104(s0)
	j	.LBB0_34
.LBB0_34:                               # %if.end110
	lw	a0, -48(s0)
	lw	a1, -68(s0)
	not	a1, a1
	add	a0, a0, a1
	sw	a0, -88(s0)
	lw	a0, -88(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_36
	j	.LBB0_35
.LBB0_35:                               # %if.then115
	mv	a0, zero
	sw	a0, -52(s0)
	lw	a0, -88(s0)
	sw	a0, -120(s0)
	j	.LBB0_37
.LBB0_36:                               # %if.else116
	lw	a0, -88(s0)
	mv	a1, zero
	sub	a0, a1, a0
	sw	a0, -52(s0)
	sw	a1, -120(s0)
	j	.LBB0_37
.LBB0_37:                               # %if.end118
	lw	a0, -96(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_39
	j	.LBB0_38
.LBB0_38:                               # %if.then121
	mv	a0, zero
	sw	a0, -56(s0)
	lw	a0, -96(s0)
	sw	a0, -124(s0)
	lw	a0, -96(s0)
	lw	a1, -120(s0)
	add	a0, a0, a1
	sw	a0, -120(s0)
	j	.LBB0_40
.LBB0_39:                               # %if.else123
	lw	a0, -96(s0)
	lw	a1, -52(s0)
	sub	a0, a1, a0
	sw	a0, -52(s0)
	lw	a0, -96(s0)
	mv	a1, zero
	sub	a0, a1, a0
	sw	a0, -56(s0)
	sw	a1, -124(s0)
	j	.LBB0_40
.LBB0_40:                               # %if.end126
	lw	a0, -28(s0)
	mv	a1, zero
	sw	a0, -308(s0)
	blt	a0, a1, .LBB0_42
	j	.LBB0_41
.LBB0_41:                               # %if.end126
	addi	a0, zero, 10
	lw	a1, -308(s0)
	blt	a1, a0, .LBB0_43
	j	.LBB0_42
.LBB0_42:                               # %if.then131
	mv	a0, zero
	sw	a0, -28(s0)
	j	.LBB0_43
.LBB0_43:                               # %if.end132
	addi	a0, zero, 1
	sw	a0, -132(s0)
	lw	a0, -28(s0)
	addi	a1, zero, 6
	blt	a0, a1, .LBB0_45
	j	.LBB0_44
.LBB0_44:                               # %if.then135
	lw	a0, -28(s0)
	addi	a0, a0, -4
	sw	a0, -28(s0)
	mv	a0, zero
	sw	a0, -132(s0)
	j	.LBB0_45
.LBB0_45:                               # %if.end137
	addi	a0, zero, 1
	sw	a0, -108(s0)
	addi	a0, zero, -1
	sw	a0, -84(s0)
	sw	a0, -76(s0)
	lw	a0, -28(s0)
	sw	a0, -312(s0)
	j	.LBB0_46
.LBB0_46:                               # %NodeBlock806
	addi	a0, zero, 3
	lw	a1, -312(s0)
	blt	a1, a0, .LBB0_50
	j	.LBB0_47
.LBB0_47:                               # %NodeBlock804
	addi	a0, zero, 4
	lw	a1, -312(s0)
	blt	a1, a0, .LBB0_57
	j	.LBB0_48
.LBB0_48:                               # %NodeBlock802
	addi	a0, zero, 5
	lw	a1, -312(s0)
	blt	a1, a0, .LBB0_54
	j	.LBB0_49
.LBB0_49:                               # %LeafBlock800
	addi	a0, zero, 5
	lw	a1, -312(s0)
	beq	a1, a0, .LBB0_58
	j	.LBB0_61
.LBB0_50:                               # %NodeBlock
	addi	a0, zero, 1
	lw	a1, -312(s0)
	blt	a0, a1, .LBB0_53
	j	.LBB0_51
.LBB0_51:                               # %LeafBlock
	mv	a0, zero
	lw	a1, -312(s0)
	blt	a1, a0, .LBB0_61
	j	.LBB0_52
.LBB0_52:                               # %sw.bb
	addi	a0, zero, 18
	sw	a0, -68(s0)
	mv	a0, zero
	sw	a0, -32(s0)
	j	.LBB0_62
.LBB0_53:                               # %sw.bb138
	mv	a0, zero
	sw	a0, -108(s0)
	j	.LBB0_54
.LBB0_54:                               # %sw.bb139
	lw	a0, -32(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_56
	j	.LBB0_55
.LBB0_55:                               # %if.then142
	addi	a0, zero, 1
	sw	a0, -32(s0)
	j	.LBB0_56
.LBB0_56:                               # %if.end143
	lw	a0, -32(s0)
	sw	a0, -68(s0)
	sw	a0, -84(s0)
	sw	a0, -76(s0)
	j	.LBB0_62
.LBB0_57:                               # %sw.bb144
	mv	a0, zero
	sw	a0, -108(s0)
	j	.LBB0_58
.LBB0_58:                               # %sw.bb145
	lw	a0, -32(s0)
	lw	a1, -96(s0)
	add	a0, a0, a1
	addi	a0, a0, 1
	sw	a0, -68(s0)
	lw	a0, -68(s0)
	sw	a0, -76(s0)
	lw	a0, -68(s0)
	addi	a0, a0, -1
	sw	a0, -84(s0)
	lw	a0, -68(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_60
	j	.LBB0_59
.LBB0_59:                               # %if.then151
	addi	a0, zero, 1
	sw	a0, -68(s0)
	j	.LBB0_60
.LBB0_60:                               # %if.end152
	j	.LBB0_62
.LBB0_61:                               # %NewDefault
	j	.LBB0_62
.LBB0_62:                               # %sw.epilog
	addi	a0, zero, 4
	sw	a0, -88(s0)
	lw	a0, -16(s0)
	mv	a1, zero
	sw	a1, 60(a0)
	j	.LBB0_63
.LBB0_63:                               # %for.cond
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -88(s0)
	addi	a0, a0, 20
	lw	a1, -68(s0)
	bltu	a1, a0, .LBB0_66
	j	.LBB0_64
.LBB0_64:                               # %for.body
                                        #   in Loop: Header=BB0_63 Depth=1
	lw	a0, -16(s0)
	lw	a1, 60(a0)
	addi	a1, a1, 1
	sw	a1, 60(a0)
	j	.LBB0_65
.LBB0_65:                               # %for.inc
                                        #   in Loop: Header=BB0_63 Depth=1
	lw	a0, -88(s0)
	slli	a0, a0, 1
	sw	a0, -88(s0)
	j	.LBB0_63
.LBB0_66:                               # %for.end
	lw	a0, -16(s0)
	lw	a1, 60(a0)
	call	_Balloc
	sw	a0, -220(s0)
	lw	a0, -220(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_68
	j	.LBB0_67
.LBB0_67:                               # %if.then163
	lui	a0, %hi(.L.str.3)
	addi	a0, a0, %lo(.L.str.3)
	lui	a1, %hi(.L.str.4)
	addi	a3, a1, %lo(.L.str.4)
	addi	a1, zero, 426
	mv	a2, zero
	call	__assert_func
	j	.LBB0_293
.LBB0_68:                               # %if.end164
	lw	a0, -220(s0)
	sw	a0, -224(s0)
	lw	a0, -224(s0)
	lw	a1, -16(s0)
	sw	a0, 56(a1)
	lw	a0, -16(s0)
	lw	a0, 56(a0)
	sw	a0, -216(s0)
	sw	a0, -212(s0)
	lw	a0, -76(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_128
	j	.LBB0_69
.LBB0_69:                               # %land.lhs.true169
	lw	a0, -76(s0)
	addi	a1, zero, 14
	blt	a1, a0, .LBB0_128
	j	.LBB0_70
.LBB0_70:                               # %land.lhs.true172
	lw	a0, -132(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_128
	j	.LBB0_71
.LBB0_71:                               # %if.then174
	mv	a0, zero
	sw	a0, -68(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	sw	a1, -148(s0)
	sw	a0, -152(s0)
	lw	a0, -96(s0)
	sw	a0, -100(s0)
	lw	a0, -76(s0)
	sw	a0, -80(s0)
	addi	a0, zero, 2
	sw	a0, -72(s0)
	lw	a0, -96(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_81
	j	.LBB0_72
.LBB0_72:                               # %if.then179
	lw	a0, -96(s0)
	andi	a0, a0, 15
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_tens)
	addi	a1, a1, %lo(__mprec_tens)
	add	a0, a0, a1
	lw	a1, 0(a0)
	lw	a0, 4(a0)
	sw	a0, -204(s0)
	sw	a1, -208(s0)
	lw	a0, -96(s0)
	srai	a0, a0, 4
	sw	a0, -88(s0)
	lbu	a0, -88(s0)
	andi	a0, a0, 16
	mv	a1, zero
	beq	a0, a1, .LBB0_74
	j	.LBB0_73
.LBB0_73:                               # %if.then185
	lw	a0, -88(s0)
	andi	a0, a0, 15
	sw	a0, -88(s0)
	lui	a0, %hi(__mprec_bigtens)
	addi	a0, a0, %lo(__mprec_bigtens)
	lw	a2, 32(a0)
	lw	a3, 36(a0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	call	__divdf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -72(s0)
	addi	a0, a0, 1
	sw	a0, -72(s0)
	j	.LBB0_74
.LBB0_74:                               # %if.end189
	j	.LBB0_75
.LBB0_75:                               # %for.cond190
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -88(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_80
	j	.LBB0_76
.LBB0_76:                               # %for.body192
                                        #   in Loop: Header=BB0_75 Depth=1
	lbu	a0, -88(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB0_78
	j	.LBB0_77
.LBB0_77:                               # %if.then195
                                        #   in Loop: Header=BB0_75 Depth=1
	lw	a0, -72(s0)
	addi	a0, a0, 1
	sw	a0, -72(s0)
	lw	a0, -68(s0)
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_bigtens)
	addi	a1, a1, %lo(__mprec_bigtens)
	add	a0, a0, a1
	lw	a2, 0(a0)
	lw	a3, 4(a0)
	lw	a0, -208(s0)
	lw	a1, -204(s0)
	call	__muldf3
	sw	a1, -204(s0)
	sw	a0, -208(s0)
	j	.LBB0_78
.LBB0_78:                               # %if.end199
                                        #   in Loop: Header=BB0_75 Depth=1
	j	.LBB0_79
.LBB0_79:                               # %for.inc200
                                        #   in Loop: Header=BB0_75 Depth=1
	lw	a0, -88(s0)
	srai	a0, a0, 1
	sw	a0, -88(s0)
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	j	.LBB0_75
.LBB0_80:                               # %for.end203
	lw	a2, -208(s0)
	lw	a3, -204(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	call	__divdf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	j	.LBB0_90
.LBB0_81:                               # %if.else206
	lw	a0, -96(s0)
	mv	a1, zero
	sub	a0, a1, a0
	sw	a0, -92(s0)
	beq	a0, a1, .LBB0_89
	j	.LBB0_82
.LBB0_82:                               # %if.then210
	lw	a0, -92(s0)
	andi	a0, a0, 15
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_tens)
	addi	a1, a1, %lo(__mprec_tens)
	add	a0, a0, a1
	lw	a2, 0(a0)
	lw	a3, 4(a0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	call	__muldf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -92(s0)
	srai	a0, a0, 4
	sw	a0, -88(s0)
	j	.LBB0_83
.LBB0_83:                               # %for.cond216
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -88(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_88
	j	.LBB0_84
.LBB0_84:                               # %for.body218
                                        #   in Loop: Header=BB0_83 Depth=1
	lbu	a0, -88(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB0_86
	j	.LBB0_85
.LBB0_85:                               # %if.then221
                                        #   in Loop: Header=BB0_83 Depth=1
	lw	a0, -72(s0)
	addi	a0, a0, 1
	sw	a0, -72(s0)
	lw	a0, -68(s0)
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_bigtens)
	addi	a1, a1, %lo(__mprec_bigtens)
	add	a0, a0, a1
	lw	a2, 0(a0)
	lw	a3, 4(a0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	call	__muldf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	j	.LBB0_86
.LBB0_86:                               # %if.end226
                                        #   in Loop: Header=BB0_83 Depth=1
	j	.LBB0_87
.LBB0_87:                               # %for.inc227
                                        #   in Loop: Header=BB0_83 Depth=1
	lw	a0, -88(s0)
	srai	a0, a0, 1
	sw	a0, -88(s0)
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	j	.LBB0_83
.LBB0_88:                               # %for.end230
	j	.LBB0_89
.LBB0_89:                               # %if.end231
	j	.LBB0_90
.LBB0_90:                               # %if.end232
	lw	a0, -104(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_96
	j	.LBB0_91
.LBB0_91:                               # %land.lhs.true234
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	mv	a2, zero
	lui	a3, 261888
	call	__ltdf2
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_96
	j	.LBB0_92
.LBB0_92:                               # %land.lhs.true238
	lw	a0, -76(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_96
	j	.LBB0_93
.LBB0_93:                               # %if.then241
	lw	a0, -84(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_95
	j	.LBB0_94
.LBB0_94:                               # %if.then244
	j	.LBB0_127
.LBB0_95:                               # %if.end245
	lw	a0, -84(s0)
	sw	a0, -76(s0)
	lw	a0, -96(s0)
	addi	a0, a0, -1
	sw	a0, -96(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	mv	a2, zero
	lui	a3, 262720
	call	__muldf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -72(s0)
	addi	a0, a0, 1
	sw	a0, -72(s0)
	j	.LBB0_96
.LBB0_96:                               # %if.end250
	lw	a0, -72(s0)
	call	__floatsidf
	lw	a2, -144(s0)
	lw	a3, -140(s0)
	call	__muldf3
	mv	a2, zero
	lui	a3, 262592
	sw	a2, -316(s0)
	call	__adddf3
	sw	a1, -156(s0)
	sw	a0, -160(s0)
	lw	a0, -156(s0)
	lui	a1, 1035264
	add	a0, a0, a1
	sw	a0, -156(s0)
	lw	a0, -76(s0)
	lw	a1, -316(s0)
	bne	a0, a1, .LBB0_102
	j	.LBB0_97
.LBB0_97:                               # %if.then261
	mv	a0, zero
	sw	a0, -192(s0)
	sw	a0, -196(s0)
	lw	a1, -144(s0)
	lw	a2, -140(s0)
	lui	a3, 786752
	sw	a0, -320(s0)
	add	a0, zero, a1
	add	a1, zero, a2
	lw	a2, -320(s0)
	call	__adddf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	call	__gtdf2
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_99
	j	.LBB0_98
.LBB0_98:                               # %if.then268
	j	.LBB0_217
.LBB0_99:                               # %if.end269
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	lui	a4, 524288
	xor	a3, a3, a4
	call	__ltdf2
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_101
	j	.LBB0_100
.LBB0_100:                              # %if.then274
	j	.LBB0_215
.LBB0_101:                              # %if.end275
	j	.LBB0_127
.LBB0_102:                              # %if.end276
	lw	a0, -108(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_112
	j	.LBB0_103
.LBB0_103:                              # %if.then278
	lw	a0, -76(s0)
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_tens)
	addi	a1, a1, %lo(__mprec_tens)
	add	a0, a0, a1
	lw	a2, -8(a0)
	lw	a3, -4(a0)
	mv	a0, zero
	lui	a1, 261632
	sw	a0, -324(s0)
	call	__divdf3
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	call	__subdf3
	sw	a1, -156(s0)
	sw	a0, -160(s0)
	lw	a0, -324(s0)
	sw	a0, -68(s0)
	j	.LBB0_104
.LBB0_104:                              # %for.cond285
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	call	__fixdfsi
	sw	a0, -164(s0)
	lw	a0, -164(s0)
	call	__floatsidf
	lw	a2, -144(s0)
	lw	a3, -140(s0)
	sw	a0, -328(s0)
	add	a0, zero, a2
	sw	a1, -332(s0)
	add	a1, zero, a3
	lw	a2, -328(s0)
	lw	a3, -332(s0)
	call	__subdf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -164(s0)
	addi	a0, a0, 48
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	call	__ltdf2
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_106
	j	.LBB0_105
.LBB0_105:                              # %if.then297
	j	.LBB0_289
.LBB0_106:                              # %if.end298
                                        #   in Loop: Header=BB0_104 Depth=1
	lw	a2, -144(s0)
	lw	a3, -140(s0)
	mv	a0, zero
	lui	a1, 261888
	call	__subdf3
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	call	__ltdf2
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_108
	j	.LBB0_107
.LBB0_107:                              # %if.then304
	j	.LBB0_142
.LBB0_108:                              # %if.end305
                                        #   in Loop: Header=BB0_104 Depth=1
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	lw	a1, -76(s0)
	blt	a0, a1, .LBB0_110
	j	.LBB0_109
.LBB0_109:                              # %if.then309
	j	.LBB0_111
.LBB0_110:                              # %if.end310
                                        #   in Loop: Header=BB0_104 Depth=1
	lw	a0, -160(s0)
	lw	a1, -156(s0)
	mv	a2, zero
	lui	a3, 262720
	sw	a2, -336(s0)
	sw	a3, -340(s0)
	call	__muldf3
	sw	a1, -156(s0)
	sw	a0, -160(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -336(s0)
	lw	a3, -340(s0)
	call	__muldf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	j	.LBB0_104
.LBB0_111:                              # %for.end315
	j	.LBB0_126
.LBB0_112:                              # %if.else316
	lw	a0, -76(s0)
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_tens)
	addi	a1, a1, %lo(__mprec_tens)
	add	a0, a0, a1
	lw	a2, -8(a0)
	lw	a3, -4(a0)
	lw	a0, -160(s0)
	lw	a1, -156(s0)
	call	__muldf3
	sw	a1, -156(s0)
	sw	a0, -160(s0)
	addi	a0, zero, 1
	sw	a0, -68(s0)
	j	.LBB0_113
.LBB0_113:                              # %for.cond321
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	call	__fixdfsi
	sw	a0, -164(s0)
	lw	a0, -164(s0)
	call	__floatsidf
	lw	a2, -144(s0)
	lw	a3, -140(s0)
	sw	a0, -344(s0)
	add	a0, zero, a2
	sw	a1, -348(s0)
	add	a1, zero, a3
	lw	a2, -344(s0)
	lw	a3, -348(s0)
	call	__subdf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -164(s0)
	addi	a0, a0, 48
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	lw	a0, -68(s0)
	lw	a1, -76(s0)
	bne	a0, a1, .LBB0_123
	j	.LBB0_114
.LBB0_114:                              # %if.then332
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	mv	a4, zero
	lui	a5, 261632
	sw	a0, -352(s0)
	add	a0, zero, a2
	sw	a1, -356(s0)
	add	a1, zero, a3
	add	a2, zero, a4
	add	a3, zero, a5
	call	__adddf3
	lw	a2, -352(s0)
	sw	a0, -360(s0)
	add	a0, zero, a2
	lw	a3, -356(s0)
	sw	a1, -364(s0)
	add	a1, zero, a3
	lw	a2, -360(s0)
	lw	a3, -364(s0)
	call	__gtdf2
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_116
	j	.LBB0_115
.LBB0_115:                              # %if.then338
	j	.LBB0_142
.LBB0_116:                              # %if.else339
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -160(s0)
	lw	a3, -156(s0)
	mv	a4, zero
	lui	a5, 261632
	sw	a0, -368(s0)
	add	a0, zero, a4
	sw	a1, -372(s0)
	add	a1, zero, a5
	call	__subdf3
	lw	a2, -368(s0)
	sw	a0, -376(s0)
	add	a0, zero, a2
	lw	a3, -372(s0)
	sw	a1, -380(s0)
	add	a1, zero, a3
	lw	a2, -376(s0)
	lw	a3, -380(s0)
	call	__ltdf2
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_121
	j	.LBB0_117
.LBB0_117:                              # %if.then345
	j	.LBB0_118
.LBB0_118:                              # %while.cond
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -212(s0)
	addi	a1, a0, -1
	sw	a1, -212(s0)
	lbu	a0, -1(a0)
	addi	a1, zero, 48
	bne	a0, a1, .LBB0_120
	j	.LBB0_119
.LBB0_119:                              # %while.body
                                        #   in Loop: Header=BB0_118 Depth=1
	j	.LBB0_118
.LBB0_120:                              # %while.end
	lw	a0, -212(s0)
	addi	a0, a0, 1
	sw	a0, -212(s0)
	j	.LBB0_289
.LBB0_121:                              # %if.end351
	j	.LBB0_122
.LBB0_122:                              # %if.end352
	j	.LBB0_125
.LBB0_123:                              # %if.end353
                                        #   in Loop: Header=BB0_113 Depth=1
	j	.LBB0_124
.LBB0_124:                              # %for.inc354
                                        #   in Loop: Header=BB0_113 Depth=1
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	mv	a2, zero
	lui	a3, 262720
	call	__muldf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	j	.LBB0_113
.LBB0_125:                              # %for.end358
	j	.LBB0_126
.LBB0_126:                              # %if.end359
	j	.LBB0_127
.LBB0_127:                              # %fast_failed
	lw	a0, -216(s0)
	sw	a0, -212(s0)
	lw	a0, -152(s0)
	lw	a1, -148(s0)
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -100(s0)
	sw	a0, -96(s0)
	lw	a0, -80(s0)
	sw	a0, -76(s0)
	j	.LBB0_128
.LBB0_128:                              # %if.end362
	lw	a0, -60(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_154
	j	.LBB0_129
.LBB0_129:                              # %land.lhs.true365
	lw	a0, -96(s0)
	addi	a1, zero, 14
	blt	a1, a0, .LBB0_154
	j	.LBB0_130
.LBB0_130:                              # %if.then368
	lw	a0, -96(s0)
	slli	a0, a0, 3
	lui	a1, %hi(__mprec_tens)
	addi	a1, a1, %lo(__mprec_tens)
	add	a0, a0, a1
	lw	a1, 0(a0)
	lw	a0, 4(a0)
	sw	a0, -204(s0)
	sw	a1, -208(s0)
	lw	a0, -32(s0)
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_136
	j	.LBB0_131
.LBB0_131:                              # %land.lhs.true372
	lw	a0, -76(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_136
	j	.LBB0_132
.LBB0_132:                              # %if.then375
	mv	a0, zero
	sw	a0, -192(s0)
	sw	a0, -196(s0)
	lw	a1, -76(s0)
	blt	a1, a0, .LBB0_134
	j	.LBB0_133
.LBB0_133:                              # %lor.lhs.false378
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -208(s0)
	lw	a3, -204(s0)
	mv	a4, zero
	lui	a5, 262464
	sw	a0, -384(s0)
	add	a0, zero, a2
	sw	a1, -388(s0)
	add	a1, zero, a3
	add	a2, zero, a4
	add	a3, zero, a5
	sw	a4, -392(s0)
	call	__muldf3
	lw	a2, -384(s0)
	sw	a0, -396(s0)
	add	a0, zero, a2
	lw	a3, -388(s0)
	sw	a1, -400(s0)
	add	a1, zero, a3
	lw	a2, -396(s0)
	lw	a3, -400(s0)
	call	__ledf2
	lw	a1, -392(s0)
	blt	a1, a0, .LBB0_135
	j	.LBB0_134
.LBB0_134:                              # %if.then383
	j	.LBB0_215
.LBB0_135:                              # %if.end384
	j	.LBB0_217
.LBB0_136:                              # %if.end385
	addi	a0, zero, 1
	sw	a0, -68(s0)
	j	.LBB0_137
.LBB0_137:                              # %for.cond386
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -208(s0)
	lw	a3, -204(s0)
	call	__divdf3
	call	__fixdfsi
	sw	a0, -164(s0)
	lw	a0, -164(s0)
	call	__floatsidf
	lw	a2, -208(s0)
	lw	a3, -204(s0)
	call	__muldf3
	lw	a2, -144(s0)
	lw	a3, -140(s0)
	sw	a0, -404(s0)
	add	a0, zero, a2
	sw	a1, -408(s0)
	add	a1, zero, a3
	lw	a2, -404(s0)
	lw	a3, -408(s0)
	call	__subdf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -164(s0)
	addi	a0, a0, 48
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	lw	a0, -68(s0)
	lw	a1, -76(s0)
	bne	a0, a1, .LBB0_149
	j	.LBB0_138
.LBB0_138:                              # %if.then399
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	sw	a0, -412(s0)
	sw	a1, -416(s0)
	lw	a2, -412(s0)
	lw	a3, -416(s0)
	call	__adddf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -208(s0)
	lw	a3, -204(s0)
	call	__gtdf2
	mv	a1, zero
	blt	a1, a0, .LBB0_141
	j	.LBB0_139
.LBB0_139:                              # %lor.lhs.false406
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	lw	a2, -208(s0)
	lw	a3, -204(s0)
	call	__nedf2
	mv	a1, zero
	bne	a0, a1, .LBB0_148
	j	.LBB0_140
.LBB0_140:                              # %land.lhs.true410
	lbu	a0, -164(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB0_148
	j	.LBB0_141
.LBB0_141:                              # %if.then413
	j	.LBB0_142
.LBB0_142:                              # %bump_up
	j	.LBB0_143
.LBB0_143:                              # %while.cond414
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -212(s0)
	addi	a1, a0, -1
	sw	a1, -212(s0)
	lbu	a0, -1(a0)
	addi	a1, zero, 57
	bne	a0, a1, .LBB0_147
	j	.LBB0_144
.LBB0_144:                              # %while.body419
                                        #   in Loop: Header=BB0_143 Depth=1
	lw	a0, -212(s0)
	lw	a1, -216(s0)
	bne	a0, a1, .LBB0_146
	j	.LBB0_145
.LBB0_145:                              # %if.then422
	lw	a0, -96(s0)
	addi	a0, a0, 1
	sw	a0, -96(s0)
	lw	a0, -212(s0)
	addi	a1, zero, 48
	sb	a1, 0(a0)
	j	.LBB0_147
.LBB0_146:                              # %if.end424
                                        #   in Loop: Header=BB0_143 Depth=1
	j	.LBB0_143
.LBB0_147:                              # %while.end425
	lw	a0, -212(s0)
	addi	a1, a0, 1
	sw	a1, -212(s0)
	lb	a1, 0(a0)
	addi	a1, a1, 1
	sb	a1, 0(a0)
	j	.LBB0_148
.LBB0_148:                              # %if.end428
	j	.LBB0_153
.LBB0_149:                              # %if.end429
                                        #   in Loop: Header=BB0_137 Depth=1
	lw	a0, -144(s0)
	lw	a1, -140(s0)
	mv	a2, zero
	lui	a3, 262720
	sw	a2, -420(s0)
	call	__muldf3
	sw	a1, -140(s0)
	sw	a0, -144(s0)
	lw	a2, -420(s0)
	lw	a3, -420(s0)
	call	__nedf2
	lw	a1, -420(s0)
	bne	a0, a1, .LBB0_151
	j	.LBB0_150
.LBB0_150:                              # %if.then433
	j	.LBB0_153
.LBB0_151:                              # %if.end434
                                        #   in Loop: Header=BB0_137 Depth=1
	j	.LBB0_152
.LBB0_152:                              # %for.inc435
                                        #   in Loop: Header=BB0_137 Depth=1
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	j	.LBB0_137
.LBB0_153:                              # %for.end437
	j	.LBB0_289
.LBB0_154:                              # %if.end438
	lw	a0, -52(s0)
	sw	a0, -112(s0)
	lw	a0, -56(s0)
	sw	a0, -116(s0)
	mv	a0, zero
	sw	a0, -188(s0)
	sw	a0, -192(s0)
	lw	a1, -108(s0)
	beq	a1, a0, .LBB0_167
	j	.LBB0_155
.LBB0_155:                              # %if.then440
	lw	a0, -28(s0)
	addi	a1, zero, 1
	blt	a1, a0, .LBB0_160
	j	.LBB0_156
.LBB0_156:                              # %if.then443
	lw	a0, -168(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_158
	j	.LBB0_157
.LBB0_157:                              # %cond.true445
	lw	a0, -60(s0)
	addi	a0, a0, 1075
	sw	a0, -424(s0)
	j	.LBB0_159
.LBB0_158:                              # %cond.false447
	lw	a0, -48(s0)
	addi	a1, zero, 54
	sub	a0, a1, a0
	sw	a0, -424(s0)
	j	.LBB0_159
.LBB0_159:                              # %cond.end449
	lw	a0, -424(s0)
	sw	a0, -68(s0)
	j	.LBB0_166
.LBB0_160:                              # %if.else451
	lw	a0, -76(s0)
	addi	a0, a0, -1
	sw	a0, -88(s0)
	lw	a0, -116(s0)
	lw	a1, -88(s0)
	blt	a0, a1, .LBB0_162
	j	.LBB0_161
.LBB0_161:                              # %if.then455
	lw	a0, -88(s0)
	lw	a1, -116(s0)
	sub	a0, a1, a0
	sw	a0, -116(s0)
	j	.LBB0_163
.LBB0_162:                              # %if.else457
	lw	a0, -116(s0)
	lw	a1, -88(s0)
	sub	a0, a1, a0
	sw	a0, -88(s0)
	lw	a1, -124(s0)
	add	a0, a0, a1
	sw	a0, -124(s0)
	lw	a0, -88(s0)
	lw	a1, -56(s0)
	add	a0, a0, a1
	sw	a0, -56(s0)
	mv	a0, zero
	sw	a0, -116(s0)
	j	.LBB0_163
.LBB0_163:                              # %if.end461
	lw	a0, -76(s0)
	sw	a0, -68(s0)
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_165
	j	.LBB0_164
.LBB0_164:                              # %if.then464
	lw	a0, -68(s0)
	lw	a1, -112(s0)
	sub	a0, a1, a0
	sw	a0, -112(s0)
	mv	a0, zero
	sw	a0, -68(s0)
	j	.LBB0_165
.LBB0_165:                              # %if.end466
	j	.LBB0_166
.LBB0_166:                              # %if.end467
	lw	a0, -68(s0)
	lw	a1, -52(s0)
	add	a0, a0, a1
	sw	a0, -52(s0)
	lw	a0, -68(s0)
	lw	a1, -120(s0)
	add	a0, a0, a1
	sw	a0, -120(s0)
	lw	a0, -16(s0)
	addi	a1, zero, 1
	call	__i2b
	sw	a0, -192(s0)
	j	.LBB0_167
.LBB0_167:                              # %if.end471
	lw	a0, -112(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_173
	j	.LBB0_168
.LBB0_168:                              # %land.lhs.true474
	lw	a0, -120(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_173
	j	.LBB0_169
.LBB0_169:                              # %if.then477
	lw	a0, -112(s0)
	lw	a1, -120(s0)
	bge	a0, a1, .LBB0_171
	j	.LBB0_170
.LBB0_170:                              # %cond.true480
	lw	a0, -112(s0)
	sw	a0, -428(s0)
	j	.LBB0_172
.LBB0_171:                              # %cond.false481
	lw	a0, -120(s0)
	sw	a0, -428(s0)
	j	.LBB0_172
.LBB0_172:                              # %cond.end482
	lw	a0, -428(s0)
	sw	a0, -68(s0)
	lw	a0, -68(s0)
	lw	a1, -52(s0)
	sub	a0, a1, a0
	sw	a0, -52(s0)
	lw	a0, -68(s0)
	lw	a1, -112(s0)
	sub	a0, a1, a0
	sw	a0, -112(s0)
	lw	a0, -68(s0)
	lw	a1, -120(s0)
	sub	a0, a1, a0
	sw	a0, -120(s0)
	j	.LBB0_173
.LBB0_173:                              # %if.end487
	lw	a0, -56(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_182
	j	.LBB0_174
.LBB0_174:                              # %if.then490
	lw	a0, -108(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_180
	j	.LBB0_175
.LBB0_175:                              # %if.then492
	lw	a0, -116(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_177
	j	.LBB0_176
.LBB0_176:                              # %if.then495
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	lw	a2, -116(s0)
	call	__pow5mult
	sw	a0, -192(s0)
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	lw	a2, -176(s0)
	call	__multiply
	sw	a0, -180(s0)
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	call	_Bfree
	lw	a0, -180(s0)
	sw	a0, -176(s0)
	j	.LBB0_177
.LBB0_177:                              # %if.end498
	lw	a0, -56(s0)
	lw	a1, -116(s0)
	sub	a0, a0, a1
	sw	a0, -88(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_179
	j	.LBB0_178
.LBB0_178:                              # %if.then502
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	lw	a2, -88(s0)
	call	__pow5mult
	sw	a0, -176(s0)
	j	.LBB0_179
.LBB0_179:                              # %if.end504
	j	.LBB0_181
.LBB0_180:                              # %if.else505
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	lw	a2, -56(s0)
	call	__pow5mult
	sw	a0, -176(s0)
	j	.LBB0_181
.LBB0_181:                              # %if.end507
	j	.LBB0_182
.LBB0_182:                              # %if.end508
	lw	a0, -16(s0)
	addi	a1, zero, 1
	sw	a1, -432(s0)
	call	__i2b
	sw	a0, -196(s0)
	lw	a0, -124(s0)
	lw	a1, -432(s0)
	blt	a0, a1, .LBB0_184
	j	.LBB0_183
.LBB0_183:                              # %if.then512
	lw	a0, -16(s0)
	lw	a1, -196(s0)
	lw	a2, -124(s0)
	call	__pow5mult
	sw	a0, -196(s0)
	j	.LBB0_184
.LBB0_184:                              # %if.end514
	mv	a0, zero
	sw	a0, -128(s0)
	lw	a0, -28(s0)
	addi	a1, zero, 1
	blt	a1, a0, .LBB0_190
	j	.LBB0_185
.LBB0_185:                              # %if.then517
	lw	a0, -144(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_189
	j	.LBB0_186
.LBB0_186:                              # %land.lhs.true521
	lw	a0, -140(s0)
	lui	a1, 256
	addi	a1, a1, -1
	and	a0, a0, a1
	mv	a1, zero
	bne	a0, a1, .LBB0_189
	j	.LBB0_187
.LBB0_187:                              # %land.lhs.true526
	lhu	a0, -138(s0)
	lui	a1, 8
	addi	a1, a1, -16
	and	a0, a0, a1
	mv	a1, zero
	beq	a0, a1, .LBB0_189
	j	.LBB0_188
.LBB0_188:                              # %if.then531
	lw	a0, -52(s0)
	addi	a0, a0, 1
	sw	a0, -52(s0)
	lw	a0, -120(s0)
	addi	a0, a0, 1
	sw	a0, -120(s0)
	addi	a0, zero, 1
	sw	a0, -128(s0)
	j	.LBB0_189
.LBB0_189:                              # %if.end534
	j	.LBB0_190
.LBB0_190:                              # %if.end535
	lw	a0, -124(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_192
	j	.LBB0_191
.LBB0_191:                              # %cond.true537
	lw	a0, -196(s0)
	lw	a1, 16(a0)
	slli	a1, a1, 2
	add	a0, a0, a1
	lw	a0, 16(a0)
	call	__hi0bits
	addi	a1, zero, 32
	sub	a0, a1, a0
	sw	a0, -436(s0)
	j	.LBB0_193
.LBB0_192:                              # %cond.false542
	addi	a0, zero, 1
	sw	a0, -436(s0)
	j	.LBB0_193
.LBB0_193:                              # %cond.end543
	lw	a0, -436(s0)
	lw	a1, -120(s0)
	add	a0, a0, a1
	andi	a0, a0, 31
	sw	a0, -68(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_195
	j	.LBB0_194
.LBB0_194:                              # %if.then549
	lw	a0, -68(s0)
	addi	a1, zero, 32
	sub	a0, a1, a0
	sw	a0, -68(s0)
	j	.LBB0_195
.LBB0_195:                              # %if.end551
	lw	a0, -68(s0)
	addi	a1, zero, 5
	blt	a0, a1, .LBB0_197
	j	.LBB0_196
.LBB0_196:                              # %if.then554
	lw	a0, -68(s0)
	addi	a0, a0, -4
	sw	a0, -68(s0)
	lw	a0, -68(s0)
	lw	a1, -52(s0)
	add	a0, a0, a1
	sw	a0, -52(s0)
	lw	a0, -68(s0)
	lw	a1, -112(s0)
	add	a0, a0, a1
	sw	a0, -112(s0)
	lw	a0, -68(s0)
	lw	a1, -120(s0)
	add	a0, a0, a1
	sw	a0, -120(s0)
	j	.LBB0_200
.LBB0_197:                              # %if.else559
	lw	a0, -68(s0)
	addi	a1, zero, 3
	blt	a1, a0, .LBB0_199
	j	.LBB0_198
.LBB0_198:                              # %if.then562
	lw	a0, -68(s0)
	addi	a0, a0, 28
	sw	a0, -68(s0)
	lw	a0, -68(s0)
	lw	a1, -52(s0)
	add	a0, a0, a1
	sw	a0, -52(s0)
	lw	a0, -68(s0)
	lw	a1, -112(s0)
	add	a0, a0, a1
	sw	a0, -112(s0)
	lw	a0, -68(s0)
	lw	a1, -120(s0)
	add	a0, a0, a1
	sw	a0, -120(s0)
	j	.LBB0_199
.LBB0_199:                              # %if.end567
	j	.LBB0_200
.LBB0_200:                              # %if.end568
	lw	a0, -52(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_202
	j	.LBB0_201
.LBB0_201:                              # %if.then571
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	lw	a2, -52(s0)
	call	__lshift
	sw	a0, -176(s0)
	j	.LBB0_202
.LBB0_202:                              # %if.end573
	lw	a0, -120(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_204
	j	.LBB0_203
.LBB0_203:                              # %if.then576
	lw	a0, -16(s0)
	lw	a1, -196(s0)
	lw	a2, -120(s0)
	call	__lshift
	sw	a0, -196(s0)
	j	.LBB0_204
.LBB0_204:                              # %if.end578
	lw	a0, -104(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_210
	j	.LBB0_205
.LBB0_205:                              # %if.then580
	lw	a0, -176(s0)
	lw	a1, -196(s0)
	call	__mcmp
	addi	a1, zero, -1
	blt	a1, a0, .LBB0_209
	j	.LBB0_206
.LBB0_206:                              # %if.then584
	lw	a0, -96(s0)
	addi	a0, a0, -1
	sw	a0, -96(s0)
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	addi	a2, zero, 10
	mv	a3, zero
	sw	a3, -440(s0)
	call	__multadd
	sw	a0, -176(s0)
	lw	a0, -108(s0)
	lw	a1, -440(s0)
	beq	a0, a1, .LBB0_208
	j	.LBB0_207
.LBB0_207:                              # %if.then588
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	addi	a2, zero, 10
	mv	a3, zero
	call	__multadd
	sw	a0, -192(s0)
	j	.LBB0_208
.LBB0_208:                              # %if.end590
	lw	a0, -84(s0)
	sw	a0, -76(s0)
	j	.LBB0_209
.LBB0_209:                              # %if.end591
	j	.LBB0_210
.LBB0_210:                              # %if.end592
	lw	a0, -76(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_218
	j	.LBB0_211
.LBB0_211:                              # %land.lhs.true595
	lw	a0, -28(s0)
	addi	a1, zero, 3
	blt	a0, a1, .LBB0_218
	j	.LBB0_212
.LBB0_212:                              # %if.then598
	lw	a0, -76(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_214
	j	.LBB0_213
.LBB0_213:                              # %lor.lhs.false601
	lw	a0, -176(s0)
	lw	a1, -16(s0)
	lw	a2, -196(s0)
	addi	a3, zero, 5
	mv	a4, zero
	sw	a0, -444(s0)
	add	a0, zero, a1
	add	a1, zero, a2
	add	a2, zero, a3
	add	a3, zero, a4
	sw	a4, -448(s0)
	call	__multadd
	sw	a0, -196(s0)
	lw	a1, -444(s0)
	sw	a0, -452(s0)
	add	a0, zero, a1
	lw	a1, -452(s0)
	call	__mcmp
	lw	a1, -448(s0)
	blt	a1, a0, .LBB0_216
	j	.LBB0_214
.LBB0_214:                              # %if.then606
	j	.LBB0_215
.LBB0_215:                              # %no_digits
	lw	a0, -32(s0)
	not	a0, a0
	sw	a0, -96(s0)
	j	.LBB0_283
.LBB0_216:                              # %if.end608
	j	.LBB0_217
.LBB0_217:                              # %one_digit
	lw	a0, -212(s0)
	addi	a1, a0, 1
	sw	a1, -212(s0)
	addi	a1, zero, 49
	sb	a1, 0(a0)
	lw	a0, -96(s0)
	addi	a0, a0, 1
	sw	a0, -96(s0)
	j	.LBB0_283
.LBB0_218:                              # %if.end611
	lw	a0, -108(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_262
	j	.LBB0_219
.LBB0_219:                              # %if.then613
	lw	a0, -112(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_221
	j	.LBB0_220
.LBB0_220:                              # %if.then616
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	lw	a2, -112(s0)
	call	__lshift
	sw	a0, -192(s0)
	j	.LBB0_221
.LBB0_221:                              # %if.end618
	lw	a0, -192(s0)
	sw	a0, -188(s0)
	lw	a0, -128(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_225
	j	.LBB0_222
.LBB0_222:                              # %if.then620
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	lw	a1, 4(a1)
	call	_Balloc
	sw	a0, -228(s0)
	lw	a0, -228(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_224
	j	.LBB0_223
.LBB0_223:                              # %if.then626
	lui	a0, %hi(.L.str.3)
	addi	a0, a0, %lo(.L.str.3)
	lui	a1, %hi(.L.str.4)
	addi	a3, a1, %lo(.L.str.4)
	addi	a1, zero, 746
	mv	a2, zero
	call	__assert_func
	j	.LBB0_293
.LBB0_224:                              # %if.end627
	lw	a0, -228(s0)
	sw	a0, -232(s0)
	lw	a0, -232(s0)
	sw	a0, -192(s0)
	lw	a0, -192(s0)
	addi	a0, a0, 12
	lw	a1, -188(s0)
	addi	a2, a1, 12
	lw	a1, 16(a1)
	slli	a1, a1, 2
	addi	a1, a1, 8
	sw	a1, -456(s0)
	add	a1, zero, a2
	lw	a2, -456(s0)
	call	memcpy
	lw	a1, -16(s0)
	lw	a2, -192(s0)
	addi	a3, zero, 1
	sw	a0, -460(s0)
	add	a0, zero, a1
	add	a1, zero, a2
	add	a2, zero, a3
	call	__lshift
	sw	a0, -192(s0)
	j	.LBB0_225
.LBB0_225:                              # %if.end635
	addi	a0, zero, 1
	sw	a0, -68(s0)
	j	.LBB0_226
.LBB0_226:                              # %for.cond636
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -176(s0)
	lw	a1, -196(s0)
	call	quorem
	addi	a0, a0, 48
	sw	a0, -64(s0)
	lw	a0, -176(s0)
	lw	a1, -188(s0)
	call	__mcmp
	sw	a0, -88(s0)
	lw	a0, -16(s0)
	lw	a1, -196(s0)
	lw	a2, -192(s0)
	call	__mdiff
	sw	a0, -184(s0)
	lw	a0, -184(s0)
	lw	a0, 12(a0)
	mv	a1, zero
	beq	a0, a1, .LBB0_228
	j	.LBB0_227
.LBB0_227:                              # %cond.true643
                                        #   in Loop: Header=BB0_226 Depth=1
	addi	a0, zero, 1
	sw	a0, -464(s0)
	j	.LBB0_229
.LBB0_228:                              # %cond.false644
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -176(s0)
	lw	a1, -184(s0)
	call	__mcmp
	sw	a0, -464(s0)
	j	.LBB0_229
.LBB0_229:                              # %cond.end646
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -464(s0)
	sw	a0, -92(s0)
	lw	a0, -16(s0)
	lw	a1, -184(s0)
	call	_Bfree
	lw	a0, -92(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_237
	j	.LBB0_230
.LBB0_230:                              # %land.lhs.true650
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -28(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_237
	j	.LBB0_231
.LBB0_231:                              # %land.lhs.true652
                                        #   in Loop: Header=BB0_226 Depth=1
	lbu	a0, -144(s0)
	andi	a0, a0, 1
	bnez	a0, .LBB0_237
	j	.LBB0_232
.LBB0_232:                              # %if.then657
	lw	a0, -64(s0)
	addi	a1, zero, 57
	bne	a0, a1, .LBB0_234
	j	.LBB0_233
.LBB0_233:                              # %if.then660
	j	.LBB0_252
.LBB0_234:                              # %if.end661
	lw	a0, -88(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_236
	j	.LBB0_235
.LBB0_235:                              # %if.then664
	lw	a0, -64(s0)
	addi	a0, a0, 1
	sw	a0, -64(s0)
	j	.LBB0_236
.LBB0_236:                              # %if.end666
	lw	a0, -64(s0)
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	j	.LBB0_283
.LBB0_237:                              # %if.end669
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -88(s0)
	mv	a1, zero
	blt	a0, a1, .LBB0_241
	j	.LBB0_238
.LBB0_238:                              # %lor.lhs.false672
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -88(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_249
	j	.LBB0_239
.LBB0_239:                              # %land.lhs.true675
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -28(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_249
	j	.LBB0_240
.LBB0_240:                              # %land.lhs.true677
                                        #   in Loop: Header=BB0_226 Depth=1
	lbu	a0, -144(s0)
	andi	a0, a0, 1
	bnez	a0, .LBB0_249
	j	.LBB0_241
.LBB0_241:                              # %if.then682
	lw	a0, -92(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_248
	j	.LBB0_242
.LBB0_242:                              # %if.then685
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	addi	a2, zero, 1
	call	__lshift
	sw	a0, -176(s0)
	lw	a0, -176(s0)
	lw	a1, -196(s0)
	call	__mcmp
	sw	a0, -92(s0)
	lw	a0, -92(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_245
	j	.LBB0_243
.LBB0_243:                              # %lor.lhs.false690
	lw	a0, -92(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_247
	j	.LBB0_244
.LBB0_244:                              # %land.lhs.true693
	lbu	a0, -64(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB0_247
	j	.LBB0_245
.LBB0_245:                              # %land.lhs.true696
	lw	a0, -64(s0)
	addi	a1, a0, 1
	sw	a1, -64(s0)
	addi	a1, zero, 57
	bne	a0, a1, .LBB0_247
	j	.LBB0_246
.LBB0_246:                              # %if.then700
	j	.LBB0_252
.LBB0_247:                              # %if.end701
	j	.LBB0_248
.LBB0_248:                              # %if.end702
	lw	a0, -64(s0)
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	j	.LBB0_283
.LBB0_249:                              # %if.end705
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -92(s0)
	addi	a1, zero, 1
	blt	a0, a1, .LBB0_254
	j	.LBB0_250
.LBB0_250:                              # %if.then708
	lw	a0, -64(s0)
	addi	a1, zero, 57
	bne	a0, a1, .LBB0_253
	j	.LBB0_251
.LBB0_251:                              # %if.then711
	j	.LBB0_252
.LBB0_252:                              # %round_9_up
	lw	a0, -212(s0)
	addi	a1, a0, 1
	sw	a1, -212(s0)
	addi	a1, zero, 57
	sb	a1, 0(a0)
	j	.LBB0_272
.LBB0_253:                              # %if.end713
	lw	a0, -64(s0)
	addi	a0, a0, 1
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	j	.LBB0_283
.LBB0_254:                              # %if.end717
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -64(s0)
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	lw	a0, -68(s0)
	lw	a1, -76(s0)
	bne	a0, a1, .LBB0_256
	j	.LBB0_255
.LBB0_255:                              # %if.then722
	j	.LBB0_261
.LBB0_256:                              # %if.end723
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	addi	a2, zero, 10
	mv	a3, zero
	call	__multadd
	sw	a0, -176(s0)
	lw	a0, -188(s0)
	lw	a1, -192(s0)
	bne	a0, a1, .LBB0_258
	j	.LBB0_257
.LBB0_257:                              # %if.then727
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	addi	a2, zero, 10
	mv	a3, zero
	call	__multadd
	sw	a0, -192(s0)
	sw	a0, -188(s0)
	j	.LBB0_259
.LBB0_258:                              # %if.else729
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -16(s0)
	lw	a1, -188(s0)
	addi	a2, zero, 10
	mv	a3, zero
	sw	a2, -468(s0)
	sw	a3, -472(s0)
	call	__multadd
	sw	a0, -188(s0)
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	lw	a2, -468(s0)
	lw	a3, -472(s0)
	call	__multadd
	sw	a0, -192(s0)
	j	.LBB0_259
.LBB0_259:                              # %if.end732
                                        #   in Loop: Header=BB0_226 Depth=1
	j	.LBB0_260
.LBB0_260:                              # %for.inc733
                                        #   in Loop: Header=BB0_226 Depth=1
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	j	.LBB0_226
.LBB0_261:                              # %for.end735
	j	.LBB0_268
.LBB0_262:                              # %if.else736
	addi	a0, zero, 1
	sw	a0, -68(s0)
	j	.LBB0_263
.LBB0_263:                              # %for.cond737
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -176(s0)
	lw	a1, -196(s0)
	call	quorem
	addi	a0, a0, 48
	sw	a0, -64(s0)
	lw	a1, -212(s0)
	addi	a2, a1, 1
	sw	a2, -212(s0)
	sb	a0, 0(a1)
	lw	a0, -68(s0)
	lw	a1, -76(s0)
	blt	a0, a1, .LBB0_265
	j	.LBB0_264
.LBB0_264:                              # %if.then744
	j	.LBB0_267
.LBB0_265:                              # %if.end745
                                        #   in Loop: Header=BB0_263 Depth=1
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	addi	a2, zero, 10
	mv	a3, zero
	call	__multadd
	sw	a0, -176(s0)
	j	.LBB0_266
.LBB0_266:                              # %for.inc747
                                        #   in Loop: Header=BB0_263 Depth=1
	lw	a0, -68(s0)
	addi	a0, a0, 1
	sw	a0, -68(s0)
	j	.LBB0_263
.LBB0_267:                              # %for.end749
	j	.LBB0_268
.LBB0_268:                              # %if.end750
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	addi	a2, zero, 1
	call	__lshift
	sw	a0, -176(s0)
	lw	a0, -176(s0)
	lw	a1, -196(s0)
	call	__mcmp
	sw	a0, -88(s0)
	lw	a0, -88(s0)
	mv	a1, zero
	blt	a1, a0, .LBB0_271
	j	.LBB0_269
.LBB0_269:                              # %lor.lhs.false755
	lw	a0, -88(s0)
	mv	a1, zero
	bne	a0, a1, .LBB0_278
	j	.LBB0_270
.LBB0_270:                              # %land.lhs.true758
	lbu	a0, -64(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB0_278
	j	.LBB0_271
.LBB0_271:                              # %if.then761
	j	.LBB0_272
.LBB0_272:                              # %roundoff
	j	.LBB0_273
.LBB0_273:                              # %while.cond762
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -212(s0)
	addi	a1, a0, -1
	sw	a1, -212(s0)
	lbu	a0, -1(a0)
	addi	a1, zero, 57
	bne	a0, a1, .LBB0_277
	j	.LBB0_274
.LBB0_274:                              # %while.body767
                                        #   in Loop: Header=BB0_273 Depth=1
	lw	a0, -212(s0)
	lw	a1, -216(s0)
	bne	a0, a1, .LBB0_276
	j	.LBB0_275
.LBB0_275:                              # %if.then770
	lw	a0, -96(s0)
	addi	a0, a0, 1
	sw	a0, -96(s0)
	lw	a0, -212(s0)
	addi	a1, a0, 1
	sw	a1, -212(s0)
	addi	a1, zero, 49
	sb	a1, 0(a0)
	j	.LBB0_283
.LBB0_276:                              # %if.end773
                                        #   in Loop: Header=BB0_273 Depth=1
	j	.LBB0_273
.LBB0_277:                              # %while.end774
	lw	a0, -212(s0)
	addi	a1, a0, 1
	sw	a1, -212(s0)
	lb	a1, 0(a0)
	addi	a1, a1, 1
	sb	a1, 0(a0)
	j	.LBB0_282
.LBB0_278:                              # %if.else777
	j	.LBB0_279
.LBB0_279:                              # %while.cond778
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -212(s0)
	addi	a1, a0, -1
	sw	a1, -212(s0)
	lbu	a0, -1(a0)
	addi	a1, zero, 48
	bne	a0, a1, .LBB0_281
	j	.LBB0_280
.LBB0_280:                              # %while.body783
                                        #   in Loop: Header=BB0_279 Depth=1
	j	.LBB0_279
.LBB0_281:                              # %while.end784
	lw	a0, -212(s0)
	addi	a0, a0, 1
	sw	a0, -212(s0)
	j	.LBB0_282
.LBB0_282:                              # %if.end786
	j	.LBB0_283
.LBB0_283:                              # %ret
	lw	a0, -16(s0)
	lw	a1, -196(s0)
	call	_Bfree
	lw	a0, -192(s0)
	mv	a1, zero
	beq	a0, a1, .LBB0_288
	j	.LBB0_284
.LBB0_284:                              # %if.then788
	lw	a0, -188(s0)
	lw	a1, -192(s0)
	mv	a2, zero
	sw	a0, -476(s0)
	sw	a1, -480(s0)
	beq	a0, a2, .LBB0_287
	j	.LBB0_285
.LBB0_285:                              # %if.then788
	lw	a0, -476(s0)
	lw	a1, -480(s0)
	beq	a0, a1, .LBB0_287
	j	.LBB0_286
.LBB0_286:                              # %if.then793
	lw	a0, -16(s0)
	lw	a1, -188(s0)
	call	_Bfree
	j	.LBB0_287
.LBB0_287:                              # %if.end794
	lw	a0, -16(s0)
	lw	a1, -192(s0)
	call	_Bfree
	j	.LBB0_288
.LBB0_288:                              # %if.end795
	j	.LBB0_289
.LBB0_289:                              # %ret1
	lw	a0, -16(s0)
	lw	a1, -176(s0)
	call	_Bfree
	lw	a0, -212(s0)
	mv	a1, zero
	sb	a1, 0(a0)
	lw	a0, -96(s0)
	addi	a0, a0, 1
	lw	a2, -36(s0)
	sw	a0, 0(a2)
	lw	a0, -44(s0)
	beq	a0, a1, .LBB0_291
	j	.LBB0_290
.LBB0_290:                              # %if.then798
	lw	a0, -212(s0)
	lw	a1, -44(s0)
	sw	a0, 0(a1)
	j	.LBB0_291
.LBB0_291:                              # %if.end799
	lw	a0, -216(s0)
	sw	a0, -12(s0)
	j	.LBB0_292
.LBB0_292:                              # %return
	lw	a0, -12(s0)
	lw	s0, 472(sp)
	lw	ra, 476(sp)
	addi	sp, sp, 480
	ret
.LBB0_293:                              # %UnifiedUnreachableBlock
.Lfunc_end0:
	.size	_dtoa_r, .Lfunc_end0-_dtoa_r
                                        # -- End function
	.p2align	1               # -- Begin function quorem
	.type	quorem,@function
quorem:                                 # @quorem
# %bb.0:                                # %entry
	addi	sp, sp, -80
	sw	ra, 76(sp)
	sw	s0, 72(sp)
	addi	s0, sp, 80
	sw	a0, -16(s0)
	sw	a1, -20(s0)
	lw	a0, -20(s0)
	lw	a0, 16(a0)
	sw	a0, -24(s0)
	lw	a0, -16(s0)
	lw	a0, 16(a0)
	lw	a1, -24(s0)
	bge	a0, a1, .LBB1_2
	j	.LBB1_1
.LBB1_1:                                # %if.then
	mv	a0, zero
	sw	a0, -12(s0)
	j	.LBB1_27
.LBB1_2:                                # %if.end
	lw	a0, -20(s0)
	addi	a0, a0, 20
	sw	a0, -56(s0)
	lw	a0, -56(s0)
	lw	a1, -24(s0)
	addi	a1, a1, -1
	sw	a1, -24(s0)
	slli	a1, a1, 2
	add	a0, a0, a1
	sw	a0, -60(s0)
	lw	a0, -16(s0)
	addi	a0, a0, 20
	sw	a0, -48(s0)
	lw	a0, -48(s0)
	lw	a1, -24(s0)
	slli	a1, a1, 2
	add	a0, a0, a1
	sw	a0, -52(s0)
	lw	a0, -52(s0)
	lw	a0, 0(a0)
	lw	a1, -60(s0)
	lw	a1, 0(a1)
	addi	a1, a1, 1
	divu	a0, a0, a1
	sw	a0, -40(s0)
	lw	a0, -40(s0)
	mv	a1, zero
	beq	a0, a1, .LBB1_14
	j	.LBB1_3
.LBB1_3:                                # %if.then5
	mv	a0, zero
	sw	a0, -28(s0)
	sw	a0, -36(s0)
	j	.LBB1_4
.LBB1_4:                                # %do.body
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -56(s0)
	addi	a1, a0, 4
	sw	a1, -56(s0)
	lw	a0, 0(a0)
	sw	a0, -68(s0)
	lhu	a0, -68(s0)
	lw	a1, -40(s0)
	mul	a0, a0, a1
	lw	a1, -36(s0)
	add	a0, a0, a1
	sw	a0, -44(s0)
	lhu	a0, -66(s0)
	lw	a1, -40(s0)
	mul	a0, a0, a1
	lhu	a1, -42(s0)
	add	a0, a0, a1
	sw	a0, -72(s0)
	lhu	a0, -70(s0)
	sw	a0, -36(s0)
	lw	a0, -48(s0)
	lhu	a0, 0(a0)
	lhu	a1, -44(s0)
	sub	a0, a0, a1
	lw	a1, -28(s0)
	add	a0, a0, a1
	sw	a0, -32(s0)
	lw	a0, -32(s0)
	srai	a0, a0, 16
	sw	a0, -28(s0)
	lw	a0, -48(s0)
	lhu	a0, 2(a0)
	lhu	a1, -72(s0)
	sub	a0, a0, a1
	lw	a1, -28(s0)
	add	a0, a0, a1
	sw	a0, -64(s0)
	lw	a0, -64(s0)
	srai	a0, a0, 16
	sw	a0, -28(s0)
	lw	a0, -64(s0)
	slli	a0, a0, 16
	lhu	a1, -32(s0)
	or	a0, a0, a1
	lw	a1, -48(s0)
	addi	a2, a1, 4
	sw	a2, -48(s0)
	sw	a0, 0(a1)
	j	.LBB1_5
.LBB1_5:                                # %do.cond
                                        #   in Loop: Header=BB1_4 Depth=1
	lw	a0, -56(s0)
	lw	a1, -60(s0)
	bgeu	a1, a0, .LBB1_4
	j	.LBB1_6
.LBB1_6:                                # %do.end
	lw	a0, -52(s0)
	lw	a0, 0(a0)
	mv	a1, zero
	bne	a0, a1, .LBB1_13
	j	.LBB1_7
.LBB1_7:                                # %if.then24
	lw	a0, -16(s0)
	addi	a0, a0, 20
	sw	a0, -48(s0)
	j	.LBB1_8
.LBB1_8:                                # %while.cond
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -52(s0)
	addi	a0, a0, -4
	sw	a0, -52(s0)
	lw	a1, -48(s0)
	mv	a2, zero
	sw	a2, -76(s0)
	bgeu	a1, a0, .LBB1_10
	j	.LBB1_9
.LBB1_9:                                # %land.rhs
                                        #   in Loop: Header=BB1_8 Depth=1
	lw	a0, -52(s0)
	lw	a0, 0(a0)
	seqz	a0, a0
	sw	a0, -76(s0)
	j	.LBB1_10
.LBB1_10:                               # %land.end
                                        #   in Loop: Header=BB1_8 Depth=1
	lw	a0, -76(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB1_12
	j	.LBB1_11
.LBB1_11:                               # %while.body
                                        #   in Loop: Header=BB1_8 Depth=1
	lw	a0, -24(s0)
	addi	a0, a0, -1
	sw	a0, -24(s0)
	j	.LBB1_8
.LBB1_12:                               # %while.end
	lw	a0, -24(s0)
	lw	a1, -16(s0)
	sw	a0, 16(a1)
	j	.LBB1_13
.LBB1_13:                               # %if.end32
	j	.LBB1_14
.LBB1_14:                               # %if.end33
	lw	a0, -16(s0)
	lw	a1, -20(s0)
	call	__mcmp
	mv	a1, zero
	blt	a0, a1, .LBB1_26
	j	.LBB1_15
.LBB1_15:                               # %if.then35
	lw	a0, -40(s0)
	addi	a0, a0, 1
	sw	a0, -40(s0)
	mv	a0, zero
	sw	a0, -28(s0)
	sw	a0, -36(s0)
	lw	a0, -16(s0)
	addi	a0, a0, 20
	sw	a0, -48(s0)
	lw	a0, -20(s0)
	addi	a0, a0, 20
	sw	a0, -56(s0)
	j	.LBB1_16
.LBB1_16:                               # %do.body40
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -56(s0)
	addi	a1, a0, 4
	sw	a1, -56(s0)
	lw	a0, 0(a0)
	sw	a0, -68(s0)
	lhu	a0, -68(s0)
	lw	a1, -36(s0)
	add	a0, a0, a1
	sw	a0, -44(s0)
	lhu	a0, -66(s0)
	lhu	a1, -42(s0)
	add	a0, a0, a1
	sw	a0, -72(s0)
	lhu	a0, -70(s0)
	sw	a0, -36(s0)
	lw	a0, -48(s0)
	lhu	a0, 0(a0)
	lhu	a1, -44(s0)
	sub	a0, a0, a1
	lw	a1, -28(s0)
	add	a0, a0, a1
	sw	a0, -32(s0)
	lw	a0, -32(s0)
	srai	a0, a0, 16
	sw	a0, -28(s0)
	lw	a0, -48(s0)
	lhu	a0, 2(a0)
	lhu	a1, -72(s0)
	sub	a0, a0, a1
	lw	a1, -28(s0)
	add	a0, a0, a1
	sw	a0, -64(s0)
	lw	a0, -64(s0)
	srai	a0, a0, 16
	sw	a0, -28(s0)
	lw	a0, -64(s0)
	slli	a0, a0, 16
	lhu	a1, -32(s0)
	or	a0, a0, a1
	lw	a1, -48(s0)
	addi	a2, a1, 4
	sw	a2, -48(s0)
	sw	a0, 0(a1)
	j	.LBB1_17
.LBB1_17:                               # %do.cond62
                                        #   in Loop: Header=BB1_16 Depth=1
	lw	a0, -56(s0)
	lw	a1, -60(s0)
	bgeu	a1, a0, .LBB1_16
	j	.LBB1_18
.LBB1_18:                               # %do.end64
	lw	a0, -16(s0)
	addi	a0, a0, 20
	sw	a0, -48(s0)
	lw	a0, -48(s0)
	lw	a1, -24(s0)
	slli	a1, a1, 2
	add	a0, a0, a1
	sw	a0, -52(s0)
	lw	a0, -52(s0)
	lw	a0, 0(a0)
	mv	a1, zero
	bne	a0, a1, .LBB1_25
	j	.LBB1_19
.LBB1_19:                               # %if.then69
	j	.LBB1_20
.LBB1_20:                               # %while.cond70
                                        # =>This Inner Loop Header: Depth=1
	lw	a0, -52(s0)
	addi	a0, a0, -4
	sw	a0, -52(s0)
	lw	a1, -48(s0)
	mv	a2, zero
	sw	a2, -80(s0)
	bgeu	a1, a0, .LBB1_22
	j	.LBB1_21
.LBB1_21:                               # %land.rhs73
                                        #   in Loop: Header=BB1_20 Depth=1
	lw	a0, -52(s0)
	lw	a0, 0(a0)
	seqz	a0, a0
	sw	a0, -80(s0)
	j	.LBB1_22
.LBB1_22:                               # %land.end76
                                        #   in Loop: Header=BB1_20 Depth=1
	lw	a0, -80(s0)
	andi	a0, a0, 1
	mv	a1, zero
	beq	a0, a1, .LBB1_24
	j	.LBB1_23
.LBB1_23:                               # %while.body77
                                        #   in Loop: Header=BB1_20 Depth=1
	lw	a0, -24(s0)
	addi	a0, a0, -1
	sw	a0, -24(s0)
	j	.LBB1_20
.LBB1_24:                               # %while.end79
	lw	a0, -24(s0)
	lw	a1, -16(s0)
	sw	a0, 16(a1)
	j	.LBB1_25
.LBB1_25:                               # %if.end81
	j	.LBB1_26
.LBB1_26:                               # %if.end82
	lw	a0, -40(s0)
	sw	a0, -12(s0)
	j	.LBB1_27
.LBB1_27:                               # %return
	lw	a0, -12(s0)
	lw	s0, 72(sp)
	lw	ra, 76(sp)
	addi	sp, sp, 80
	ret
.Lfunc_end1:
	.size	quorem, .Lfunc_end1-quorem
                                        # -- End function
	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"Infinity"
	.size	.L.str, 9

	.type	.L.str.1,@object        # @.str.1
.L.str.1:
	.asciz	"NaN"
	.size	.L.str.1, 4

	.type	.L.str.2,@object        # @.str.2
.L.str.2:
	.asciz	"0"
	.size	.L.str.2, 2

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	"/home/blaise/dev/newlib-cygwin/newlib/libc/stdlib/dtoa.c"
	.size	.L.str.3, 57

	.type	.L.str.4,@object        # @.str.4
.L.str.4:
	.asciz	"Balloc succeeded"
	.size	.L.str.4, 17

	.ident	"clang version 10.0.1 (https://github.com/tinebp/llvm-project.git 4d79316ed039bf4e0da1dfe432fb0d723ee9a9d5)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym _Bfree
	.addrsig_sym __d2b
	.addrsig_sym _Balloc
	.addrsig_sym __assert_func
	.addrsig_sym __i2b
	.addrsig_sym __pow5mult
	.addrsig_sym __multiply
	.addrsig_sym __hi0bits
	.addrsig_sym __lshift
	.addrsig_sym __mcmp
	.addrsig_sym __multadd
	.addrsig_sym memcpy
	.addrsig_sym quorem
	.addrsig_sym __mdiff
	.addrsig_sym __mprec_tens
	.addrsig_sym __mprec_bigtens
