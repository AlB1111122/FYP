// lld
#pragma once

#define MM_TYPE_PAGE_TABLE 0x3
#define MM_TYPE_PAGE 0x3
#define MM_TYPE_BLOCK 0x1
#define MM_ACCESS (0x1 << 10)
#define MM_ACCESS_PERMISSION (0x01 << 6)

/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]
 *			n	MAIR
 *   DEVICE_nGnRnE	000	00000000
 *   NORMAL_CACHABLE		001	01000100
 */
#define MT_DEVICE_nGnRnE 0x0
#define MT_NORMAL_CACHABLE 0x1
#define MT_DEVICE_nGnRnE_FLAGS 0x00
// 0xCC fastest, needs clear
//  0b01110111 try next
// 0b01010101 slower than cc but works and no need of manual clearence
#define MT_NORMAL_CACHABLE_FLAGS 0xCC
#define MAIR_VALUE                                     \
  (MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | \
      (MT_NORMAL_CACHABLE_FLAGS << (8 * MT_NORMAL_CACHABLE))

#define ATTRINDX_NORMAL 0
#define ATTRINDX_DEVICE 1
#define ATTRINDX_COHERENT 2

#define MAIR_VALUENEW                                          \
  (0xFF << ATTRINDX_NORMAL * 8 | 0x04 << ATTRINDX_DEVICE * 8 | \
   0x00 << ATTRINDX_COHERENT * 8)

#define MMU_FLAGS (MM_TYPE_BLOCK | (MT_NORMAL_CACHABLE << 2) | MM_ACCESS)
#define MMU_DEVICE_FLAGS (MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)
#define MMU_PTE_FLAGS \
  (MM_TYPE_PAGE | (MT_NORMAL_CACHABLE << 2) | MM_ACCESS | MM_ACCESS_PERMISSION)

#define TCR_T0SZ (64 - 48)
#define TCR_T1SZ ((64 - 48) << 16)
#define TCR_TG0_4K (0 << 14)
#define TCR_TG1_4K (2 << 30)
#define TCR_VALUE (TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K)

// System registers
#define SCTLR_EL1_WXN (1 << 19)  // SCTLR_EL1
#define SCTLR_EL1_I (1 << 12)
#define SCTLR_EL1_C (1 << 2)
#define SCTLR_EL1_A (1 << 1)
#define SCTLR_EL1_M (1 << 0)