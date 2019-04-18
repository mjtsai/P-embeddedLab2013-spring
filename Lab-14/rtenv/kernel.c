#include "stm32f10x.h"
#include "RTOSConfig.h"

#include "syscall.h"

#include <stddef.h>

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


int strncmp(const char *a, const char *b, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		if (a[i] != b[i])
			return a[i] - b[i];

	return 0;
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
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
			/* wait */ ;
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

#define PRIORITY_DEFAULT 20
#define PRIORITY_LIMIT (PRIORITY_DEFAULT * 2 - 1)

#define TASK_READY      0
#define TASK_WAIT_READ  1
#define TASK_WAIT_WRITE 2
#define TASK_WAIT_INTR  3
#define TASK_WAIT_TIME  4

#define S_IFIFO 1
#define S_IMSGQ 2

/*Global Variables*/
char next_line[3] = {'\n','\r','\0'};
size_t task_count = 0;
int fdout;
int fdin;


/* Stack struct of user thread, see "Exception entry and return" */
struct user_thread_stack {
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int fp;
	unsigned int _lr;	/* Back to system calls or return exception */
	unsigned int _r7;	/* Backup from isr */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int ip;
	unsigned int lr;	/* Back to user thread code */
	unsigned int pc;
	unsigned int xpsr;
	unsigned int stack[STACK_SIZE - 18];
};

/* Task Control Block */
struct task_control_block {
    struct user_thread_stack *stack;
    int pid;
    int status;
    struct task_control_block **prev;
    struct task_control_block  *next;
};
struct task_control_block tasks[TASK_LIMIT];

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
			int dev;
			read(PATHSERVER_FD, &dev, 4);
			memcpy(paths[npaths], path, plen);
			mknod(npaths + 3 + TASK_LIMIT, 0, dev);
			npaths++;
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
}

int mkfile(const char *pathname, int mode, int dev)
{
	size_t plen = strlen(pathname)+1;
	char buf[4+4+PATH_MAX+4];
	(void) mode;

	*((unsigned int *)buf) = 0;
	*((unsigned int *)(buf + 4)) = plen;
	memcpy(buf + 4 + 4, pathname, plen);
	*((int *)(buf + 4 + 4 + plen)) = dev;
	write(PATHSERVER_FD, buf, 4 + 4 + plen + 4);

	return 0;
}

int mkfifo(const char *pathname, int mode)
{
	mkfile(pathname, mode, S_IFIFO);
	return 0;
}

int open(const char *pathname, int flags)
{
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
}


void serialout(USART_TypeDef* uart, unsigned int intr)
{
	int fd;
	char c;
	int doread = 1;
	mkfifo("/dev/tty0/out", 0);
	fd = open("/dev/tty0/out", 0);

	while (1) {
		if (doread)
			read(fd, &c, 1);
		doread = 0;
		if (USART_GetFlagStatus(uart, USART_FLAG_TXE) == SET) {
			USART_SendData(uart, c);
			USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
			doread = 1;
		}
		interrupt_wait(intr);
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	}
}

void serialin(USART_TypeDef* uart, unsigned int intr)
{
	int fd;
	char c;
	mkfifo("/dev/tty0/in", 0);
	fd = open("/dev/tty0/in", 0);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	while (1) {
		interrupt_wait(intr);
		if (USART_GetFlagStatus(uart, USART_FLAG_RXNE) == SET) {
			c = USART_ReceiveData(uart);
			write(fd, &c, 1);
		}
	}
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
	int fdout;
	int fdin;
	char c;

	fdout = open("/dev/tty0/out", 0);
	fdin = open("/dev/tty0/in", 0);

	while (1) {
		read(fdin, &c, 1);
		write(fdout, &c, 1);
	}
}

//this function helps to show int

void itoa(int n, char *dst, int base)
{
	char buf[33] = {0};
	char *p = &buf[32];

	if (n == 0)
		*--p = '0';
	else {
		char *q;
		unsigned int num = (base == 10 && n < 0) ? -n : n;

		for (; num; num/=base)
			*--p = "0123456789ABCDEF" [num % base];
		if (base == 10 && n < 0)
			*--p = '-';
	}

	strcpy(dst, p);
}

void first()
{

	if (!fork()) pathserver();
	if (!fork()) serialout(USART2, USART2_IRQn);
	if (!fork()) serialin(USART2, USART2_IRQn);
	if (!fork()) greeting();
	if (!fork()) echo();


	while(1);
}

struct pipe_ringbuffer {
	int start;
	int end;
	char data[PIPE_BUF];

	int (*readable) (struct pipe_ringbuffer*, struct task_control_block*);
	int (*writable) (struct pipe_ringbuffer*, struct task_control_block*);
	int (*read) (struct pipe_ringbuffer*, struct task_control_block*);
	int (*write) (struct pipe_ringbuffer*, struct task_control_block*);
};

#define RB_PUSH(rb, size, v) do { \
		(rb).data[(rb).end] = (v); \
		(rb).end++; \
		if ((rb).end >= size) (rb).end = 0; \
	} while (0)

#define RB_POP(rb, size, v) do { \
		(v) = (rb).data[(rb).start]; \
		(rb).start++; \
		if ((rb).start >= size) (rb).start = 0; \
	} while (0)

#define RB_PEEK(rb, size, v, i) do { \
		int _counter = (i); \
		int _src_index = (rb).start; \
		int _dst_index = 0; \
		while (_counter--) { \
			((char*)&(v))[_dst_index++] = (rb).data[_src_index++]; \
			if (_src_index >= size) _src_index = 0; \
		} \
	} while (0)

#define RB_LEN(rb, size) (((rb).end - (rb).start) + \
	(((rb).end < (rb).start) ? size : 0))

#define PIPE_PUSH(pipe, v) RB_PUSH((pipe), PIPE_BUF, (v))
#define PIPE_POP(pipe, v)  RB_POP((pipe), PIPE_BUF, (v))
#define PIPE_PEEK(pipe, v, i)  RB_PEEK((pipe), PIPE_BUF, (v), (i))
#define PIPE_LEN(pipe)     (RB_LEN((pipe), PIPE_BUF))

unsigned int *init_task(unsigned int *stack, void (*start)())
{
	stack += STACK_SIZE - 9; /* End of stack, minus what we're about to push */
	stack[8] = (unsigned int)start;
	return stack;
}

int
task_push (struct task_control_block **list, struct task_control_block *item)
{
	if (list && item) {
		/* Remove itself from original list */
		if (item->prev)
			*(item->prev) = item->next;
		if (item->next)
			item->next->prev = item->prev;
		/* Insert into new list */
		while (*list) list = &((*list)->next);
		*list = item;
		item->prev = list;
		item->next = NULL;
		return 0;
	}
	return -1;
}

struct task_control_block*
task_pop (struct task_control_block **list)
{
	if (list) {
		struct task_control_block *item = *list;
		if (item) {
			*list = item->next;
			if (item->next)
				item->next->prev = list;
			item->prev = NULL;
			item->next = NULL;
			return item;
		}
	}
	return NULL;
}

void _read(struct task_control_block *task, struct task_control_block *tasks, size_t task_count, struct pipe_ringbuffer *pipes);
void _write(struct task_control_block *task, struct task_control_block *tasks, size_t task_count, struct pipe_ringbuffer *pipes);

void _read(struct task_control_block *task, struct task_control_block *tasks, size_t task_count, struct pipe_ringbuffer *pipes)
{
	task->status = TASK_READY;
	/* If the fd is invalid */
	if (task->stack->r0 > PIPE_LIMIT) {
		task->stack->r0 = -1;
	}
	else {
		struct pipe_ringbuffer *pipe = &pipes[task->stack->r0];

		if (pipe->readable(pipe, task)) {
			size_t i;

			pipe->read(pipe, task);

			/* Unblock any waiting writes */
			for (i = 0; i < task_count; i++)
				if (tasks[i].status == TASK_WAIT_WRITE)
					_write(&tasks[i], tasks, task_count, pipes);
		}
	}
}

void _write(struct task_control_block *task, struct task_control_block *tasks, size_t task_count, struct pipe_ringbuffer *pipes)
{
	task->status = TASK_READY;
	/* If the fd is invalid */
	if (task->stack->r0 > PIPE_LIMIT) {
		task->stack->r0 = -1;
	}
	else {
		struct pipe_ringbuffer *pipe = &pipes[task->stack->r0];

		if (pipe->writable(pipe, task)) {
			size_t i;

			pipe->write(pipe, task);

			/* Unblock any waiting reads */
			for (i = 0; i < task_count; i++)
				if (tasks[i].status == TASK_WAIT_READ)
					_read(&tasks[i], tasks, task_count, pipes);
		}
	}
}

int
fifo_readable (struct pipe_ringbuffer *pipe,
			   struct task_control_block *task)
{
	/* Trying to read too much */
	if (task->stack->r2 > PIPE_BUF) {
		task->stack->r0 = -1;
		return 0;
	}
	if ((size_t)PIPE_LEN(*pipe) < task->stack->r2) {
		/* Trying to read more than there is: block */
		task->status = TASK_WAIT_READ;
		return 0;
	}
	return 1;
}

int
mq_readable (struct pipe_ringbuffer *pipe,
			 struct task_control_block *task)
{
	size_t msg_len;

	/* Trying to read too much */
	if ((size_t)PIPE_LEN(*pipe) < sizeof(size_t)) {
		/* Nothing to read */
		task->status = TASK_WAIT_READ;
		return 0;
	}

	PIPE_PEEK(*pipe, msg_len, 4);

	if (msg_len > task->stack->r2) {
		/* Trying to read more than buffer size */
		task->stack->r0 = -1;
		return 0;
	}
	return 1;
}

int
fifo_read (struct pipe_ringbuffer *pipe,
		   struct task_control_block *task)
{
	size_t i;
	char *buf = (char*)task->stack->r1;
	/* Copy data into buf */
	for (i = 0; i < task->stack->r2; i++) {
		PIPE_POP(*pipe, buf[i]);
	}
	return task->stack->r2;
}

int
mq_read (struct pipe_ringbuffer *pipe,
		 struct task_control_block *task)
{
	size_t msg_len;
	size_t i;
	char *buf = (char*)task->stack->r1;
	/* Get length */
	for (i = 0; i < 4; i++) {
		PIPE_POP(*pipe, *(((char*)&msg_len)+i));
	}
	/* Copy data into buf */
	for (i = 0; i < msg_len; i++) {
		PIPE_POP(*pipe, buf[i]);
	}
	return msg_len;
}

int
fifo_writable (struct pipe_ringbuffer *pipe,
			   struct task_control_block *task)
{
	/* If the write would be non-atomic */
	if (task->stack->r2 > PIPE_BUF) {
		task->stack->r0 = -1;
		return 0;
	}
	/* Preserve 1 byte to distiguish empty or full */
	if ((size_t)PIPE_BUF - PIPE_LEN(*pipe) - 1 < task->stack->r2) {
		/* Trying to write more than we have space for: block */
		task->status = TASK_WAIT_WRITE;
		return 0;
	}
	return 1;
}

int
mq_writable (struct pipe_ringbuffer *pipe,
			 struct task_control_block *task)
{
	size_t total_len = sizeof(size_t) + task->stack->r2;

	/* If the write would be non-atomic */
	if (total_len > PIPE_BUF) {
		task->stack->r0 = -1;
		return 0;
	}
	/* Preserve 1 byte to distiguish empty or full */
	if ((size_t)PIPE_BUF - PIPE_LEN(*pipe) - 1 < total_len) {
		/* Trying to write more than we have space for: block */
		task->status = TASK_WAIT_WRITE;
		return 0;
	}
	return 1;
}

int
fifo_write (struct pipe_ringbuffer *pipe,
			struct task_control_block *task)
{
	size_t i;
	const char *buf = (const char*)task->stack->r1;
	/* Copy data into pipe */
	for (i = 0; i < task->stack->r2; i++)
		PIPE_PUSH(*pipe,buf[i]);
	return task->stack->r2;
}

int
mq_write (struct pipe_ringbuffer *pipe,
		  struct task_control_block *task)
{
	size_t i;
	const char *buf = (const char*)task->stack->r1;
	/* Copy count into pipe */
	for (i = 0; i < sizeof(size_t); i++)
		PIPE_PUSH(*pipe,*(((char*)&task->stack->r2)+i));
	/* Copy data into pipe */
	for (i = 0; i < task->stack->r2; i++)
		PIPE_PUSH(*pipe,buf[i]);
	return task->stack->r2;
}

int
_mknod(struct pipe_ringbuffer *pipe, int dev)
{
	switch(dev) {
	case S_IFIFO:
		pipe->readable = fifo_readable;
		pipe->writable = fifo_writable;
		pipe->read = fifo_read;
		pipe->write = fifo_write;
		break;
	case S_IMSGQ:
		pipe->readable = mq_readable;
		pipe->writable = mq_writable;
		pipe->read = mq_read;
		pipe->write = mq_write;
		break;
	default:
		return 1;
	}
	return 0;
}

int main()
{
	unsigned int stacks[TASK_LIMIT][STACK_SIZE];
	//struct task_control_block tasks[TASK_LIMIT];
	struct pipe_ringbuffer pipes[PIPE_LIMIT];
	struct task_control_block *ready_list[PRIORITY_LIMIT + 1];  /* [0 ... 39] */
	struct task_control_block *wait_list = NULL;
	//size_t task_count = 0;
	size_t current_task = 0;
	size_t i;
	struct task_control_block *task;
	int timeup;
	unsigned int tick_count = 0;

	SysTick_Config(configCPU_CLOCK_HZ / configTICK_RATE_HZ);

	init_rs232();
	__enable_irq();

	tasks[task_count].stack = (void*)init_task(stacks[task_count], &first);
	tasks[task_count].pid = 0;
	task_count++;

	/* Initialize all pipes */
	for (i = 0; i < PIPE_LIMIT; i++)
		pipes[i].start = pipes[i].end = 0;

	/* Initialize fifos */
	for (i = 0; i <= PATHSERVER_FD; i++)
		_mknod(&pipes[i], S_IFIFO);

	/* Initialize ready lists */
	for (i = 0; i <= PRIORITY_LIMIT; i++)
		ready_list[i] = NULL;

	while (1) {
		tasks[current_task].stack = activate(tasks[current_task].stack);
		tasks[current_task].status = TASK_READY;
		timeup = 0;

		switch (tasks[current_task].stack->r7) {
		case 0x1: /* fork */
			if (task_count == TASK_LIMIT) {
				/* Cannot create a new task, return error */
				tasks[current_task].stack->r0 = -1;
			}
			else {
				/* Compute how much of the stack is used */
				size_t used = stacks[current_task] + STACK_SIZE
					      - (unsigned int*)tasks[current_task].stack;
				/* New stack is END - used */
				tasks[task_count].stack = (void*)(stacks[task_count] + STACK_SIZE - used);
				/* Copy only the used part of the stack */
				memcpy(tasks[task_count].stack, tasks[current_task].stack,
				       used * sizeof(unsigned int));
				/* Set PID */
				tasks[task_count].pid = task_count;
				/* Set return values in each process */
				tasks[current_task].stack->r0 = task_count;
				tasks[task_count].stack->r0 = 0;
				tasks[task_count].prev = NULL;
				tasks[task_count].next = NULL;
				task_push(&ready_list[0], &tasks[task_count]);
				/* There is now one more task */
				task_count++;
			}
			break;
		case 0x2: /* getpid */
			tasks[current_task].stack->r0 = current_task;
			break;
		case 0x3: /* write */
			_write(&tasks[current_task], tasks, task_count, pipes);
			break;
		case 0x4: /* read */
			_read(&tasks[current_task], tasks, task_count, pipes);
			break;
		case 0x5: /* interrupt_wait */
			/* Enable interrupt */
			NVIC_EnableIRQ(tasks[current_task].stack->r0);
			/* Block task waiting for interrupt to happen */
			tasks[current_task].status = TASK_WAIT_INTR;
			break;
		case 0x8: /* mknod */
			if (tasks[current_task].stack->r0 < PIPE_LIMIT)
				tasks[current_task].stack->r0 =
					_mknod(&pipes[tasks[current_task].stack->r0],
						   tasks[current_task].stack->r2);
			else
				tasks[current_task].stack->r0 = -1;
			break;
		default: /* Catch all interrupts */
			if ((int)tasks[current_task].stack->r7 < 0) {
				unsigned int intr = -tasks[current_task].stack->r7 - 16;

				if (intr == SysTick_IRQn) {
					/* Never disable timer. We need it for pre-emption */
					timeup = 1;
					tick_count++;
				}
				else {
					/* Disable interrupt, interrupt_wait re-enables */
					NVIC_DisableIRQ(intr);
				}
				/* Unblock any waiting tasks */
				for (i = 0; i < task_count; i++)
					if ((tasks[i].status == TASK_WAIT_INTR && tasks[i].stack->r0 == intr) ||
					    (tasks[i].status == TASK_WAIT_TIME && tasks[i].stack->r0 == tick_count))
						tasks[i].status = TASK_READY;
			}
		}

		/* Put waken tasks in ready list */
		for (task = wait_list; task != NULL;) {
			struct task_control_block *next = task->next;
			if (task->status == TASK_READY)
				task_push(&ready_list[0], task);
			task = next;
		}
        
		if (tasks[current_task].status == TASK_READY) {
			if (!timeup)
				/* Current task has highest priority and remains execution time */
				continue;
			else    // last of this priority
				task_push(&ready_list[0], &tasks[current_task]);
		}
		else {
			task_push(&wait_list, &tasks[current_task]);
		}

        current_task = task_pop(&ready_list[0])->pid;
	}

	return 0;
}
