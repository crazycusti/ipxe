ENTRY(_of_start)
SECTIONS {
 . = 0x01000000;
 .text : { KEEP(*(.text.of_start)) *(.text .text.*) }
 .rodata : { *(.rodata .rodata.*) }
 .data : { *(.data .data.*) KEEP(*(SORT(.tbl.*))) KEEP(*(.provided .provided.*)) }
 .bss : { _bss = .; *(.bss .bss.*) *(COMMON) . = ALIGN(16); _ebss = .; }
 . = ALIGN(16); .stack (NOLOAD) : { . += 65536; _of_stack_top = .; }
 /DISCARD/ : { *(.comment .note* .eh_frame* .einfo* .discard* .weak*) }
}
