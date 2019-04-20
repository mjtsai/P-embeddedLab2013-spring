#include "FreeRTOSConfig.h"
#include <stddef.h>
#include <stm32f10x.h>
#include <misc.h>

//
void *memcpy(void *dest, const void *src, size_t n)
{
	char *d = dest;
	const char *s = src;
	size_t i;
	for (i = 0; i < n; i++)
		d[i] = s[i];
	return d;
}

int strcmp(const char *a, const char *b)
{
	int r = 0;
	while (!r && *a && *b)
		r = (*a++) - (*b++);
	return (*a) - (*b);
}

size_t strlen(const char *s)
{
	size_t r = 0;
	while (*s++)
		r++;
	return r;
}

void puts(char *s)
{
	while (*s) {
//mj		while (*(UART0 + UARTFR) & UARTFR_TXFF)
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
			/* wait */ ;
//mj		*UART0 = *s;
        USART_SendData(USART2, *s);
		s++;
	}
}

#define STACK_SIZE 512 /* Size of task stacks in words */
#define TASK_LIMIT 8  /* Max number of tasks we can handle */
#define PIPE_BUF   64 /* Size of largest atomic pipe message */
#define PATH_MAX   32 /* Longest absolute path */
#define PIPE_LIMIT (TASK_LIMIT * 2)

#define PATHSERVER_FD (TASK_LIMIT + 3) 
	/* File descriptor of pipe to pathserver */

#define TASK_READY      0
#define TASK_WAIT_READ  1
#define TASK_WAIT_WRITE 2
#define TASK_WAIT_INTR  3

/* 
 * pathserver assumes that all files are FIFOs that were registered
 * with mkfifo.  It also assumes a global tables of FDs shared by all
 * processes.  It would have to get much smarter to be generally useful.
 *
 * The first TASK_LIMIT FDs are reserved for use by their respective tasks.
 * 0-2 are reserved FDs and are skipped.
 * The server registers itself at /sys/pathserver
*/
#define PATH_SERVER_NAME "/sys/pathserver"
void pathserver()
{
//{{{
	char paths[PIPE_LIMIT - TASK_LIMIT - 3][PATH_MAX];
	int npaths = 0;
	int i = 0;
	unsigned int plen = 0;
	unsigned int replyfd = 0;
	char path[PATH_MAX];

	memcpy(paths[npaths++], PATH_SERVER_NAME, sizeof(PATH_SERVER_NAME));

	while (1) {
		read(PATHSERVER_FD, &replyfd, 4);
		read(PATHSERVER_FD, &plen, 4);
		read(PATHSERVER_FD, path, plen);

		if (!replyfd) { /* mkfifo */
			memcpy(paths[npaths++], path, plen);
		}
		else { /* open */
			/* Search for path */
			for (i = 0; i < npaths; i++) {
				if (*paths[i] && strcmp(path, paths[i]) == 0) {
					i += 3; /* 0-2 are reserved */
					i += TASK_LIMIT; /* FDs reserved for tasks */
					write(replyfd, &i, 4);
					i = 0;
					break;
				}
			}

			if (i >= npaths) {
				i = -1; /* Error: not found */
				write(replyfd, &i, 4);
			}
		}
	}
//}}}
}

int mkfifo(const char *pathname, int mode)
{
//{{{
	size_t plen = strlen(pathname)+1;
	char buf[4+4+PATH_MAX];
	(void) mode;

	*((unsigned int *)buf) = 0;
	*((unsigned int *)(buf + 4)) = plen;
	memcpy(buf + 4 + 4, pathname, plen);
	write(PATHSERVER_FD, buf, 4 + 4 + plen);

	return 0;
//}}}
}

int open(const char *pathname, int flags)
{
//{{{
	unsigned int replyfd = getpid() + 3;
	size_t plen = strlen(pathname) + 1;
	unsigned int fd = -1;
	char buf[4 + 4 + PATH_MAX];
	(void) flags;

	*((unsigned int *)buf) = replyfd;
	*((unsigned int *)(buf + 4)) = plen;
	memcpy(buf + 4 + 4, pathname, plen);
	write(PATHSERVER_FD, buf, 4 + 4 + plen);
	read(replyfd, &fd, 4);

	return fd;
//}}}
}

//mj void serialout(volatile unsigned int* uart, unsigned int intr)
void serialout(USART_TypeDef* uart, unsigned int intr)
{
//{{{
	int fd;
	char c;
	int doread = 1;
	mkfifo("/dev/tty0/out", 0);
	fd = open("/dev/tty0/out", 0);

	/* enable TX interrupt on UART */
//mj	*(uart + UARTIMSC) |= UARTIMSC_TXIM;

    USART_ITConfig(uart, USART_IT_TXE, ENABLE);


	while (1) {
		if (doread)
			read(fd, &c, 1);
		doread = 0;
//mj		if (!(*(uart + UARTFR) & UARTFR_TXFF)) {
        if(USART_GetITStatus(uart, USART_IT_TXE) == SET){
//mj			*uart = c;
            USART_SendData(uart, c);   
			doread = 1;
		}
		interrupt_wait(intr);
//mj		*(uart + UARTICR) = UARTICR_TXIC;

	}
//}}}
}

//mj void serialin(volatile unsigned int* uart, unsigned int intr)
void serialin(USART_TypeDef* uart, unsigned int intr)
{
//{{{
	int fd;
	char c;
	mkfifo("/dev/tty0/in", 0);
	fd = open("/dev/tty0/in", 0);

	/* enable RX interrupt on UART */
//mj	*(uart + UARTIMSC) |= UARTIMSC_RXIM;
    USART_ITConfig(uart, USART_IT_RXNE, ENABLE);


	while (1) {
		interrupt_wait(intr);
//mj		*(uart + UARTICR) = UARTICR_RXIC;

//mj		if (!(*(uart + UARTFR) & UARTFR_RXFE)) {
		if (USART_GetITStatus(uart, USART_IT_RXNE) != RESET) {
//mj			c = *uart;
            c = USART_ReceiveData(uart);
			write(fd, &c, 1);
		}
	}
//}}}    
}

void greeting()
{
	int fdout = open("/dev/tty0/out", 0);
	char *string = "Hello, World!\n";
	while (*string) {
		write(fdout, string, 1);
		string++;
	}
}

void echo()
{
//{{{    
	int fdout, fdin;
	char c;
	fdout = open("/dev/tty0/out", 0);
	fdin = open("/dev/tty0/in", 0);

	while (1) {
		read(fdin, &c, 1);
		write(fdout, &c, 1);
	}
//}}}    
}

void first()
{
//{{{
	if (!fork()) pathserver();
//mj	if (!fork()) serialout(UART0, PIC_UART0);
	if (!fork()) serialout(USART2, USART2_IRQn);
//mj	if (!fork()) serialin(UART0, PIC_UART0);
	if (!fork()) serialin(USART2, USART2_IRQn);
	if (!fork()) greeting();
	if (!fork()) echo();

	while(1);
//}}}    
}

struct pipe_ringbuffer {
	int start;
	int end;
	char data[PIPE_BUF];
};

#define RB_PUSH(rb, size, v) do { \
		(rb).data[(rb).end] = (v); \
		(rb).end++; \
		if ((rb).end > size) (rb).end = 0; \
	} while (0)

#define RB_POP(rb, size, v) do { \
		(v) = (rb).data[(rb).start]; \
		(rb).start++; \
		if ((rb).start > size) (rb).start = 0; \
	} while (0)

#define RB_LEN(rb, size) (((rb).end - (rb).start) + \
	(((rb).end < (rb).start) ? size : 0))

#define PIPE_PUSH(pipe, v) RB_PUSH((pipe), PIPE_BUF, (v))
#define PIPE_POP(pipe, v)  RB_POP((pipe), PIPE_BUF, (v))
#define PIPE_LEN(pipe)     (RB_LEN((pipe), PIPE_BUF))

unsigned int *init_task(unsigned int *stack, void (*start)())
{
	stack += STACK_SIZE - 16; /* End of stack, minus what we're about to push */
	stack[7] = 0x10;                    // reserved
	stack[8] = (unsigned int)start;
	return stack;
}

void _read(unsigned int *task, unsigned int **tasks, size_t task_count, struct pipe_ringbuffer *pipes);
void _write(unsigned int *task, unsigned int **tasks, size_t task_count, struct pipe_ringbuffer *pipes);

void _read(unsigned int *task, unsigned int **tasks, size_t task_count, struct pipe_ringbuffer *pipes)
{
//{{{    
	task[-1] = TASK_READY;
	/* If the fd is invalid, or trying to read too much  */
	if (task[0] > PIPE_LIMIT || task[2] > PIPE_BUF) {
		task[0] = -1;
	}
	else {
		struct pipe_ringbuffer *pipe = &pipes[task[0]];
		if ((size_t)PIPE_LEN(*pipe) < task[2]) {
			/* Trying to read more than there is: block */
			task[-1] = TASK_WAIT_READ;
		}
		else {
			size_t i;
			char *buf = (char*)task[1];
			/* Copy data into buf */
			for (i = 0; i < task[2]; i++) {
				PIPE_POP(*pipe, buf[i]);
			}

			/* Unblock any waiting writes */
			for (i = 0; i < task_count; i++)
				if (tasks[i][-1] == TASK_WAIT_WRITE)
					_write(tasks[i], tasks, task_count, pipes);
		}
	}
//}}}    
}

void _write(unsigned int *task, unsigned int **tasks, size_t task_count, struct pipe_ringbuffer *pipes)
{
//{{{    
	/* If the fd is invalid or the write would be non-atomic */
	if (task[0] > PIPE_LIMIT || task[2] > PIPE_BUF) {
		task[0] = -1;
	}
	else {
		struct pipe_ringbuffer *pipe = &pipes[task[0]];

		if ((size_t)PIPE_BUF - PIPE_LEN(*pipe) < task[2]) {
			/* Trying to write more than we have space for: block */
			task[-1] = TASK_WAIT_WRITE;
		}
		else {
			size_t i;
			const char *buf = (const char*)task[1];
			/* Copy data into pipe */
			for (i = 0; i < task[2]; i++)
				PIPE_PUSH(*pipe,buf[i]);

			/* Unblock any waiting reads */
			for (i = 0; i < task_count; i++)
				if (tasks[i][-1] == TASK_WAIT_READ)
					_read(tasks[i], tasks, task_count, pipes);
		}
	}
//}}}    
}

int main()
{
	unsigned int stacks[TASK_LIMIT][STACK_SIZE];
	unsigned int *tasks[TASK_LIMIT];
	struct pipe_ringbuffer pipes[PIPE_LIMIT];
	size_t task_count = 0;
	size_t current_task = 0;
	size_t i;

	SysTick_Config(configCPU_CLOCK_HZ / configTICK_RATE_HZ);

	init_rs232();
	__enable_irq();

    // enable timer
//mj	*(PIC + VIC_INTENABLE) = PIC_TIMER01;
//mj
//mj	*TIMER0 = 10000;        // counter
//mj	*(TIMER0 + TIMER_CONTROL) = TIMER_EN | TIMER_PERIODIC
//mj	                            | TIMER_32BIT | TIMER_INTEN;

    // freertos
//mj    *(portNVIC_SYSTICK_LOAD) = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
//mj	*(portNVIC_SYSTICK_CTRL) = portNVIC_SYSTICK_CLK | portNVIC_SYSTICK_INT | portNVIC_SYSTICK_ENABLE;
    

    //
	tasks[task_count] = init_task(stacks[task_count], &first);
	task_count++;

	/* Initialize all pipes */
	for (i = 0; i < PIPE_LIMIT; i++)
		pipes[i].start = pipes[i].end = 0;



	while (1) {
		tasks[current_task] = activate(tasks[current_task]);
		tasks[current_task][-1] = TASK_READY;

		switch (tasks[current_task][9]) {
		case 0x1: /* fork */
			if (task_count == TASK_LIMIT) {
				/* Cannot create a new task, return error */
				tasks[current_task][0] = -1;
			}
			else {
				/* Compute how much of the stack is used */
				size_t used = stacks[current_task] + STACK_SIZE
					      - tasks[current_task];
				/* New stack is END - used */
				tasks[task_count] = stacks[task_count] + STACK_SIZE - used;
				/* Copy only the used part of the stack */
				memcpy(tasks[task_count], tasks[current_task],
				       used * sizeof(*tasks[current_task]));
				/* Set return values in each process */
				tasks[current_task][0] = task_count;
				tasks[task_count][0] = 0;
				/* There is now one more task */
				task_count++;
			}
			break;
		case 0x2: /* getpid */
			tasks[current_task][0] = current_task;
			break;
		case 0x3: /* write */
			_write(tasks[current_task], tasks, task_count, pipes);
			break;
		case 0x4: /* read */
			_read(tasks[current_task], tasks, task_count, pipes);
			break;
		case 0x5: /* interrupt_wait */
			/* Enable interrupt */
//mj			*(PIC + VIC_INTENABLE) = tasks[current_task][2 + 0];
            NVIC_EnableIRQ(tasks[current_task][0]);
            /* Block task waiting for interrupt to happen */
			tasks[current_task][-1] = TASK_WAIT_INTR;
			break;
		default: /* Catch all interrupts */
			if ((int)tasks[current_task][9] < 0) {      // **negative number** to handle so called exception return(cm3 arch)
				unsigned int intr = (1 << -tasks[current_task][9]);

//mj				if (intr == PIC_TIMER01) {
				if (intr == SysTick_IRQn) {
					/* Never disable timer. We need it for pre-emption */
//??					if (*(TIMER0 + TIMER_MIS)) { /* Timer0 went off */      // status
//??						*(TIMER0 + TIMER_INTCLR) = 1; /* Clear interrupt */
//??					}
				}
				else {
					/* Disable interrupt, interrupt_wait re-enables */
//mj					*(PIC + VIC_INTENCLEAR) = intr;
                    NVIC_DisableIRQ(intr);
				}
				/* Unblock any waiting tasks */
				for (i = 0; i < task_count; i++)
					if (tasks[i][-1] == TASK_WAIT_INTR && tasks[i][0] == intr)
						tasks[i][-1] = TASK_READY;
			}
		}

		/* Select next TASK_READY task */
		while (TASK_READY != tasks[current_task =
			(current_task+1 >= task_count ? 0 : current_task+1)][-1]);
	}

	return 0;
}

void vApplicationTickHook()
{
}
void vApplicationIdleHook()
{
    puts("idle\n\r");
}
void my_switched_out_task()
{
}
void my_switched_in_task()
{
}
