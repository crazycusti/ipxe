FILE_LICENCE ( GPL2_OR_LATER );
.text
#if _CALL_ELF == 1
.section .opd,"aw"
.align 3
.globl of_linux_enter
of_linux_enter: .quad .of_linux_enter,.TOC.@tocbase,0
.text
.globl .of_linux_enter
.of_linux_enter:
#else
.globl of_linux_enter
.type of_linux_enter,@function
of_linux_enter:
#endif
 mr 12,3
 mr 3,4
 mr 4,5
 mr 5,6
 /* Linux prom_init takes the original 32-bit BE firmware entry. */
 rldicl 10,7,0,1
 clrrdi 10,10,1
 li 6,0
 li 7,0
 mtsrr0 12
 mtsrr1 10
 rfid
.section .note.GNU-stack,"",@progbits
