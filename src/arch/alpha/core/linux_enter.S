FILE_LICENCE ( GPL2_OR_LATER );
.text
.globl alpha_linux_enter
.ent alpha_linux_enter
alpha_linux_enter:
 mov $16,$27
 lda $30,-0xe000($27)
 jmp $31,($27),0
.end alpha_linux_enter
.section .note.GNU-stack,"",@progbits
