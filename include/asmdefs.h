#pragma once

#define SCTLR_RESERVED (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_EE_LITTLE_ENDIAN (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN (0 << 24)
#define SCTLR_I_CACHE_DISABLED (0 << 12)
#define SCTLR_D_CACHE_DISABLED (0 << 2)
#define SCTLR_I_CACHE_ENABLED (1 << 12)
#define SCTLR_D_CACHE_ENABLED (1 << 2)
#define SCTLR_MMU_DISABLED (0 << 0)
#define SCTLR_MMU_ENABLED (1 << 0)
// for use on SCTLR_EL1, System Control Register (EL1)
#define SCTLR_VALUE_MMU_DISABLED                                      \
  (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | \
   SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)
#define SCTLR_VALUE_MMU_ENABLED                                      \
  (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_ENABLED | \
   SCTLR_D_CACHE_ENABLED | SCTLR_MMU_ENABLED)

// make aarch64 instructions avalible in el1 otherwise aarch32 only
#define HCR_RW (1 << 31)
// for use on HCR_EL2, Hypervisor Configuration Register
#define HCR_VALUE HCR_RW

#define SCR_RESERVED (3 << 4)
#define SCR_RW (1 << 10)
#define SCR_NS (1 << 0)
// for use on SPSR_EL2 Saved Program Status Register (EL2)
#define SCR_VALUE (SCR_RESERVED | SCR_RW | SCR_NS)

// mask interrupts to disable them untill we get to el1
#define SPSR_MASK_ALL (7 << 6)
// switch to el1 on eret
#define SPSR_EL1h (5 << 0)
// for use on ELR_EL2, Exception Link Register (EL2)
#define SPSR_VALUE (SPSR_MASK_ALL | SPSR_EL1h)

/* architectural feature access control register */
#define CPACR_EL1_FPEN (1 << 21) | (1 << 20)  // don't trap SIMD/FP registers
#define CPACR_EL1_ZEN (1 << 17) | (1 << 16)   // don't trap SVE instructions
#define CPACR_EL1_VAL (CPACR_EL1_FPEN | CPACR_EL1_ZEN)
