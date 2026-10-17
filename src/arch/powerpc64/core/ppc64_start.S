FILE_LICENCE ( GPL2_OR_LATER );
.section .text.of_start,"ax"
#if _CALL_ELF == 1
.section .opd,"aw"
.align 3
.globl _of_start
_of_start: .quad .Lof_entry,.TOC.@tocbase,0
.section .text.of_start,"ax"
#else
.globl _of_start
_of_start:
#endif
.globl _of_code_entry
_of_code_entry:
.Lof_entry:
 mr 31,5
 /* Preserve the original firmware translation state before entering C. */
 mfmsr 10
 lis 9,of_firmware_msr@ha
 addi 9,9,of_firmware_msr@l
 std 10,0(9)
#if _CALL_ELF == 1
 /* Apple OF enters a 64-bit CPU in 32-bit mode. Run the 64-bit client
  * in real mode, and restore firmware translation for each CIF call. */
 srdi 11,10,63
 cmpdi 11,0
 bne 5f
 li 11,1
 sldi 11,11,63
 or 10,10,11
 li 11,0x30
 andc 10,10,11
 lis 9,5f@ha
 addi 9,9,5f@l
 mtsrr0 9
 mtsrr1 10
 rfid
5:
#endif
 bcl 20,31,1f
1: mflr 12
 addis 2,12,.TOC.-1b@ha
 addi 2,2,.TOC.-1b@l
 lis 1,_of_stack_top@ha
 addi 1,1,_of_stack_top@l
 clrrdi 1,1,4
 stdu 1,-128(1)
 lis 3,_bss@ha
 addi 3,3,_bss@l
 lis 4,_ebss@ha
 addi 4,4,_ebss@l
 li 0,0
2: cmpld 3,4
 bge 3f
 stb 0,0(3)
 addi 3,3,1
 b 2b
3: mr 3,31
#if _CALL_ELF == 1
 bl .of_platform_start
#else
 bl of_platform_start
#endif
 nop
4: b 4b
.section .note.GNU-stack,"",@progbits

.section .data,"aw"
.align 3
.globl of_firmware_msr
of_firmware_msr: .quad 0
