FILE_LICENCE ( GPL2_OR_LATER );
.text
.align 4
.globl __bswapsi2
__bswapsi2:
 srl %o0,24,%o1
 srl %o0,8,%o2
 set 0x0000ff00,%o3
 and %o2,%o3,%o2
 or %o1,%o2,%o1
 sll %o0,8,%o2
 set 0x00ff0000,%o3
 and %o2,%o3,%o2
 or %o1,%o2,%o1
 sll %o0,24,%o0
 retl
  or %o0,%o1,%o0
.section .note.GNU-stack,"",@progbits
