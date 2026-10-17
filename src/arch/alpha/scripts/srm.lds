OUTPUT_FORMAT("elf64-alpha")
OUTPUT_ARCH(alpha)
ENTRY(_srm_start)
SECTIONS {
 . = 0xfffffc0001010000;
 .text : { KEEP(*(.text.srm_start)) *(.text .text.*) }
 .rodata : { *(.rodata .rodata.*) }
 . = ALIGN(16); _gp = . + 0x8000;
 .data : { *(.data .data.* .sdata .sdata.*) KEEP(*(SORT(.tbl.*))) KEEP(*(.provided .provided.*)) *(.got .got.* .lit8 .lit4) }
 .bss : { . = ALIGN(16); _bss = .; *(.bss .bss.* .sbss .sbss.*) *(COMMON) . = ALIGN(16); _ebss = .; }
 .stack (NOLOAD) : { . += 262144; _srm_stack_top = .; }
 /DISCARD/ : { *(.comment .note* .eh_frame* .einfo* .discard* .weak*) }
}
