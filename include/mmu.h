#pragma once

#ifndef __ASSEMBLER__
extern "C" void init_mmu();
#endif

// asm register defs for mmu
#define MT_DEVICE_nGnRnE_IDX 0x0
#define MT_NORMAL_CACHABLE_IDX 0x1
#define MT_NORMAL_C_NA_IDX 0x2
#define MT_DEVICE_nGnRnE_FLAGS 0x00
#define MT_NORMAL_CACHABLE_FLAGS 0xFF
// non allocating write back cache
#define MT_NORMAL_C_NA_FLAGS 0xCC

#define MAIR_VALUE                                                 \
  (MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE_IDX)) |         \
      (MT_NORMAL_CACHABLE_FLAGS << (8 * MT_NORMAL_CACHABLE_IDX)) | \
      (MT_NORMAL_C_NA_FLAGS << (8 * MT_NORMAL_C_NA_IDX))

// translation control register defines page size and va size
// 48 bit va space, 4kb page
#define TCR_TG0_4K (0 << 14)
#define TCR_T0SZ (64 - 48)
#define TCR_EL1_VAL (TCR_TG0_4K | TCR_T0SZ)

// magic numbers for setting up va tables
#define PAGE_SHIFT 12
#define TABLE_SHIFT 9
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)
#define PAGE_SIZE (1 << PAGE_SHIFT)  // must match linkscript .data.id_pgd
#define SECTION_SIZE (1 << SECTION_SHIFT)

#define LOW_MEMORY (8 * SECTION_SIZE)

#define HIGH_MEMORY 0x40000000
#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES (PAGING_MEMORY / PAGE_SIZE)

#define ID_MAP_PAGES 6  // must match linkscript .data.id_pgd
#define ID_MAP_TABLE_SIZE (ID_MAP_PAGES * PAGE_SIZE)
#define ENTRIES_PER_TABLE 512
#define PGD_SHIFT (PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT (PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PUD_ENTRY_MAP_SIZE (1 << PUD_SHIFT)

#define BLOCK_SIZE 0x40000000

// descriptiors for blocks in table
#define TD_VALID (1 << 0)
#define TD_BLOCK (0 << 1)
#define TD_TABLE (1 << 1)
#define TD_ACCESS (1 << 10)
#define TD_KERNEL_PERMS (1L << 54)
#define TD_INNER_SHARABLE (3 << 8)

#define TD_KERNEL_TABLE_FLAGS (TD_TABLE | TD_VALID)
#define TD_KERNEL_BLOCK_FLAGS                        \
  (TD_ACCESS | TD_INNER_SHARABLE | TD_KERNEL_PERMS | \
   (MT_NORMAL_CACHABLE_IDX << 2) | TD_BLOCK | TD_VALID)
#define TD_DEVICE_BLOCK_FLAGS                        \
  (TD_ACCESS | TD_INNER_SHARABLE | TD_KERNEL_PERMS | \
   (MT_DEVICE_nGnRnE_IDX << 2) | TD_BLOCK | TD_VALID)
#define TD_GPU_BLOCK_FLAGS                           \
  (TD_ACCESS | TD_INNER_SHARABLE | TD_KERNEL_PERMS | \
   (MT_NORMAL_C_NA_IDX << 2) | TD_BLOCK | TD_VALID)
