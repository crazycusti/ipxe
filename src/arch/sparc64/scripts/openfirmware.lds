ENTRY(_of_start)
SECTIONS {
 . = 0x40004000;
 .text : { KEEP(*(.text.of_start)) *(.text .text.*) }
 .rodata : { *(.rodata .rodata.*) }
 .data : { *(.data .data.*) KEEP(*(SORT(.tbl.*))) KEEP(*(.provided .provided.*)) }
 .bss : { _bss = .; *(.bss .bss.*) *(COMMON) . = ALIGN(16); _ebss = .; }
 . = ALIGN(16); .stack (NOLOAD) : { . += 65536; _of_stack_top = .; /* OpenBIOS maps from 0x4000 below the entry. */ . += 0x4000; }
 /DISCARD/ : { *(.comment .note* .eh_frame* .einfo* .discard* .weak*) }
}
