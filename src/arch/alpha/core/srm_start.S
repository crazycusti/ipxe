FILE_LICENCE ( GPL2_OR_LATER );
.section .text.srm_start,"ax"
.globl _srm_start
.ent _srm_start
_srm_start:
 ldgp $29,0($27)
 lda $30,_srm_stack_top
 lda $16,_bss
 lda $17,_ebss
1: cmpult $16,$17,$1
 beq $1,2f
 stq $31,0($16)
 addq $16,8,$16
 br $31,1b
2: jsr $26,srm_platform_start
3: br $31,3b
.end _srm_start
.section .note.GNU-stack,"",@progbits
