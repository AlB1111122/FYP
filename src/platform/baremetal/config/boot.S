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
    //switch from EL2 down to EL1 as rpi4 boots to el2 where fast fp is not avalible
	ldr x0, =SCTLR_VALUE_MMU_DISABLED
	msr sctlr_el1, x0

	ldr x0, =HCR_VALUE
	msr hcr_el2, x0

	ldr x0, =SPSR_VALUE
	msr spsr_el2, x0
    bl full_cache_clean

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

    //mrs x0, sctlr_el1
    //mov x1, #SCTLR_MMU_ENABLED
    //orr x0, x0, x1
    ldr x0, =SCTLR_VALUE_MMU_ENABLED
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

.global full_cache_clean
full_cache_clean:
    MRS X0, CLIDR_EL1
    AND W3, W0, #0x07000000 // Get 2 x Level of Coherence
    LSR W3, W3, #23
    CBZ W3, Finished
    MOV W10, #0 // W10 = 2 x cache level
    MOV W8, #1 // W8 = constant 0b1
Loop1: 
    ADD W2, W10, W10, LSR #1 // Calculate 3 x cache level
    LSR W1, W0, W2 // extract 3-bit cache type for this level
    AND W1, W1, #0x7
    CMP W1, #2
    B.LT Skip // No data or unified cache at this level
    MSR CSSELR_EL1, X10 // Select this cache level
    ISB // Synchronize change of CSSELR
    MRS X1, CCSIDR_EL1 // Read CCSIDR
    AND W2, W1, #7 // W2 = log2(linelen)-4
    ADD W2, W2, #4 // W2 = log2(linelen)
    UBFX W4, W1, #3, #10 // W4 = max way number, right aligned
    CLZ W5, W4 /* W5 = 32-log2(ways), bit position of way in DC
    operand */
    LSL W9, W4, W5 /* W9 = max way number, aligned to position in DC
    operand */
    LSL W16, W8, W5 // W16 = amount to decrement way number per iteration
Loop2: 
    UBFX W7, W1, #13, #15 // W7 = max set number, right aligned
    LSL W7, W7, W2 /* W7 = max set number, aligned to position in DC
operand */
    LSL W17, W8, W2 // W17 = amount to decrement set number per iteration
Loop3: 
    ORR W11, W10, W9 // W11 = combine way number and cache number...
    ORR W11, W11, W7 // ... and set number for DC operand
    DC CSW, X11 // Do data cache clean by set and way
    SUBS W7, W7, W17 // Decrement set number
    B.GE Loop3
    SUBS X9, X9, X16 // Decrement way number
    B.GE Loop2
Skip: 
    ADD W10, W10, #2 // Increment 2 x cache level
    CMP W3, W10
    DSB sy
    B.GT Loop1
Finished:
    ret