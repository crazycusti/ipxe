FILE_LICENCE ( GPL2_OR_LATER );
.text
.align 4
.globl setjmp
setjmp:
 flushw
 stx %sp,[%o0]
 stx %o7,[%o0+8]
 stx %l0,[%o0+16]
 stx %l1,[%o0+24]
 stx %l2,[%o0+32]
 stx %l3,[%o0+40]
 stx %l4,[%o0+48]
 stx %l5,[%o0+56]
 stx %l6,[%o0+64]
 stx %l7,[%o0+72]
 stx %i0,[%o0+80]
 stx %i1,[%o0+88]
 stx %i2,[%o0+96]
 stx %i3,[%o0+104]
 stx %i4,[%o0+112]
 stx %i5,[%o0+120]
 stx %i6,[%o0+128]
 stx %i7,[%o0+136]
 retl
  clr %o0
.globl longjmp
longjmp:
 flushw
 ldx [%o0],%sp
 ldx [%o0+8],%o7
 ldx [%o0+16],%l0
 ldx [%o0+24],%l1
 ldx [%o0+32],%l2
 ldx [%o0+40],%l3
 ldx [%o0+48],%l4
 ldx [%o0+56],%l5
 ldx [%o0+64],%l6
 ldx [%o0+72],%l7
 ldx [%o0+80],%i0
 ldx [%o0+88],%i1
 ldx [%o0+96],%i2
 ldx [%o0+104],%i3
 ldx [%o0+112],%i4
 ldx [%o0+120],%i5
 ldx [%o0+128],%i6
 ldx [%o0+136],%i7
 mov %o1,%o0
 cmp %o0,0
 move %icc,1,%o0
 retl
  nop
.section .note.GNU-stack,"",@progbits
