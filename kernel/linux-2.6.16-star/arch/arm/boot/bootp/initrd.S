	.type	initrd_start,#object
	.globl	initrd_start
initrd_start:
	.incbin	INITRD
	.globl	initrd_end
initrd_end:

	.align 4
	.global ecksum
ecksum:	
        .word   0x0
        .word   0x0
        .word   0x0
        .word   0x0

