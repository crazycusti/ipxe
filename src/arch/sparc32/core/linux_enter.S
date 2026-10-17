/* Stack-free relocation: the kernel replaces the low iPXE image. */
FILE_LICENCE ( GPL2_OR_LATER );
.text
.align 16
.globl sparc32_linux_copy,sparc32_linux_copy_end
sparc32_linux_copy:
 mov %o0,%g7
 rd %psr,%g1
 or %g1,0xf00,%g1
 wr %g1,0,%psr
 nop
 nop
 nop
 ld [%g7],%g2
 add %g7,16,%g3
1:
 ld [%g3],%o1
 ld [%g3+4],%o2
 ld [%g3+8],%o3
 ld [%g3+12],%o4
 sub %o4,%o3,%o4
 mov %o2,%g4
2:
 tst %o3
 bz 3f
  nop
 ldub [%o1],%g1
 stba %g1,[%o2] 0x20
 inc %o1
 inc %o2
 ba 2b
  dec %o3
3:
 tst %o4
 bz 4f
  nop
 stba %g0,[%o2] 0x20
 inc %o2
 ba 3b
  dec %o4
4:
 /* Only kernel destinations use the low identity instruction mapping. */
 set 0x400000,%g5
 cmp %g4,%g5
 bgeu 6f
  andn %g4,7,%g4
5:
 flush %g4
 add %g4,8,%g4
 cmp %g4,%o2
 blu 5b
  nop
6:
 dec %g2
 tst %g2
 bnz 1b
  add %g3,16,%g3
 stbar
 ld [%g7+4],%g1
 ld [%g7+8],%o0
 clr %o1
 jmp %g1
  nop
sparc32_linux_copy_end:
.section .note.GNU-stack,"",@progbits
