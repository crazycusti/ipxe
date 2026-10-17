/* Bridge the Unix ABI to the SRM VMS callback ABI. */
FILE_LICENCE ( GPL2_OR_LATER );
.text
.globl srm_dispatch
.ent srm_dispatch
srm_dispatch:
 lda $30,-80($30)
 stq $9,0($30)
 stq $10,8($30)
 stq $11,16($30)
 stq $12,24($30)
 stq $13,32($30)
 stq $14,40($30)
 stq $15,48($30)
 stq $26,56($30)
 stq $29,64($30)
 mov $16,$27
 ldq $1,8($27)
 mov $17,$16
 mov $18,$17
 mov $19,$18
 mov $20,$19
 mov $21,$20
 jsr $26,($1),0
 ldq $9,0($30)
 ldq $10,8($30)
 ldq $11,16($30)
 ldq $12,24($30)
 ldq $13,32($30)
 ldq $14,40($30)
 ldq $15,48($30)
 ldq $26,56($30)
 ldq $29,64($30)
 lda $30,80($30)
 ret $31,($26),1
.end srm_dispatch
.section .note.GNU-stack,"",@progbits
