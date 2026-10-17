FILE_LICENCE ( GPL2_OR_LATER );
.text
#if _CALL_ELF == 1
.section .opd,"aw"
.align 3
.globl setjmp
setjmp: .quad .setjmp,.TOC.@tocbase,0
.text
.globl .setjmp
.setjmp:
#else
.globl setjmp
.type setjmp,@function
setjmp:
#endif
 std 1,0(3)
 mflr 0
 std 0,8(3)
 mfcr 0
 std 0,16(3)
 std 2,24(3)
 std 13,32(3)
 std 14,40(3)
 std 15,48(3)
 std 16,56(3)
 std 17,64(3)
 std 18,72(3)
 std 19,80(3)
 std 20,88(3)
 std 21,96(3)
 std 22,104(3)
 std 23,112(3)
 std 24,120(3)
 std 25,128(3)
 std 26,136(3)
 std 27,144(3)
 std 28,152(3)
 std 29,160(3)
 std 30,168(3)
 std 31,176(3)
 li 3,0
 blr
#if _CALL_ELF == 1
.section .opd,"aw"
.align 3
.globl longjmp
longjmp: .quad .longjmp,.TOC.@tocbase,0
.text
.globl .longjmp
.longjmp:
#else
.globl longjmp
.type longjmp,@function
longjmp:
#endif
 ld 13,32(3)
 ld 14,40(3)
 ld 15,48(3)
 ld 16,56(3)
 ld 17,64(3)
 ld 18,72(3)
 ld 19,80(3)
 ld 20,88(3)
 ld 21,96(3)
 ld 22,104(3)
 ld 23,112(3)
 ld 24,120(3)
 ld 25,128(3)
 ld 26,136(3)
 ld 27,144(3)
 ld 28,152(3)
 ld 29,160(3)
 ld 30,168(3)
 ld 31,176(3)
 ld 2,24(3)
 ld 0,16(3)
 mtcrf 255,0
 ld 0,8(3)
 mtlr 0
 ld 1,0(3)
 mr 3,4
 cmpdi 3,0
 bne 1f
 li 3,1
1: blr
.section .note.GNU-stack,"",@progbits
