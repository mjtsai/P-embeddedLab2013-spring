#include <stddef.h>

#include "versatilepb.h"
#include "asm.h"

#define portSTACK_TYPE  unsigned int
#define portBASE_TYPE   int
typedef struct tskTaskControlBlock
{
	volatile portSTACK_TYPE	*pxTopOfStack   ;	/* should be r0 , current pointer */	
	unsigned portBASE_TYPE	uxPriority      ;			
	portSTACK_TYPE			*pxStack        ;	/* a single stack start */
    volatile portSTACK_TYPE state           ;

} tskTCB;



void *memcpy(void *dest, const void *src, size_t n)
{
/*{{{*/    
	char *d = dest;
	const char *s = src;
	size_t i;
	for (i = 0; i < n; i++)
		d[i] = s[i];
	return d;
/*}}}*/    
}

int strcmp(const char *a, const char *b)
{
/*{{{*/    
	int r = 0;
	while (!r && *a && *b)
		r = (*a++) - (*b++);
	return (*a) - (*b);
/*}}}*/    
}

size_t strlen(const char *s)
{
/*{{{*/    
	size_t r = 0;
	while (*s++)
		r++;
	return r;
/*}}}*/    
}

void puts(char *s)
{
/*{{{*/    
	while (*s) {
		while (*(UART0 + UARTFR) & UARTFR_TXFF)
			/* wait */ ;
		*UART0 = *s;
		s++;
	}
/*}}}*/    
}

#define STACK_SIZE 1024 /* Size of task stacks in words */
#define TASK_LIMIT 10  /* Max number of tasks we can handle */
#define PIPE_BUF   512 /* Size of largest atomic pipe message */
#define PATH_MAX   255 /* Longest absolute path */
#define PIPE_LIMIT (TASK_LIMIT * 5)

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
/*{{{*/    
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
/*}}}*/    
}

int mkfifo(const char *pathname, int mode)
{
/*{{{*/    
	size_t plen = strlen(pathname)+1;
	char buf[4+4+PATH_MAX];
	(void) mode;

	*((unsigned int *)buf) = 0;
	*((unsigned int *)(buf + 4)) = plen;
	memcpy(buf + 4 + 4, pathname, plen);
	write(PATHSERVER_FD, buf, 4 + 4 + plen);

	return 0;
/*}}}*/    
}

int open(const char *pathname, int flags)
{
/*{{{*/    
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
/*}}}*/
}

void serialout(volatile unsigned int* uart, unsigned int intr)
{
/*{{{*/    
	int fd;
	char c;
	int doread = 1;
	mkfifo("/dev/tty0/out", 0);
	fd = open("/dev/tty0/out", 0);

	/* enable TX interrupt on UART */
	*(uart + UARTIMSC) |= UARTIMSC_TXIM;

	while (1) {
		if (doread)
			read(fd, &c, 1);
		doread = 0;
		if (!(*(uart + UARTFR) & UARTFR_TXFF)) {
			*uart = c;
			doread = 1;
		}
		interrupt_wait(intr);
		*(uart + UARTICR) = UARTICR_TXIC;
	}
/*}}}*/    
}

void serialin(volatile unsigned int* uart, unsigned int intr)
{
/*{{{*/    
	int fd;
	char c;
    char haha = '[';
	mkfifo("/dev/tty0/in", 0);
	fd = open("/dev/tty0/in", 0);

	/* enable RX interrupt on UART */
	*(uart + UARTIMSC) |= UARTIMSC_RXIM;

	while (1) {
		interrupt_wait(intr);
		*(uart + UARTICR) = UARTICR_RXIC;
		if (!(*(uart + UARTFR) & UARTFR_RXFE)) {
			c = *uart;
			write(fd, &c, 1);
			write(fd, &haha, 1);
			write(fd, &haha, 1);
			write(fd, &haha, 1);
		}
	}
/*}}}*/    
}

void greeting()
{
/*{{{*/    
	int fdout = open("/dev/tty0/out", 0);
	char *string = "Hello, World!\n";
	while (*string) {
		write(fdout, string, 1);
		string++;
	}
/*}}}*/    
}

void echo()
{
/*{{{*/    
	int fdout, fdin;
	char c;
	fdout = open("/dev/tty0/out", 0);
	fdin = open("/dev/tty0/in", 0);

	while (1) {
		read(fdin, &c, 1);          /* r0, r1, r2 */
		write(fdout, &c, 1);
	}
/*}}}*/    
}

void first()
{
	if (!fork(0)) pathserver();
	if (!fork(0)) serialout(UART0, PIC_UART0);
	if (!fork(0)) serialin(UART0, PIC_UART0);
	if (!fork(0)) greeting();
	if (!fork(0)) echo();

	while(1);
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

portSTACK_TYPE *init_task(portSTACK_TYPE *stack, void (*start)())
{
	stack += STACK_SIZE - 16; /* End of stack, minus what we're about to push */
	stack[0] = 0x10; /* User mode, interrupts on */
	stack[1] = (unsigned int)start;
	return stack;
}

void _read(tskTCB *task, tskTCB *tasks, size_t task_count, struct pipe_ringbuffer *pipes);
void _write(tskTCB *task, tskTCB *tasks, size_t task_count, struct pipe_ringbuffer *pipes);

void _read(tskTCB *task, tskTCB *tasks, size_t task_count, struct pipe_ringbuffer *pipes)
{
/*{{{*/    
	task->state = TASK_READY;
	/* If the fd is invalid, or trying to read too much  */
	if (task->pxTopOfStack[2+0] > PIPE_LIMIT || task->pxTopOfStack[2+2] > PIPE_BUF) {
		task->pxTopOfStack[2+0] = -1;
	}
	else {
		struct pipe_ringbuffer *pipe = &pipes[task->pxTopOfStack[2+0]];
		if ((size_t)PIPE_LEN(*pipe) < task->pxTopOfStack[2+2]) {
			/* Trying to read more than there is: block */
			task->state = TASK_WAIT_READ;
		}
		else {
			size_t i;
			char *buf = (char*)task->pxTopOfStack[2+1];
			/* Copy data into buf */
			for (i = 0; i < task->pxTopOfStack[2+2]; i++) {
				PIPE_POP(*pipe, buf[i]);
			}

			/* Unblock any waiting writes */
			for (i = 0; i < task_count; i++)
				if (tasks[i].state == TASK_WAIT_WRITE)
					_write(&tasks[i], tasks, task_count, pipes);
		}
	}
/*}}}*/    
}

void _write(tskTCB *task, tskTCB *tasks, size_t task_count, struct pipe_ringbuffer *pipes)
{
/*{{{*/    
	/* If the fd is invalid or the write would be non-atomic */
	if (task->pxTopOfStack[2 + 0] > PIPE_LIMIT || task->pxTopOfStack[2 + 2] > PIPE_BUF) {
		task->pxTopOfStack[2 + 0] = -1;
	}
	else {
		struct pipe_ringbuffer *pipe = &pipes[task->pxTopOfStack[2 + 0]];

		if ((size_t)PIPE_BUF - PIPE_LEN(*pipe) < task->pxTopOfStack[2 + 2]) {
			/* Trying to write more than we have space for: block */
			task->state = TASK_WAIT_WRITE;
		}
		else {
			size_t i;
			const char *buf = (const char*)task->pxTopOfStack[2+1];
			/* Copy data into pipe */
			for (i = 0; i < task->pxTopOfStack[2 + 2]; i++)
				PIPE_PUSH(*pipe,buf[i]);

			/* Unblock any waiting reads */
			for (i = 0; i < task_count; i++)
				if (tasks[i].state == TASK_WAIT_READ)
					_read(&tasks[i], tasks, task_count, pipes);
		}
	}
/*}}}*/    
}

int main()
{
	unsigned int    stacks[TASK_LIMIT][STACK_SIZE];
	tskTCB          tasks[TASK_LIMIT];                            /* task structure, point to head of a hidden task structure as r0 */
	struct          pipe_ringbuffer pipes[PIPE_LIMIT];
	size_t          task_count = 0;
	size_t          current_task = 0;
	size_t          i;
    int             max_priority = 0, current_priority = 0;

	*(PIC + VIC_INTENABLE) = PIC_TIMER01;

	*TIMER0 = 10000;
	*(TIMER0 + TIMER_CONTROL) = TIMER_EN | TIMER_PERIODIC
	                            | TIMER_32BIT | TIMER_INTEN;

    tasks[task_count].pxStack = stacks[task_count];
	tasks[task_count].pxTopOfStack = init_task(tasks[task_count].pxStack, &first);  /* register first() in 1st task , return stack head */
    tasks[task_count].uxPriority = 0;
	task_count++;

	/* Initialize all pipes */
	for (i = 0; i < PIPE_LIMIT; i++)
		pipes[i].start = pipes[i].end = 0;

	while (1) { /* this is kernel , task manager */
		tasks[current_task].pxTopOfStack = activate(tasks[current_task].pxTopOfStack);    /* parameter as r0, do the task -> implicit system calls , return stack head */
        /* <-- kernel state lr return */
		tasks[current_task].state = TASK_READY;            

		switch (tasks[current_task].pxTopOfStack[2 + 7]) {
		case 0x1: /* fork */
			if (task_count == TASK_LIMIT) {
				/* Cannot create a new task, return error */
				tasks[current_task].pxTopOfStack[2 + 0] = -1;
			}
			else {
				/* Compute how much of the stack is used */
				size_t used = stacks[current_task] + STACK_SIZE
					      - tasks[current_task].pxTopOfStack;
				/* New stack is END - used */
				tasks[task_count].pxTopOfStack = stacks[task_count] + STACK_SIZE - used;
                tasks[task_count].pxStack = stacks[task_count];
				/* Copy only the used part of the stack */
				memcpy(tasks[task_count].pxTopOfStack, tasks[current_task].pxTopOfStack,
				       used * sizeof(*tasks[current_task].pxTopOfStack));
                tasks[task_count].uxPriority = tasks[current_task].pxTopOfStack[2 + 0];     /* pass priority */
                if(tasks[task_count].uxPriority > max_priority) max_priority = tasks[task_count].uxPriority;
				/* Set return values in each process */
				tasks[current_task].pxTopOfStack[2 + 0] = task_count;
				tasks[task_count].pxTopOfStack[2 + 0] = 0;               /* for !fork() = r0 */
				/* There is now one more task */
				task_count++;
			}
			break;
		case 0x2: /* getpid */
			tasks[current_task].pxTopOfStack[2 + 0] = current_task;
			break;
		case 0x3: /* write */
			_write(&tasks[current_task], tasks, task_count, pipes);
			break;
		case 0x4: /* read */
			_read(&tasks[current_task], tasks, task_count, pipes);
			break;
		case 0x5: /* interrupt_wait */
			/* Enable interrupt */
			*(PIC + VIC_INTENABLE) = tasks[current_task].pxTopOfStack[2 + 0];
			/* Block task waiting for interrupt to happen */
			tasks[current_task].state = TASK_WAIT_INTR;
			break;
		default: /* Catch all interrupts */
			if ((int)tasks[current_task].pxTopOfStack[2 + 7] < 0) {
				unsigned int intr = (1 << -tasks[current_task].pxTopOfStack[2+7]);

				if (intr == PIC_TIMER01) {
					/* Never disable timer. We need it for pre-emption */
					if (*(TIMER0 + TIMER_MIS)) { /* Timer0 went off */
						*(TIMER0 + TIMER_INTCLR) = 1; /* Clear interrupt */
					}
				}
				else {
					/* Disable interrupt, interrupt_wait re-enables */
					*(PIC + VIC_INTENCLEAR) = intr;
				}
				/* Unblock any waiting tasks */
				for (i = 0; i < task_count; i++)
					if (tasks[i].state == TASK_WAIT_INTR && tasks[i].pxTopOfStack[2+0] == intr)
						tasks[i].state = TASK_READY;
			}
		}

		/* Select next TASK_READY task */
/*		while (TASK_READY != tasks[current_task = (current_task+1 >= task_count ? 0 : current_task+1)].state); */

        current_priority = max_priority;
        while(current_priority >= 0){
    		for(i=0; i<task_count; i++){
                current_task = (current_task+1 >= task_count ? 0 : current_task+1);                
                if( TASK_READY == tasks[current_task].state && tasks[current_task].uxPriority == current_priority ){
                    break;
                }
            }
            if(i!=task_count) break;
            current_priority--;
        }
        
	}

	return 0;
}
