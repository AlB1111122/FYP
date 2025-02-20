#include "../../../../include/asmdefs.h"
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

	adr x0, el1_entry
	msr elr_el2, x0

	eret		

el1_entry:
    // Set stack to start below our code
//    ldr     x1, =__stack_end    // Load the end of the stack space to get the 1/2 gig stack
//    mov     sp, x1
    ldr     x1, =_start
    mov     sp, x1

    // Clean the BSS section
    ldr     x1, =__bss_start     // Start address
    ldr     w2, =__bss_size      // Size of the section
3:  cbz     w2, 4f               // Quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b               // Loop if non-zero

4:  bl      main
    // In case it does return, halt the master core too
    b       1b

.global get_el //exeption level
get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret
