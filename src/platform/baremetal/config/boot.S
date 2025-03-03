#include "../../../../include/asmdefs.h"
#include "../../../../include/mm.h"
#include "../../../../include/mmu.h"
.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here

_start:
    // Check processor ID is zero (executing on main core), else hang
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // We're not on the main core, so hang in an infinite wait loop
1:  wfe
    b       1b
2:  // We're on the main core!
    //enable fpu
    mov     x0, #3 << 20
    msr     cpacr_el1, x0	 // Enable FP/SIMD at EL1
    //switch from EL2 down to EL1 as rpi4 boots to el2 where fast fp is not avalible
	ldr x0, =SCTLR_VALUE_MMU_DISABLED
	msr sctlr_el1, x0

	ldr x0, =HCR_VALUE
	msr hcr_el2, x0

	ldr x0, =SPSR_VALUE
	msr spsr_el2, x0

    //mmu enablement:
    ldr x0, =CPACR_EL1_VAL
    msr CPACR_EL1, x0

    ldr x0, =TCR_EL1_VAL
    msr TCR_EL1, x0

    ldr x0, =MAIR_VALUE
    msr MAIR_EL1, x0
    //end mmu enablement

	adr x0, el1_entry
	msr elr_el2, x0

	eret		

el1_entry:
    // Set stack to start above our code
    ldr     x1, =LOW_MEMORY
    mov     sp, x1

    // Clean the BSS section
    ldr     x1, =__bss_start     // Start address
    ldr     w2, =__bss_size      // Size of the section
3:  cbz     w2, 4f               // Quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b               // Loop if non-zero

    //mmu enablement:
    bl init_mmu

    adrp x0, id_pgd
    msr ttbr0_el1, x0

    mrs x0, sctlr_el1
    mov x1, #SCTLR_MMU_ENABLED
    orr x0, x0, x1
    msr sctlr_el1, x0
    //end mmu enablement

4:  bl      main
    // In case it does return, halt the master core too
    b       1b

.global getEl //exeption level
getEl:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

// https://developer.arm.com/documentation/100941/0101/Barriers
.global instructionSyncBarrier
instructionSyncBarrier:
	isb

.global dataSyncBarrier //data sync, whole system
dataSyncBarrier:
	dsb sy

.global memorySyncBarrier //data sync, whole system
memorySyncBarrier:
	dmb sy

.globl memzero
memzero:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt memzero
    ret

.globl id_pgd_addr
id_pgd_addr:
    adrp x0, id_pgd
    ret