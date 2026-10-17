ENTRY(_of_start)
SECTIONS {
 . = 0x00004000;
 .text : AT(ADDR(.text) + 0xf0000000) { KEEP(*(.text.of_start)) *(.text .text.*) }
 .rodata : AT(ADDR(.rodata) + 0xf0000000) { *(.rodata .rodata.*) }
 .data : AT(ADDR(.data) + 0xf0000000) { *(.data .data.*) KEEP(*(SORT(.tbl.*))) KEEP(*(.provided .provided.*)) }
 .bss : AT(ADDR(.bss) + 0xf0000000) { _bss = .; *(.bss .bss.*) *(COMMON) . = ALIGN(16); _ebss = .; }
 . = ALIGN(16); .stack (NOLOAD) : AT(ADDR(.stack) + 0xf0000000) { . += 65536; _of_stack_top = .; }
 /DISCARD/ : { *(.comment .note* .eh_frame* .einfo* .discard* .weak*) }
}
