/* Alpha's compiler division convention uses r24/r25, r27 and link r23,
 * rather than the C calling convention. No integer divide instruction exists.
 */
FILE_LICENCE ( GPL2_OR_LATER );
.text
.set noat
.macro divide name, narrow=0, signed=0, remainder=0
.globl \name
.ent \name
\name:
 lda $30,-64($30)
 stq $1,0($30)
 stq $2,8($30)
 stq $3,16($30)
 stq $4,24($30)
 stq $24,32($30)
 stq $25,40($30)
 stq $28,48($30)
 .if \narrow
 .if \signed
 addl $24,0,$24
 addl $25,0,$25
 .else
 zapnot $24,15,$24
 zapnot $25,15,$25
 .endif
 .endif
 .if \signed
 .if \remainder
 mov $24,$4
 .else
 xor $24,$25,$4
 .endif
 cmplt $24,0,$1
 subq $31,$24,$2
 cmovne $1,$2,$24
 cmplt $25,0,$1
 subq $31,$25,$2
 cmovne $1,$2,$25
 .endif
 bne $25,1f
 call_pal 0x80
1: mov $31,$27
 mov $31,$28
 lda $1,64
2: srl $28,63,$2
 sll $28,1,$28
 srl $24,63,$3
 bis $28,$3,$28
 sll $24,1,$24
 sll $27,1,$27
 cmpule $25,$28,$3
 bis $2,$3,$3
 beq $3,3f
 subq $28,$25,$28
 bis $27,1,$27
3: subq $1,1,$1
 bne $1,2b
 .if \remainder
 mov $28,$27
 .endif
 .if \signed
 cmplt $4,0,$1
 subq $31,$27,$2
 cmovne $1,$2,$27
 .endif
 .if \narrow
 addl $27,0,$27
 .endif
 ldq $1,0($30)
 ldq $2,8($30)
 ldq $3,16($30)
 ldq $4,24($30)
 ldq $24,32($30)
 ldq $25,40($30)
 ldq $28,48($30)
 lda $30,64($30)
 ret $31,($23),1
.end \name
.endm
divide __divqu
divide __remqu,0,0,1
divide __divq,0,1
divide __remq,0,1,1
divide __divlu,1
divide __remlu,1,0,1
divide __divl,1,1
divide __reml,1,1,1
.section .note.GNU-stack,"",@progbits
