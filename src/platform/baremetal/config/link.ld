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
        __bss_end = .;
    }
    . = 0x40000000;
    .stack (NOLOAD) : {
        . = ALIGN(16);
        __stack_start = .;
        . += 0x20000000;   /* Reserve 512MB for stack */
        __stack_end = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

__bss_size = (__bss_end - __bss_start)>>3;
/*ASSERT(__stack_start >= __bss_end, "Stack overlaps BSS");
ASSERT(__stack_end <= 0x100000000, "Stack exceeds available memory");*/

ASSERT(__stack_start >= __bss_end, "Stack overlaps BSS");
ASSERT(__stack_end <= 0x100000000, "Stack exceeds available memory");
ASSERT(__start_text <= __start_data, "Text overlaps data section");
ASSERT(__end_data <= __bss_start, "Data overlaps BSS section");
ASSERT(__bss_end <= __stack_start, "BSS overlaps stack section");
ASSERT(__stack_end <= 0x100000000, "Stack exceeds available memory");