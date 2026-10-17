/* Position-independent, stack-free copy trampoline: Linux's decompressor
 * overwrites the original low-memory iPXE text and stack. */
FILE_LICENCE ( GPL2_OR_LATER );
.level 1.1
.text
.align 16
.export pdc_linux_copy,code
.export pdc_linux_copy_end,code
pdc_linux_copy:
 copy %r26,%r20
 ldw 0(%r20),%r19
 ldo 32(%r20),%r18
1:
 ldw 0(%r18),%r21
 ldw 4(%r18),%r22
 ldw 8(%r18),%r23
 ldw 12(%r18),%r24
 sub %r24,%r23,%r24
 copy %r22,%r17
2:
 comb,= %r23,%r0,3f
 nop
 ldb 0(%r21),%r1
 stb %r1,0(%r22)
 ldo 1(%r21),%r21
 ldo 1(%r22),%r22
 b 2b
 ldo -1(%r23),%r23
3:
 comb,= %r24,%r0,4f
 nop
 stb %r0,0(%r22)
 ldo 1(%r22),%r22
 b 3b
 ldo -1(%r24),%r24
4:
 /* Flush all destination cache lines before instruction fetch. */
 depi 0,31,5,%r17
5:
 fdc 0(%r17)
 fic 0(%sr0,%r17)
 ldo 32(%r17),%r17
 comb,< %r17,%r22,5b
 nop
 ldo 16(%r18),%r18
 addib,> -1,%r19,1b
 nop
 sync
 ldw 4(%r20),%r1
 ldw 8(%r20),%r26
 ldw 12(%r20),%r25
 ldw 16(%r20),%r24
 ldw 20(%r20),%r23
 bv,n 0(%r1)
pdc_linux_copy_end:
.section .note.GNU-stack,"",@progbits
