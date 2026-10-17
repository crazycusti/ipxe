FILE_LICENCE ( GPL2_OR_LATER );
.text
#if _CALL_ELF == 1
.section .opd,"aw"
.align 3
.globl of_arch_invoke
of_arch_invoke: .quad .of_arch_invoke,.TOC.@tocbase,0
.text
.globl .of_arch_invoke
.of_arch_invoke:
#else
.globl of_arch_invoke
.type of_arch_invoke,@function
of_arch_invoke:
#endif
 stdu 1,-288(1)
 /* Leave the firmware caller parameter area free before saved state. */
 mflr 0
 std 0,64(1)
 std 2,72(1)
 mfmsr 10
 std 10,80(1)
 mfcr 0
 std 0,88(1)
 std 13,96(1)
 std 14,104(1)
 std 15,112(1)
 std 16,120(1)
 std 17,128(1)
 std 18,136(1)
 std 19,144(1)
 std 20,152(1)
 std 21,160(1)
 std 22,168(1)
 std 23,176(1)
 std 24,184(1)
 std 25,192(1)
 std 26,200(1)
 std 27,208(1)
 std 28,216(1)
 std 29,224(1)
 std 30,232(1)
 std 31,240(1)
 mr 31,4
 mtctr 31
 lis 29,.Lnative_return@ha
 addi 29,29,.Lnative_return@l
 lis 9,.Lfirmware_be@ha
 addi 9,9,.Lfirmware_be@l
 mtsrr0 9
 lis 10,of_firmware_msr@ha
 ld 10,of_firmware_msr@l(10)
 /* Clear SF and LE for the 32-bit firmware call. */
 rldicl 10,10,0,1
 clrrdi 10,10,1
 mtsrr1 10
 rfid
.Lfirmware_be:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
 /* Big-endian opcodes inside a little-endian ELF section. */
 .long 0x2104804e /* bctrl */
 .long 0x50002039 /* li r9,80 */
 .long 0x284c417d /* ldbrx r10,r1,r9 */
 .long 0xa603ba7f /* mtsrr0 r29 */
 .long 0xa6035b7d /* mtsrr1 r10 */
 .long 0x2400004c /* rfid */
#else
 bctrl
 ld 10,80(1)
 mtsrr0 29
 mtsrr1 10
 rfid
#endif
.Lnative_return:
 ld 13,96(1)
 ld 14,104(1)
 ld 15,112(1)
 ld 16,120(1)
 ld 17,128(1)
 ld 18,136(1)
 ld 19,144(1)
 ld 20,152(1)
 ld 21,160(1)
 ld 22,168(1)
 ld 23,176(1)
 ld 24,184(1)
 ld 25,192(1)
 ld 26,200(1)
 ld 27,208(1)
 ld 28,216(1)
 ld 29,224(1)
 ld 30,232(1)
 ld 31,240(1)
 ld 0,88(1)
 mtcrf 255,0
 ld 2,72(1)
 ld 0,64(1)
 mtlr 0
 addi 1,1,288
 extsw 3,3
 blr
.section .note.GNU-stack,"",@progbits
