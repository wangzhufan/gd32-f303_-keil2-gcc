/*!
    \file    systick.c
    \brief   the systick configuration file

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x 
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

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

#include "gd32f30x.h"
#include "systick.h"


volatile static uint32_t s_delay;

#if defined(CONFIG_TCPC_FUNC) || defined(CONFIG_UFCS_FUNC) || defined(CONFIG_UFCS_SIMULATOR) \
    || defined(CONFIG_SCP_FUNC)
#define SYSTICK_SUPPORT_IRQ_MODE
#endif

#ifndef SYSTICK_SUPPORT_IRQ_MODE
volatile static float s_count_1us = 0;
volatile static float s_count_1ms = 0;
#endif
/*!
    \brief      configure systick
    \param[in]  none
    \param[out] none
    \retval     none
*/
void systick_config(void)
{
#ifdef SYSTICK_SUPPORT_IRQ_MODE
    /* setup systick timer for 1000000Hz interrupts */
	  // change by wang: 1us
    if (SysTick_Config(SystemCoreClock / 1000000U)){
        /* capture error */
        while (1){
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
#else
	/* systick clock source is from HCLK/8 */
    systick_clksource_set(SYSTICK_CLKSOURCE_HCLK_DIV8);
    s_count_1us = (float)SystemCoreClock/8000000;
    s_count_1ms = (float)s_count_1us * 1000;
#endif
}

#ifdef SYSTICK_SUPPORT_IRQ_MODE
/*!
    \brief      delay a time in microseconds
    \param[in]  count: count in microseconds
    \param[out] none
    \retval     none
*/
void delay_1us(uint32_t count)
{
    s_delay = count;

    while(0U != s_delay){
    }
}

/*!
    \brief      delay a time in milliseconds
    \param[in]  count: count in milliseconds
    \param[out] none
    \retval     none
*/
void delay_1ms(uint32_t count)
{
    delay_1us(1000 * count);
//    delay = count;
//    while(0U != delay){
//    }
}

#else

void delay_1us(uint32_t count)
{
    uint32_t ctl;

    /* reload the count value */
    SysTick->LOAD = (uint32_t)(count * s_count_1us);
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* enable the systick timer */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    /* wait for the COUNTFLAG flag set */
    do {
        ctl = SysTick->CTRL;
    } while((ctl & SysTick_CTRL_ENABLE_Msk) && !(ctl & SysTick_CTRL_COUNTFLAG_Msk));
    /* disable the systick timer */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
}

/*!
    \brief      delay a time in milliseconds in polling mode
    \param[in]  count: count in milliseconds
    \param[out] none
    \retval     none
*/
void delay_1ms(uint32_t count)
{
    uint32_t ctl;

    /* reload the count value */
    SysTick->LOAD = (uint32_t)(count * s_count_1ms);
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* enable the systick timer */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    /* wait for the COUNTFLAG flag set */
    do {
        ctl = SysTick->CTRL;
    } while((ctl & SysTick_CTRL_ENABLE_Msk) && !(ctl & SysTick_CTRL_COUNTFLAG_Msk));
    /* disable the systick timer */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
}
#endif



/*!
    \brief      delay decrement
    \param[in]  none
    \param[out] none
    \retval     none
*/
void delay_decrement(void)
{
    if (0U != s_delay){
        s_delay--;
    }
}
