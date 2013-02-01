	.file	"usertrap.c"
	.text
	.align	2
	.global	kernelEntry
	.type	kernelEntry, %function
kernelEntry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Switch to system mode
	msr		CPSR_c, #0xdf

	@ Save all register to user stack
	@ Inorder to handling irq, save both `ip` and `sp` and manully adjust `sp`
	stmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}
	sub		sp, sp, #60

	@ Save user_sp to r2, so can be saved in task_descriptor
	mov		r2, sp

	@ Switch back to svc mode
	msr		CPSR_c, #0xd3

	@ Save user_resume_point to r3, so can be saved in task_descriptor
	mov		r3, lr

	@ Restore kernel trapframe (side effect: jump back to original lr)
	ldmfd	sp, {r4, r5, r6, r7, r8, sb, sl, fp, sp, pc}

	.align	2
	.global	kernelExit
	.type	kernelExit, %function
kernelExit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Save kernel trapframe
	mov		ip, sp
	stmfd	sp!, {r4, r5, r6, r7, r8, sb, sl, fp, ip, lr}

	@ Save r0 (user_program resume point) to lr_svc
	mov		lr, r0

	@ Switch to system mode
	msr		CPSR_c, #0xdf

	@ Restore user trapframe, include ip to handle irq
	ldmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}

	@ Switch back to svc mode
	msr		CPSR_c, #0xd3

	@ Switch to user mode and go back to lr_svc (user_program resume point)
	movs	pc, lr


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
	msr 	CPSR_c, #0xdf

	@ Save all register to user stack
	mov		sp, r0
	mov		lr, r1
	mov		r0, #0x00
	mov		r1, #0x11
	mov		r2, #0x22
	mov		r3, #0x33
	mov		ip, sp
	stmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}
	sub		sp, sp, #60

	@ Return new sp to caller
	mov		r0, sp

	@ Switch back to svc mode
	msr 	CPSR_c, #0xd3

	mov		pc, lr

	.align	2
	.global	activateStack
	.type	activateStack, %function
activateStack:
	@ Switch to system mode
	msr 	CPSR_c, #0xdf

	@ activate given stack pointer
	mov		sp, r0

	@ Switch back to svc mode
	msr 	CPSR_c, #0xd3

	mov		pc, lr
