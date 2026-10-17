OUTPUT_FORMAT("elf32-hppa-linux")
OUTPUT_ARCH(hppa:hppa1.1)
ENTRY(_pdc_start)
pdc_console_device = 0x3a0;
pdc_keyboard_device = 0x400;
pdc_cycles_per_10ms = 0x38c;
pdc_entry_address = 0x388;
SECTIONS {
 . = 0x00100000;
 .text : { KEEP(*(.text.pdc_start)) *(.text .text.*) }
 .rodata : { *(.rodata .rodata.*) }
 . = ALIGN(16); $global$ = .;
 .data : { *(.data .data.* .sdata .sdata.*) KEEP(*(SORT(.tbl.*))) KEEP(*(.provided .provided.*)) *(.got .got.* .plt .plt.*) }
 .bss : { _bss = .; *(.bss .bss.* .sbss .sbss.*) *(COMMON) . = ALIGN(64); _ebss = .; }
 .stack (NOLOAD) : { _pdc_stack = .; . += 262144; }
 /DISCARD/ : { *(.comment .note* .eh_frame* .einfo* .discard* .weak* .PARISC.unwind .PARISC.archext) }
}
