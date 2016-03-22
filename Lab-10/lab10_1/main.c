#define USE_STDPERIPH_DRIVER
#include "stm32f10x.h"

/* STM32F10x common functions */
#include "stm32_p103.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Our own string function implementation */
#include "string-util.h"

static void setup_hardware();

volatile xQueueHandle serial_str_queue = NULL;
volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
volatile xQueueHandle serial_rx_queue = NULL;

/* Queue structure used for passing messages. */
typedef struct {
	char str[100];
} serial_str_msg;

/* Queue structure used for passing characters. */
typedef struct {
	char ch;
} serial_ch_msg;


void timeout_trap_high_priority(){              // to recognize if trapped
    while(1){
        GPIOC->ODR = GPIOC->ODR ^ 0x00001000;        
    }
}


/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
//{{{   
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
	serial_ch_msg rx_msg;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}
	else if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		/* Receive the byte from the buffer. */
		rx_msg.ch = USART_ReceiveData(USART2);

		/* Queue the received byte. */
		if(!xQueueSendToBackFromISR(serial_rx_queue, &rx_msg, &xHigherPriorityTaskWoken)) {
			/* If there was an error queueing the received byte,
			 * freeze. */
			while(1);
		}
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
//}}}    
}

void disrupt_task()
{
//{{{    
    volatile int i;
    while(1){
	    while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

        // consume much time in this task
        // ~101 instructions * cpu clock * 1000 ~ 1.38ms
        for(i=0; i<1000; i++){
            USART_SendData(USART2, 'p');
            USART_SendData(USART2, 'r');
            USART_SendData(USART2, 'e');
            USART_SendData(USART2, 'e');
            USART_SendData(USART2, 'm');
            USART_SendData(USART2, 'p');
            USART_SendData(USART2, 't');
            USART_SendData(USART2, ' ');
            USART_SendData(USART2, 'A'+i/100);
            USART_SendData(USART2, 'A'+(i%100)/10);
            USART_SendData(USART2, 'A'+i%10);
            USART_SendData(USART2, '\n');
            USART_SendData(USART2, '\r');
        }
//        for(i=0; i<100000; i++);
	    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
        vTaskDelay(100);
    }
//}}}    
}


void send_byte(char ch)
{
//{{{    
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART2, ch);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
//}}}    
}

char receive_byte()
{
//{{{    
	serial_ch_msg msg;

	/* Wait for a byte to be queued by the receive interrupts handler. */
	while (!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));

	return msg.ch;
//}}}    
}

void rs232_xmit_msg_task(void *pvParameters)
{
//{{{    
	serial_str_msg msg;
	int curr_char;

	while (1) {
		/* Read from the queue.  Keep trying until a message is
		 * received.  This will block for a period of time (specified
		 * by portMAX_DELAY). */
		while (!xQueueReceive(serial_str_queue, &msg, portMAX_DELAY));          // enter critical

		/* Write each character of the message to the RS232 port. */
		curr_char = 0;
		while (msg.str[curr_char] != '\0') {
			send_byte(msg.str[curr_char]);
			curr_char++;
		}
	}
///}}}    
}


/* Repeatedly queues a string to be sent to the RS232.
 *   delay - the time to wait between sending messages.  A delay of 1 means
 *           wait 1/100th of a second.
 */
void queue_str_task(const char *str, int delay)
{
//{{{    
	serial_str_msg msg;

	/* Prepare the message to be queued. */
	my_strcpy(msg.str, str);

	while (1) {
		/* Post the message.  Keep on trying until it is successful. */
//		while (!xQueueSendToBack(serial_str_queue, &msg, portMAX_DELAY));                   // enter critical
		if (xQueueSendToBack(serial_str_queue, &msg, ( portTickType )100)!=pdTRUE){         // enter critical
            timeout_trap_high_priority();                                                   // if high priority been blocked then goto trap
//	        my_strcpy(msg.str, "timeout");
//		    while (!xQueueSendToBack(serial_str_queue, &msg, portMAX_DELAY));               // enter critical
        }

		/* Wait. */
		vTaskDelay(delay);
	}
//}}}    
}

void queue_str_task1(void *pvParameters)
{
	queue_str_task("Hello 1\n", 200);
}

void queue_str_task2(void *pvParameters)
{
	queue_str_task("Hello 2\n", 50);
}

void serial_readwrite_task(void *pvParameters)
{
//{{{    
	serial_str_msg msg;
	char ch;
	int curr_char;
	int done;

	/* Prepare the response message to be queued. */
	my_strcpy(msg.str, "Got:");

	while (1) {
		curr_char = 4;
		done = 0;
		do {
			/* Receive a byte from the RS232 port (this call will
			 * block). */
			ch = receive_byte();

			/* If the byte is an end-of-line type character, then
			 * finish the string and inidcate we are done.
			 */
			if ((ch == '\r') || (ch == '\n')) {
				msg.str[curr_char] = '\n';
				msg.str[curr_char+1] = '\0';
				done = -1;
				/* Otherwise, add the character to the
				 * response string. */
			}
			else {
				msg.str[curr_char++] = ch;
			}
		} while (!done);

		/* Once we are done building the response string, queue the
		 * response to be sent to the RS232 port.
		 */
		while (!xQueueSendToBack(serial_str_queue, &msg, portMAX_DELAY));       // enter critical
	}
//}}}    
}

int main()
{
	init_led();

	init_button();
	enable_button_interrupts();

	init_rs232();
	enable_rs232_interrupts();
	enable_rs232();

	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	serial_str_queue = xQueueCreate(1, sizeof(serial_str_msg));
	vSemaphoreCreateBinary(serial_tx_wait_sem);
	serial_rx_queue = xQueueCreate(1, sizeof(serial_ch_msg));


    // queue_str_task1 = queue_str_task2 = serial_readwrite_task > rs232_xmit_msg_task
    // 
    // 
    // 
    // queue_str_task1 ->
    // queue_str_task2 ->
    //                                                               -> [serial_str_queue] -> rs232_xmit_msg_task
    // isr -> [serial_rx_queue] -> serial_readwrite_task:receive_byte
    // 
    // ---------->
    //
    // rs232_xmit_msg_task:send_byte -> <serial_tx_wait_sem> -> isr(complete)
    // 

    // insert a middle priority task to grab <serial_tx_wait_sem> for a long time
    // higher tasks set bounded timeout
    // queue_str_task1 = queue_str_task2 = serial_readwrite_task > disrupt_task > rs232_xmit_msg_task

	/* Create tasks to queue a string to be written to the RS232 port. */
	xTaskCreate(queue_str_task1,
	            (signed portCHAR *) "Serial Write 1",
	            512 /* stack size */, NULL,
	            tskIDLE_PRIORITY + 10, NULL );
	xTaskCreate(queue_str_task2,
	            (signed portCHAR *) "Serial Write 2",
	            512 /* stack size */, NULL, 
                tskIDLE_PRIORITY + 10, NULL);

	/* Create a task to write messages from the queue to the RS232 port. */
	xTaskCreate(rs232_xmit_msg_task,
	            (signed portCHAR *) "Serial Xmit Str",
	            512 /* stack size */, NULL, 
                tskIDLE_PRIORITY + 2, NULL);

	/* Create a task to receive characters from the RS232 port and echo
	 * them back to the RS232 port. */
	xTaskCreate(serial_readwrite_task,
	            (signed portCHAR *) "Serial Read/Write",
	            512 /* stack size */, NULL,
	            tskIDLE_PRIORITY + 10, NULL);

    // middle priority
	xTaskCreate(disrupt_task,
	            (signed portCHAR *) "Disrupt",
	            512 /* stack size */, NULL,
	            tskIDLE_PRIORITY + 5, NULL);


	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
