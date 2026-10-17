FILE_LICENCE ( GPL2_OR_LATER );
.section ".text.of_start","ax"
.globl _of_start
_of_start:
 save %sp,-96,%sp
 mov %i0,%g7
 set _of_stack_top,%sp
 sub %sp,96,%sp
 set _bss,%o0
 set _of_stack_top,%o1
1: cmp %o0,%o1
 bgeu 2f
  nop
 st %g0,[%o0]
 ba 1b
  add %o0,4,%o0
2:
 /* Keep the PROM trap table; permit its level-14 clock interrupt. */
 rd %psr,%g1
 andn %g1,0xf00,%g1
 or %g1,0xd00,%g1
 wr %g1,0,%psr
 nop
 nop
 nop
 call openprom_start
  mov %g7,%o0
3: ba 3b
  nop
.section .note.GNU-stack,"",@progbits
