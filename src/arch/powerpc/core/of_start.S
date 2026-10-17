FILE_LICENCE ( GPL2_OR_LATER );
.section .text.of_start,"ax"
.globl _of_start
_of_start:
 mr 31,5
 lis 1,_of_stack_top@ha
 addi 1,1,_of_stack_top@l
 clrrwi 1,1,4
 stwu 1,-32(1)
 lis 3,_bss@ha
 addi 3,3,_bss@l
 lis 4,_ebss@ha
 addi 4,4,_ebss@l
 li 0,0
1: cmplw 3,4
 bge 2f
 stb 0,0(3)
 addi 3,3,1
 b 1b
2: mr 3,31
 bl of_platform_start
3: b 3b
.section .note.GNU-stack,"",@progbits
