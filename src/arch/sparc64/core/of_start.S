FILE_LICENCE ( GPL2_OR_LATER );
.section ".text.of_start","ax"
.globl _of_start
_of_start:
 save %sp,-192,%sp
 mov %i4,%g7
 set _of_stack_top,%sp
 sub %sp,2047,%sp
 sub %sp,192,%sp
 set _bss,%o0
 set _of_stack_top,%o1
1: cmp %o0,%o1
 bgeu,pn %xcc,2f
  nop
 stx %g0,[%o0]
 ba 1b
  add %o0,8,%o0
2: call of_platform_start
  mov %g7,%o0
3: ba 3b
  nop

.section .note.GNU-stack,"",@progbits
