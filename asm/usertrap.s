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

	@ Restore kernel trapframe (side effect: jump back to original lr)
	mov		r0, lr
	ldmfd	sp, {r4, r5, r6, r7, r8, sb, sl, fp, sp, pc}

	.align	2
	.global	kernelExit
	.type	kernelExit, %function
kernelExit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Save kernel trapframe
	stmfd	sp!, {r4, r5, r6, r7, r8, sb, sl, fp, ip, lr}

	@ Switch to system mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x1f
	msr 	CPSR_c, r12

	@ Copy exit_syscall to r12 for later use
	mov		r12, r0

	@ Save user trapframe
	ldmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, sp, lr}

	@ Copy user_program entry point to r3; assign Exit to user lr
	mov		r3, lr
	mov		lr, r12

	@ Switch back to svc mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x13
	msr 	CPSR_c, r12

	@ Go back to user_program entry point
	movs	pc, r3

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

	.align	2
	.global	initTrap
	.type	initTrap, %function
initTrap:
	@ Switch to system mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x1f
	msr 	CPSR_c, r12

	@ Save all register to user stack
	mov		sp, r0
	mov		lr, r1
	mov		r0, #0x00
	mov		r1, #0x11
	mov		r2, #0x22
	mov		r3, #0x33
	mov		ip, sp
	stmfd	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, lr}

	@ Switch back to svc mode
	mrs		r12, cpsr
	bic 	r12, r12, #0x1f
	orr 	r12, r12, #0x13
	msr 	CPSR_c, r12

	mov		pc, lr
