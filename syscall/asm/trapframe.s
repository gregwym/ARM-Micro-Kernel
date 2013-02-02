	.file	"usertrap.c"
	.text

	.align	2
	.global	irqEntry
	.type	irqEntry, %function
irqEntry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Switch to system mode
	msr		CPSR_c, #0xdf

	@ Save all register to user stack
	@ In order to handling irq, save both `ip` and `sp` and manully adjust `sp`
	stmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}
	sub		sp, sp, #60

	@ Save user_sp to r2, so can be saved in task_descriptor
	mov		r2, sp

	@ Switch back to irq mode and fetch SPSR_irq to r3
	msr		CPSR_c, #0xd2
	mrs		r3, SPSR

	@ Save r3 (SPSR_irq) and (lr_irq - 4) (user resume point) to user stack
	sub		lr, lr, #4
	stmfd	r2!, {r3, lr}

	@ Switch back to svc mode
	msr		CPSR_c, #0xd3

	@ Restore kernel trapframe (side effect: jump back to original lr)
	ldmfd	sp, {r4, r5, r6, r7, r8, sb, sl, fp, sp, pc}


	.align	2
	.global	swiEntry
	.type	swiEntry, %function
swiEntry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	@ Switch to system mode
	msr		CPSR_c, #0xdf

	@ Save all register to user stack
	@ In order to handling irq, save both `ip` and `sp` and manully adjust `sp`
	stmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}
	sub		sp, sp, #60

	@ Save user_sp to r2, so can be saved in task_descriptor
	mov		r2, sp

	@ Switch back to svc mode
	msr		CPSR_c, #0xd3
	mrs		r3, SPSR

	@ Save r3 (SPSR_svc) and lr_svc (user resume point) to user stack
	stmfd	r2!, {r3, lr}

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

	@ Restore the SPSR and lr_svc that was saved on user_stack
	ldmfd	r0!, {r3, lr}
	msr		SPSR, r3

	@ Switch to system mode
	msr		CPSR_c, #0xdf

	@ Setup user_sp
	mov		sp, r0

	@ Restore user trapframe, include ip to handle irq
	ldmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}

	@ Switch back to svc mode
	msr		CPSR_c, #0xd3

	@ Switch to user mode and go back to lr_svc (user_program resume point)
	movs	pc, lr


	.align	2
	.global	initTrap
	.type	initTrap, %function
initTrap:
	@ Switch to system mode
	msr 	CPSR_c, #0xdf

	@ Save all register to user stack
	mov		sp, r0
	mov		lr, r1
	mov		ip, sp
	stmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, sp, lr}
	sub		sp, sp, #60

	@ Save initial SPSR and user_resume_point
	mrs		r0, CPSR
	bic		r0, r0, #0xff
	orr		r0, r0, #0x10
	stmfd	sp!, {r0, r2}

	@ Return new sp to caller
	mov		r0, sp

	@ Switch back to svc mode
	msr 	CPSR_c, #0xd3

	mov		pc, lr

