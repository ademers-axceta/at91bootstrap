/*
 * Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: MIT
 */

#include <hardware.h>
#include <mon_macros.h>

#if defined(CONFIG_ENTER_NWD)
#define	STACK_TOP		SVC_STACK_BASE
#else
#define	STACK_TOP		TOP_OF_MEMORY
#endif

.section start
	.text

/*----------------------------------------------------------------------------
 Area Definition
----------------
 Must be defined as function to put first in the code as it must be mapped
 at offset 0 of the flash EBI_CSR0, ie. at address 0 before remap.
_---------------------------------------------------------------------------*/

/* Application startup entry point */

	.globl reset
	.align 4
reset:

/* Exception vectors (should be a branch to be detected as a valid code by the rom */
_exception_vectors:
	b 	reset_vector    /* reset */
	b 	undef_vector 	/* Undefined Instruction */
	b 	swi_vector   	/* Software Interrupt */
	b 	pabt_vector  	/* Prefetch Abort */
	b 	dabt_vector  	/* Data Abort */
.word		_romsize	/* Size of the binary for ROMCode loading */
	b 	irq_vector	/* IRQ : read the AIC */
	b 	fiq_vector      /* FIQ */

undef_vector:
	b 	undef_vector
swi_vector:
	b 	swi_vector
pabt_vector:
	b 	pabt_vector
dabt_vector:
	subs	pc, r14, #4	/* return */
	nop
rsvd_vector:
	b 	rsvd_vector
irq_vector:
	b 	irq_vector
fiq_vector:
	b 	fiq_vector
reset_vector:

#if defined(CONFIG_CPU_V7)
	/*
	 * We must configure VBAR for the reset vectors to be at
	 * the start of SRAM (CONFIG_LINK_ADDR)
	 */
	mrc     p15, 0, r2, c12, c0, 0 /* Read VBAR into R2 */
	mov	r2,#LINK_ADDR
	mcr	p15, 0, r2, c12, c0, 0
#endif /* CONFIG_CPU_V7 */

#ifdef BACKUP_REGISTER_BOOT_MODE_R4
	ldr	r1, =BACKUP_REGISTER_BOOT_MODE_R4
	str	r4, [r1]
#endif

/* Init the stack */
_init_stack:
	ldr	sp,=STACK_TOP
	/* Save BootROM supplied boot source information to stack */
	push	{r4}

#ifdef CONFIG_FLASH
/*
 * When running from NOR, we must relocate to SRAM prior to resetting
 * the clocks and SMC timings.
 */
_relocate_to_sram:

#if 0
	/* relocation is slow, disable the watchdog or it will trigger */
	ldr	r1, =0xFFFFFD44
	mov	r2, #0x00008000
	str	r2, [r1]
#endif

	mov	r1, #0
	ldr	r3, =_stext
	ldr	r4, =_edata
1:
	cmp     r3, r4
	ldrcc   r2, [r1], #4
	strcc   r2, [r3], #4
	bcc     1b
#endif /* CONFIG_FLASH */

#if defined(CONFIG_PMC_COMMON)
	ldr     r4, = lowlevel_clock_init
	mov     lr, pc
	bx      r4
#endif /* CONFIG_PMC */

#if defined(CONFIG_CPU_V7)
/* Clear Abort condition if it is pending with help of the abort handler */
	ldr	r1, =(CPSR_A_BIT)
	mrs	r0, cpsr
	eor	r0, r0, r1
	msr	cpsr_x, r0
	nop
	eor	r0, r0, r1
	msr	cpsr_x, r0
#endif

/* Copy the data section in RAM at .data link address */
_init_data:
        ldr      r2, =_lp_data
        ldmia    r2, {r1, r3, r4}
1:
        cmp      r3, r4
        ldrcc    r2, [r1], #4
        strcc    r2, [r3], #4
        bcc      1b

/* Initialize the bss segment */
_init_bss:
	adr    r2, _lp_bss
	ldmia  r2, {r3, r4}
	mov    r2, #0
1:
	cmp    r3, r4
	strcc  r2, [r3], #4
	bcc    1b

#if defined(CONFIG_ENTER_NWD)
/* Copy the monitor in RAM at its VMA address */
_init_mon:
        ldr      r2, =_lp_mon
        ldmia    r2, {r1, r3, r4}
1:
        cmp      r3, r4
        ldrcc    r2, [r1], #4
        strcc    r2, [r3], #4
        bcc      1b
#endif

/* Branch on C code Main function (with interworking) */
_branch_main:
	ldr     r4, = main
	mov     lr, pc
	bx      r4

/* Branch to the application at the end of the bootstrap init */
_go:
	ldr	r1, =(STACK_TOP - 4)
	ldr	r4, [r1]
	ldr 	r1, =MACH_TYPE
	mov     lr, pc

/* we endless loop here and not jump to any application
 * if we are configured to not load anything
 */
#if defined(CONFIG_ENTER_NWD) || !defined(CONFIG_JUMP_TO_SW)
loop:
#ifdef CONFIG_BKPT_NOTIFY_DONE
	bkpt
#endif
	b	loop
#endif

	bx      r0

/*#ifdef CONFIG_THUMB*/

	.globl set_cp15
set_cp15:
	mcr	p15, 0, r0, c1, c0, 0
	bx	lr

	.globl get_cp15
get_cp15:
	mrc	p15, 0, r0, c1, c0, 0
	bx	lr

	.global disable_irq
disable_irq:
	mrs	r0, cpsr
	orr r0, r0, #0xc0
	msr	cpsr_c, r0
	bx	lr

	.global get_cpsr
get_cpsr:
	mrs r0, cpsr
	bx	lr

	.global set_cpsr
set_cpsr:
	msr cpsr_c, r0
	bx	lr

	.global disable_icache
disable_icache:
	mrc p15, 0, r0, c1, c0, 0
	mvn	r1, #(1 << 12)
	and r0, r0, r1
	mcr	p15, 0, r0, c1, c0, 0
	bx	lr

	.global disable_dcache
disable_dcache:
	mrc p15, 0, r0, c1, c0, 0
	mvn	r1, #(1 << 2)
	and r0, r0, r1
	mcr	p15, 0, r0, c1, c0, 0
	bx	lr

	.global flush_idcache
flush_idcache:
	mov	r0, #0
	mcr p15, 0, r0, c7, c7, 0
	bx	lr

/*#endif*/

	.align
_lp_data:
        .word _edummy
        .word _sdata
        .word _edata

_lp_bss:
	.word _sbss
	.word _ebss

#if defined(CONFIG_ENTER_NWD)
_lp_mon:
        .word _emon_dummy
	.word _smon_text
	.word _emon_text
#endif
#if defined(CONFIG_MMU)
	.arm
	.global cp15_read_sctlr
	.type	cp15_read_sctlr, %function
cp15_read_sctlr:
	mrc	p15, 0, r0, c1, c0, 0
	bx	lr

	.global cp15_write_sctlr
	.type	cp15_write_sctlr, %function
cp15_write_sctlr:
	mcr	p15, 0, r0, c1, c0, 0
	bx	lr

	.global cp15_write_ttbr0
	.type	cp15_write_ttbr0, %function
cp15_write_ttbr0:
	mcr	p15, 0, r0, c2, c0, 0
	bx	lr

	.global cp15_write_dacr
	.type	cp15_write_dacr, %function
cp15_write_dacr:
	mcr	p15, 0, r0, c3, c0, 0
	bx	lr

	.global cp15_icache_invalidate
	.type	cp15_icache_invalidate, %function
cp15_icache_invalidate:
	mcr	p15, 0, r0, c7, c5, 0
	bx	lr

	.global cp15_dcache_invalidate_setway
	.type	cp15_dcache_invalidate_setway, %function
cp15_dcache_invalidate_setway:
	mcr	p15, 0, r0, c7, c6, 2
	bx	lr

	.global cp15_dcache_clean_setway
	.type	cp15_dcache_clean_setway, %function
cp15_dcache_clean_setway:
	mcr	p15, 0, r0, c7, c10, 2
	bx	lr

	.global dsb
	.type	dsb, %function
dsb:
	mcr	p15, 0, r0, c7, c10, 4
	bx	lr
#endif
