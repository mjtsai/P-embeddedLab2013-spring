.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.global fork
fork:
	push {r7}
	mov r7, #0x1
	svc 0                   /* -> svc_entry */
    nop
    nop
	bx lr
    nop
.global getpid
getpid:
	push {r7}
	mov r7, #0x2
	svc 0
    nop
    nop
	bx lr
    nop
.global write
write:
	push {r7}
	mov r7, #0x3
	svc 0
    nop
    nop
	bx lr
    nop
.global read
read:
	push {r7}
	mov r7, #0x4
	svc 0
    nop
    nop
	bx lr
    nop
.global interrupt_wait
interrupt_wait:
	push {r7}
	mov r7, #0x5
	svc 0
    nop
    nop
	bx lr
    nop

