.syntax unified
	
.type	SysTick_Handler, %function
.global SysTick_Handler
.type	USART2_IRQHandler, %function
.global USART2_IRQHandler
SysTick_Handler:
USART2_IRQHandler:
	mrs r9, psp
	stmdb r9!, {r7}

	/* Get ISR number */
	mrs r7, ipsr
	neg r7, r7

	/* save user state */
	stmdb r9!, {r0, r1, r2, r3, r4, r5, r6, r7, r8}    
	stmdb r9!, {ip, lr}    
    mov r0, r9

	/* load kernel state */
	pop {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
	msr psr, ip

	bx lr
    nop
.type	SVC_Handler, %function   
.global SVC_Handler
SVC_Handler:
	/* save user state */
	mrs r9, psp
	stmdb r9!, {r7}    
	stmdb r9!, {r0, r1, r2, r3, r4, r5, r6, r7, r8}    
	stmdb r9!, {ip, lr}    
    mov r0, r9

	/* load kernel state */
	pop {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}              /* use msp */
	msr psr, ip
	
	bx lr
    nop
    /* use psp */
.global activate
activate:
	/* save kernel state */
	mrs ip, psr
	push {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
	
	/* load user state */
    mov r9, r0
    ldmfd r9!, {ip, lr}
    ldmfd r9!, {r0, r1, r2, r3, r4, r5, r6, r7, r8}
    ldmfd r9!, {r7}

	/* switch to process stack pointer */
	msr psp, r9
	mov r0, #3
	msr control, r0

	bx lr    
    nop
