SECTIONS
{
    . = 0x80000;     /* Kernel load address for AArch64 */
    .text : {
        __start_text = .;
        KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*)
        __end_text = .;
        }
    .rodata : { 
        __start_rodata = .;
        *(.rodata .rodata.* .gnu.linkonce.r*) 
        __end_rodata = .;
        }
    PROVIDE(_data = .);
    .data : {
        __start_data = .;
        *(.data .data.* .gnu.linkonce.d*) 
        __end_data = .;}
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    . = ALIGN(0x00001000);
    id_pgd = .;
    .data.id_pgd : { . += (6 * (1 << 12)); }/* Must match ID_MAP_PAGES and PAGE_SIZE in mmu.h respectivley*/
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

__bss_size = (__bss_end - __bss_start)>>3;