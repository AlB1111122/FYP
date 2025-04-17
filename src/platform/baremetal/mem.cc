#include "../../../include/mem.h"

#include "../../../include/mm.h"
#include "../../../include/peripheralReg.h"

// assembly func defined in boot.s
extern "C" uint64_t id_pgd_addr();

void create_table_entry(uint64_t tbl, uint64_t next_tbl, uint64_t va,
                        uint64_t shift, uint64_t flags) {
  // calc index of current table
  // shift bc different bits are used for diffrent adresses
  uint64_t table_index = (va >> shift) & (ENTRIES_PER_TABLE - 1);

  // comb address of the next-level table with entry flags
  uint64_t descriptor = next_tbl | flags;

  auto table = reinterpret_cast<uint64_t *>(tbl);
  table[table_index] = descriptor;
}

// fill page middle directory
void create_block_map(uint64_t pmd, uint64_t vstart, uint64_t vend,
                      uint64_t pa) {
  // convert virtual start/end to section-aligned table indices
  vstart >>= SECTION_SHIFT;
  vstart &= (ENTRIES_PER_TABLE - 1);

  vend >>= SECTION_SHIFT;
  vend--;
  vend &= (ENTRIES_PER_TABLE - 1);

  // align physical address to section boundary (clear low bits)
  pa >>= SECTION_SHIFT;
  pa <<= SECTION_SHIFT;

  // working on the page middle directory 2mb chunks
  auto table = reinterpret_cast<uint64_t *>(pmd);

  do {
    uint64_t _pa = pa;

    // set cache strategy based of physical adress
    if ((pa >= reg::MAIN_PERIPHERAL_BASE) && (pa <= reg::PERIPHERALS_END)) {
      _pa |= TD_DEVICE_BLOCK_FLAGS;
    } else if (pa == 0xE00000) {  // mark the mailbox non-allocating wb cache
      _pa |= TD_GPU_BLOCK_FLAGS;
    } else {
      _pa |= TD_KERNEL_BLOCK_FLAGS;
    }

    table[vstart] = _pa;

    // next block
    pa += SECTION_SIZE;
    vstart++;
  } while (vstart <= vend);
}

// to be used in the assembly before main
void init_mmu() {
  // get base address of the Page Global Directory (PGD)
  uint64_t id_pgd = id_pgd_addr();

  // can't use memset because MMU needs to already be on for unaligned memory
  // acess
  memzero(id_pgd, ID_MAP_TABLE_SIZE);

  // current virtual address being mapped from
  uint64_t map_base = 0;

  // tbl = the current table being mapped from. Set to current PGD table
  uint64_t tbl = id_pgd;

  // PUD (Page Upper Directory) (lower level table) table beign mapped to
  uint64_t next_tbl = tbl + PAGE_SIZE;

  // PGD to PUD mapping
  create_table_entry(tbl, next_tbl, map_base, PGD_SHIFT,
                     TD_KERNEL_TABLE_FLAGS);  // default kernel flags

  // move on so the PUD table being mapped from
  tbl += PAGE_SIZE;
  // to the PMD
  next_tbl += PAGE_SIZE;

  uint64_t block_tbl = tbl;

  // create 4 PUD entries, each identity map 1GB via 2MB blocks
  for (uint64_t i = 0; i < 4; i++) {
    // PUD → PMD mapping
    create_table_entry(tbl, next_tbl, map_base, PUD_SHIFT,
                       TD_KERNEL_TABLE_FLAGS);

    next_tbl += PAGE_SIZE;
    map_base += PUD_ENTRY_MAP_SIZE;

    block_tbl += PAGE_SIZE;

    uint64_t offset = BLOCK_SIZE * i;
    create_block_map(block_tbl, offset, offset + BLOCK_SIZE, offset);
  }
}