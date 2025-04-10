#include "../../../include/mem.h"

#include "../../../include/mm.h"
#include "../../../include/peripheralReg.h"

static uint16_t mem_map[PAGING_PAGES] = {
    0,
};

void create_table_entry(uint64_t tbl, uint64_t next_tbl, uint64_t va,
                        uint64_t shift, uint64_t flags) {
  uint64_t table_index = va >> shift;
  table_index &= (ENTRIES_PER_TABLE - 1);
  uint64_t descriptor = next_tbl | flags;
  *((uint64_t *)(tbl + (table_index << 3))) = descriptor;
}

unsigned int HIGH = 0x1A00000;
unsigned int LOW = 0x1600000;
unsigned int SZ = HIGH - LOW;
extern uint8_t __framebuffer_start;
extern uint8_t __framebuffer_end;

void create_block_map(uint64_t pmd, uint64_t vstart, uint64_t vend,
                      uint64_t pa) {
  vstart >>= SECTION_SHIFT;
  vstart &= (ENTRIES_PER_TABLE - 1);

  vend >>= SECTION_SHIFT;
  vend--;
  vend &= (ENTRIES_PER_TABLE - 1);

  pa >>= SECTION_SHIFT;
  pa <<= SECTION_SHIFT;
  /*
  ((pa <= HIGH) && (pa >= LOW)) ||
               ((pa <= 0xF00000) && (pa >= 0xE00000)))
  */

  do {
    uint64_t _pa = pa;

    if (((pa >= reg::MAIN_PERIPHERAL_BASE))) {
      _pa |= TD_DEVICE_BLOCK_FLAGS;
    } else if (pa == 0xE00000) {
      _pa |= TD_GPU_BLOCK_FLAGS;
    } else {
      _pa |= TD_KERNEL_BLOCK_FLAGS;
    }

    *((uint64_t *)(pmd + (vstart << 3))) = _pa;
    pa += SECTION_SIZE;
    vstart++;
  } while (vstart <= vend);
}

extern "C" uint64_t id_pgd_addr();

void init_mmu() {
  uint64_t id_pgd = id_pgd_addr();

  memzero(id_pgd, ID_MAP_TABLE_SIZE);

  uint64_t map_base = 0;
  uint64_t tbl = id_pgd;
  uint64_t next_tbl = tbl + PAGE_SIZE;

  create_table_entry(tbl, next_tbl, map_base, PGD_SHIFT, TD_KERNEL_TABLE_FLAGS);

  tbl += PAGE_SIZE;
  next_tbl += PAGE_SIZE;

  uint64_t block_tbl = tbl;

  for (uint64_t i = 0; i < 4; i++) {
    create_table_entry(tbl, next_tbl, map_base, PUD_SHIFT,
                       TD_KERNEL_TABLE_FLAGS);

    next_tbl += PAGE_SIZE;
    map_base += PUD_ENTRY_MAP_SIZE;

    block_tbl += PAGE_SIZE;

    uint64_t offset = BLOCK_SIZE * i;
    create_block_map(block_tbl, offset, offset + BLOCK_SIZE, offset);
  }
}