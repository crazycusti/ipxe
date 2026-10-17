FILE_LICENCE ( GPL2_OR_LATER );
.text
.globl setjmp
setjmp:
 stw 1,0(3)
 mflr 0
 stw 0,4(3)
 mfcr 0
 stw 0,8(3)
 stmw 14,12(3)
 li 3,0
 blr
.globl longjmp
longjmp:
 lmw 14,12(3)
 lwz 0,8(3)
 mtcrf 255,0
 lwz 0,4(3)
 mtlr 0
 lwz 1,0(3)
 mr 3,4
 cmpwi 3,0
 bne 1f
 li 3,1
1: blr
.section .note.GNU-stack,"",@progbits
