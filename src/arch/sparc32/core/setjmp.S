FILE_LICENCE ( GPL2_OR_LATER );
.text
.align 4
.globl setjmp
setjmp:
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 restore
 restore
 restore
 restore
 restore
 restore
 restore
 st %sp,[%o0]
 st %o7,[%o0+4]
 st %l0,[%o0+8]
 st %l1,[%o0+12]
 st %l2,[%o0+16]
 st %l3,[%o0+20]
 st %l4,[%o0+24]
 st %l5,[%o0+28]
 st %l6,[%o0+32]
 st %l7,[%o0+36]
 st %i0,[%o0+40]
 st %i1,[%o0+44]
 st %i2,[%o0+48]
 st %i3,[%o0+52]
 st %i4,[%o0+56]
 st %i5,[%o0+60]
 st %i6,[%o0+64]
 st %i7,[%o0+68]
 retl
  clr %o0
.globl longjmp
longjmp:
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 save %sp,-96,%sp
 restore
 restore
 restore
 restore
 restore
 restore
 restore
 ld [%o0],%sp
 ld [%o0+4],%o7
 ld [%o0+8],%l0
 ld [%o0+12],%l1
 ld [%o0+16],%l2
 ld [%o0+20],%l3
 ld [%o0+24],%l4
 ld [%o0+28],%l5
 ld [%o0+32],%l6
 ld [%o0+36],%l7
 ld [%o0+40],%i0
 ld [%o0+44],%i1
 ld [%o0+48],%i2
 ld [%o0+52],%i3
 ld [%o0+56],%i4
 ld [%o0+60],%i5
 ld [%o0+64],%i6
 ld [%o0+68],%i7
 mov %o1,%o0
 tst %o0
 bne 1f
  nop
 mov 1,%o0
1: retl
  nop
.section .note.GNU-stack,"",@progbits
