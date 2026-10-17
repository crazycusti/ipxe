FILE_LICENCE ( GPL2_OR_LATER );
.level 1.1
.section .text.pdc_start,"ax",@progbits
.export _pdc_start,entry
_pdc_start:
 b,n 1f
 .word 0
.align 16
1:
 rsm 1,%r0
 ldil L%($global$),%dp
 ldo R%($global$)(%dp),%dp
 ldil L%(_pdc_stack),%sp
 ldo R%(_pdc_stack)(%sp),%sp
 ldo 64(%sp),%sp
 ldil L%(_bss),%r4
 ldo R%(_bss)(%r4),%r4
 ldil L%(_ebss),%r5
 ldo R%(_ebss)(%r5),%r5
2:
 comb,>= %r4,%r5,3f
 nop
 stw %r0,0(%r4)
 b 2b
 ldo 4(%r4),%r4
3:
 .import pdc_platform_start,code
 bl pdc_platform_start,%r2
 nop
4:
 b 4b
 nop
.section .note.GNU-stack,"",@progbits
