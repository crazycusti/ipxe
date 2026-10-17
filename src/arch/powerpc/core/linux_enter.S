FILE_LICENCE ( GPL2_OR_LATER );
.text
.globl of_linux_enter
of_linux_enter:
 mr 12,3
 mr 3,4
 mr 4,5
 mr 5,6
 mr 10,7
 li 6,0
 li 7,0
 mtsrr0 12
 mtsrr1 10
 rfi
.section .note.GNU-stack,"",@progbits
