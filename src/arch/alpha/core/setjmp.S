FILE_LICENCE ( GPL2_OR_LATER );
.text
.globl setjmp
.ent setjmp
setjmp:
 stq $9,0($16)
 stq $10,8($16)
 stq $11,16($16)
 stq $12,24($16)
 stq $13,32($16)
 stq $14,40($16)
 stq $15,48($16)
 stq $26,56($16)
 stq $30,64($16)
 stq $29,72($16)
 mov $31,$0
 ret $31,($26),1
.end setjmp
.globl longjmp
.ent longjmp
longjmp:
 ldq $9,0($16)
 ldq $10,8($16)
 ldq $11,16($16)
 ldq $12,24($16)
 ldq $13,32($16)
 ldq $14,40($16)
 ldq $15,48($16)
 ldq $26,56($16)
 ldq $30,64($16)
 ldq $29,72($16)
 mov $17,$0
 bne $0,1f
 lda $0,1
1: ret $31,($26),1
.end longjmp
.section .note.GNU-stack,"",@progbits
