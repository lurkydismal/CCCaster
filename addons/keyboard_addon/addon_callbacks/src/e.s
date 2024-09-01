	.file	"e.c"
# GNU C11 (GCC) version 14.2.1 20240805 (x86_64-pc-linux-gnu)
#	compiled by GNU C version 14.2.1 20240805, GMP version 6.3.0, MPFR version 4.2.1, MPC version 1.3.1, isl version isl-0.26-GMP

# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# options passed: -march=znver1 -mmmx -mpopcnt -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -msse4a -mno-fma4 -mno-xop -mfma -mno-avx512f -mbmi -mbmi2 -maes -mpclmul -mno-avx512vl -mno-avx512bw -mno-avx512dq -mno-avx512cd -mno-avx512vbmi -mno-avx512ifma -mno-avx512vpopcntdq -mno-avx512vbmi2 -mno-gfni -mno-vpclmulqdq -mno-avx512vnni -mno-avx512bitalg -mno-avx512bf16 -mno-avx512vp2intersect -mno-3dnow -madx -mabm -mno-cldemote -mclflushopt -mno-clwb -mclzero -mcx16 -mno-enqcmd -mf16c -mfsgsbase -mfxsr -mno-hle -msahf -mno-lwp -mlzcnt -mmovbe -mno-movdir64b -mno-movdiri -mmwaitx -mno-pconfig -mno-pku -mprfchw -mno-ptwrite -mno-rdpid -mrdrnd -mrdseed -mno-rtm -mno-serialize -mno-sgx -msha -mno-shstk -mno-tbm -mno-tsxldtrk -mno-vaes -mno-waitpkg -mno-wbnoinvd -mxsave -mxsavec -mxsaveopt -mxsaves -mno-amx-tile -mno-amx-int8 -mno-amx-bf16 -mno-uintr -mno-hreset -mno-kl -mno-widekl -mno-avxvnni -mno-avx512fp16 -mno-avxifma -mno-avxvnniint8 -mno-avxneconvert -mno-cmpccxadd -mno-amx-fp16 -mno-prefetchi -mno-raoint -mno-amx-complex -mno-avxvnniint16 -mno-sm3 -mno-sha512 -mno-sm4 -mno-apxf -mno-usermsr --param=l1-cache-size=32 --param=l1-cache-line-size=64 --param=l2-cache-size=512 -mtune=znver1 -m32 -Ofast -std=c11 -fopenmp
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"TEst"
.LC1:
	.string	"lpb: %d %d\n"
	.text
	.p2align 4
	.globl	test
	.type	test, @function
test:
.LFB6603:
	.cfi_startproc
	pushl	%ebx	#
	.cfi_def_cfa_offset 8
	.cfi_offset 3, -8
	call	__x86.get_pc_thunk.bx	#
	addl	$_GLOBAL_OFFSET_TABLE_, %ebx	# tmp98,
	subl	$20, %esp	#,
	.cfi_def_cfa_offset 28
# e.c:8:     printf( "TEst\n" );
	leal	.LC0@GOTOFF(%ebx), %eax	#, tmp99
	pushl	%eax	# tmp99
	.cfi_def_cfa_offset 32
	call	puts@PLT	#
# e.c:32:     printf( "lpb: %d %d\n", l_pressedButtons[ 0 ], l_pressedButtons[ 1 ] );
	addl	$12, %esp	#,
	.cfi_def_cfa_offset 20
	leal	.LC1@GOTOFF(%ebx), %eax	#, tmp100
	pushl	$-1	#
	.cfi_def_cfa_offset 24
	pushl	$0	#
	.cfi_def_cfa_offset 28
	pushl	%eax	# tmp100
	.cfi_def_cfa_offset 32
	call	printf@PLT	#
# e.c:33: }
	addl	$24, %esp	#,
	.cfi_def_cfa_offset 8
	popl	%ebx	#
	.cfi_restore 3
	.cfi_def_cfa_offset 4
	ret	
	.cfi_endproc
.LFE6603:
	.size	test, .-test
	.section	.rodata.str1.1
.LC2:
	.string	"begin"
.LC3:
	.string	"end"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB6604:
	.cfi_startproc
	leal	4(%esp), %ecx	#,
	.cfi_def_cfa 1, 0
	andl	$-16, %esp	#,
	pushl	-4(%ecx)	#
	pushl	%ebp	#
	movl	%esp, %ebp	#,
	.cfi_escape 0x10,0x5,0x2,0x75,0
	pushl	%ebx	#
	pushl	%ecx	#
	.cfi_escape 0xf,0x3,0x75,0x78,0x6
	.cfi_escape 0x10,0x3,0x2,0x75,0x7c
	call	__x86.get_pc_thunk.bx	#
	addl	$_GLOBAL_OFFSET_TABLE_, %ebx	# tmp98,
# e.c:36:     printf( "begin\n" );
	subl	$12, %esp	#,
	leal	.LC2@GOTOFF(%ebx), %eax	#, tmp102
	pushl	%eax	# tmp102
	call	puts@PLT	#
# e.c:38:     test();
	call	test	#
# e.c:40:     printf( "end\n" );
	leal	.LC3@GOTOFF(%ebx), %eax	#, tmp103
	movl	%eax, (%esp)	# tmp103,
	call	puts@PLT	#
# e.c:43: }
	addl	$16, %esp	#,
	leal	-8(%ebp), %esp	#,
	xorl	%eax, %eax	#
	popl	%ecx	#
	.cfi_restore 1
	.cfi_def_cfa 1, 0
	popl	%ebx	#
	.cfi_restore 3
	popl	%ebp	#
	.cfi_restore 5
	leal	-4(%ecx), %esp	#,
	.cfi_def_cfa 4, 4
	ret	
	.cfi_endproc
.LFE6604:
	.size	main, .-main
	.section	.text.__x86.get_pc_thunk.bx,"axG",@progbits,__x86.get_pc_thunk.bx,comdat
	.globl	__x86.get_pc_thunk.bx
	.hidden	__x86.get_pc_thunk.bx
	.type	__x86.get_pc_thunk.bx, @function
__x86.get_pc_thunk.bx:
.LFB6605:
	.cfi_startproc
	movl	(%esp), %ebx	#,
	ret
	.cfi_endproc
.LFE6605:
	.ident	"GCC: (GNU) 14.2.1 20240805"
	.section	.note.GNU-stack,"",@progbits
