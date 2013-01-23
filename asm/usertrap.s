	.file	"usertrap.c"
	.text
	.align	2
	.global	kernelEntry
	.type	kernelEntry, %function
kernelEntry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Switch to system mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x1f
	msr 	CPSR_c, r12

	@ Save all register to user stack
	mov		ip, sp
	stmfd	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, lr}

	@ Save sp to r5 for syscallHandler
	mov		r5, sp

	@ Switch back to svc mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x13
	msr 	CPSR_c, r12

	@ Branch to syscallHandler, NEVER comeback
	b		syscallHandler(PLT)

	.align	2
	.global	kernelExit
	.type	kernelExit, %function
kernelExit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Switch to system mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x1f
	msr 	CPSR_c, r12

	ldmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, sp, lr}

	@ Switch back to svc mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x13
	msr 	CPSR_c, r12

	ldmfd	sp!, {lr}
	movs	pc, lr

	.size	kernelExit, .-kernelExit
	.ident	"GCC: (GNU) 4.0.2"

	.align	2
	.global	switchCpuMode
	.type	switchCpuMode, %function
switchCpuMode:
	mov		r3, lr
	mrs		r12, cpsr
	bic		r12, r12, #0x1f
	orr		r12, r12, r0
	msr		CPSR_c, r12

	mov		pc, r3
