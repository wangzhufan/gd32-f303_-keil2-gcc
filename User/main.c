/*!
    \file  main.c
    \brief GPIO running led demo
    
    \version 2017-02-10, V1.0.0, demo for GD32F30x
    \version 2018-10-10, V1.1.0, demo for GD32F30x
    \version 2018-12-25, V2.0.0, demo for GD32F30x
*/

/*
    Copyright (c) 2018, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/
#include <stdio.h>
#include "main.h"
#include "cm_backtrace.h"
#include "gd32f303c_eval.h"
#include "i2c.h"
#include "systick.h"
#include "string.h"
#include "at_cmd_common.h"


/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/

extern int sgm_irq_flag;
void led_spark(void)
{
}

#if 0
/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM1, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM1, USART_FLAG_TBE));
    return ch;
}
#else
/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
#endif

#define USART_MAX_RX_SIZE   (2048)

uint8_t g_sgm_slave_addr = (0x6B<<1);

extern uint8_t g_rxbuffer[USART_MAX_RX_SIZE];
extern __IO uint16_t g_rxcount;
extern int g_sgm_debug_flag;
struct test_param  s_read_write_buff[20];

struct at_queue g_queue = {
        .front = 1,
        .rear = 0,
        .size = 0
};

bool queue_is_full(struct at_queue *q)
{
    return q->size == QUEUE_CAPACITY;
}

bool queue_is_empty(struct at_queue *q)
{
    return q->size == 0;
}

void dequeue(struct at_queue *q)
{
		if(queue_is_empty(q)) {        
				q->front = 1;
				q->rear = 0;
        return ;
    }		
    memset(q->data[q->front].message, 0, q->data[q->front].len);
		q->data[q->front].len = 0;
		
    --q->size;
    ++q->front;
    if(q->front == QUEUE_CAPACITY) {
        q->front = 0;
    }
		
		if(queue_is_empty(q)) {        
				q->front = 1;
				q->rear = 0;
        return ;
    }
}

void enqueue(struct at_queue *q, struct at_message *message)
{
		if(message->len > AT_MESSAGE_MAX_LEN) {			
        return;
    }
		
    if(queue_is_full(q)) {				
        //dequeue(q);
        return;
    }
		
		++q->rear;
    ++q->size;
    if(q->rear == QUEUE_CAPACITY) {				
        q->rear = 0;
    }

    q->data[q->rear].len = message->len;
    memcpy(q->data[q->rear].message, message->message, message->len);
}

static bool queue_head(struct at_queue *q, struct at_message *m)
{
    if(queue_is_empty(q)) {
        m->len = 0;
				q->front = 1;
				q->rear = 0;
        return 0;
    }

    memcpy(m->message, q->data[q->front].message, q->data[q->front].len);
    m->len = q->data[q->front].len;
		
		return 1;
}

int main(void)
{ 
	struct at_message msg = { 0 };

    systick_config();

#if 0
    /* USART interrupt configuration */
    nvic_irq_enable(USART0_IRQn, 0, 0);

	gd_eval_com_init(EVAL_COM1);

	/* enable USART0 receive interrupt */
    usart_interrupt_enable(USART0, USART_INT_RBNE);

    /* enable USART0 transmit interrupt */
    //usart_interrupt_enable(USART0, USART_INT_TBE);

#else

	/* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART0);

    /*  USARTx_Tx */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    /*  USARTx_Rx */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* USART configure */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
#endif
	delay_1ms(500);
	i2c_init(400000);
	//IIC_Init();
	//pclk1 = rcu_clock_freq_get(CK_APB1);
	//printf("a usart transmit test example pclk1=%d!",pclk1);


    /************************* Enable the LEDs GPIO  Start ****************************/
    rcu_periph_clock_enable(RCU_GPIOB);


	memset(g_rxbuffer,0,USART_MAX_RX_SIZE);
    memset(s_read_write_buff,0,sizeof(struct test_param) * 20);
	/************************* Enable the LEDs GPIO Clock End ***********************/

	/**************************cm_backtrace_init**********************/
	cm_backtrace_init("GD32", FW_VERSION_NUM, FW_VERSION_NUM);


    while(1) {
			#if 0
			if ('A' == g_rxbuffer[0] && '\n' == g_rxbuffer[g_rxcount - 1])
			{
					if (g_sgm_debug_flag > 0)
						printf("%s,len = %d!\n",g_rxbuffer,g_rxcount);
					uart_buff_parse(g_rxbuffer,g_rxcount);
					memset(g_rxbuffer,0,g_rxcount);
					g_rxcount = 0;
			}
			#else

            printf("wang test33333!\n");
            delay_1ms(1000);
			memset(&msg,0,sizeof(msg));
			if (queue_head(&g_queue, &msg))	
			{	
				if (g_sgm_debug_flag > 0)
					printf("msg: %s,len = %d!\n",msg.message,msg.len);				
				dequeue(&g_queue);
				uart_buff_parse(msg.message,msg.len);
			}
			
			#endif

		}
}


