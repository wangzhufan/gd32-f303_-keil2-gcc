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
#include "ring_buffer.h"

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
ring_buffer_t g_ring_buffer;
#define BUF_SIZE  512

// 状态机状态
typedef enum {
    STATE_IDLE,      // 空闲，等待 'A'
    STATE_A,         // 收到 'A'，等待 'T'
    STATE_AT,        // 收到 "AT"，等待 '+' 或 '\r'
    STATE_RECV,      // 接收数据中
} at_state_t;

struct at_message g_at_msg = { 0 };

static at_state_t at_state = STATE_IDLE;

/**
 * @brief 处理串口接收到的字符（主循环调用）
 */
void at_process_char(uint8_t c)
{
    // 状态机处理
    switch(at_state)
    {
        case STATE_IDLE:
            if(c == 'A') {
                at_state = STATE_A;
                g_at_msg.len = 0;
                g_at_msg.message[g_at_msg.len++] = c;
            }
            break;
            
        case STATE_A:
            if(c == 'T') {
                at_state = STATE_AT;
                g_at_msg.message[g_at_msg.len++] = c;
            } else {
                at_state = STATE_IDLE;  // 不是"AT"，重置
                g_at_msg.len = 0;
            }
            break;
            
        case STATE_AT:
            if(c == '+') {
                at_state = STATE_RECV;
                g_at_msg.message[g_at_msg.len++] = c;
            } else if(c == '\r' || c == '\n') {
                // AT+ 没有+，可能是查询指令如 AT+VERSION\r\n
                // 但我们只处理 AT+ 开头的，丢弃
                at_state = STATE_IDLE;
                g_at_msg.len = 0;
            } else {
                // 不是 '+'，重置
                at_state = STATE_IDLE;
                g_at_msg.len = 0;
            }
            break;
            
        case STATE_RECV:
            // 接收数据，直到遇到 \r\n
            if(c == '\n' && g_at_msg.len > 0 && g_at_msg.message[g_at_msg.len-1] == '\r')
            {
                // 完整指令，去掉末尾的 \r
                g_at_msg.message[g_at_msg.len-1] = '\0';
                if (g_sgm_debug_flag > 0)
						printf("%s,len = %d!\n",g_at_msg.message,g_at_msg.len-1);
                uart_buff_parse(g_at_msg.message, g_at_msg.len-1);
                
                // 重置状态
                at_state = STATE_IDLE;
                g_at_msg.len = 0;
            }
            else
            {
                // 存储数据
                if(g_at_msg.len < BUF_SIZE-1) {
                    g_at_msg.message[g_at_msg.len++] = c;
                } else {
                    // 溢出，重置
                    at_state = STATE_IDLE;
                    g_at_msg.len = 0;
                }
            }
            break;
            
        default:
            at_state = STATE_IDLE;
            g_at_msg.len = 0;
            break;
    }
}

/**
 * @brief 从ring_buffer读取数据并处理
 */
void at_process_ring(void)
{
    uint8_t c;
    uint32_t count;
    
    ring_buffer_get_count(&g_ring_buffer, &count);
    
    while(count > 0)
    {
        ring_buffer_read(&g_ring_buffer, &c);
        count--;
        at_process_char(c);  // 喂给状态机
    }
}

int main(void)
{ 

    systick_config();

#if 1
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

    /**************************ring buffer_init**********************/
    ring_buffer_init(&g_ring_buffer);
    memset(&g_at_msg,0,sizeof(g_at_msg));

    while(1) {

            at_process_ring();
		}
}


