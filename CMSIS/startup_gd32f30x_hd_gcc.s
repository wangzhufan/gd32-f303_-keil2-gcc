/**
  ******************************************************************************
  * @file      startup_gd32f30x_hd_gcc.s
  * @brief     GD32F30x High Density devices GCC startup file.
  *            Based on CMSIS GCC startup template, adapted for GD32F30x.
  *
  *            This file performs:
  *              - Stack & Heap initialization
  *              - Vector table setup
  *              - .data section copy (Flash -> RAM)
  *              - .bss section zeroing
  *              - Call SystemInit, then main
  *
  * @version   2.1.0 (GCC port)
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m4
  .fpu fpv4-sp-d16
  .thumb

/*
 * Stack and Heap are managed by the linker script (GD32F303CC.ld).
 * _estack is defined in the linker script at ORIGIN(RAM) + LENGTH(RAM).
 * _Min_Heap_Size and _Min_Stack_Size are also reserved in the linker script.
 */

/* ----- Vector Table ----- */
  .section  .isr_vector, "a", %progbits
  .globl  __Vectors
  .globl  __Vectors_End
  .globl  __Vectors_Size

  .type   __Vectors, %object
  .size   __Vectors, . - __Vectors

__Vectors:
  .word  _estack                   /* Top of Stack (from linker script) */
  .word  Reset_Handler             /* Reset Handler */
  .word  NMI_Handler               /* NMI Handler */
  .word  HardFault_Handler         /* Hard Fault Handler */
  .word  MemManage_Handler         /* MPU Fault Handler */
  .word  BusFault_Handler          /* Bus Fault Handler */
  .word  UsageFault_Handler        /* Usage Fault Handler */
  .word  0                         /* Reserved */
  .word  0                         /* Reserved */
  .word  0                         /* Reserved */
  .word  0                         /* Reserved */
  .word  SVC_Handler               /* SVCall Handler */
  .word  DebugMon_Handler          /* Debug Monitor Handler */
  .word  0                         /* Reserved */
  .word  PendSV_Handler            /* PendSV Handler */
  .word  SysTick_Handler           /* SysTick Handler */

  /* External interrupts */
  .word  WWDGT_IRQHandler          /* 16: Window Watchdog Timer */
  .word  LVD_IRQHandler            /* 17: LVD */
  .word  TAMPER_IRQHandler         /* 18: Tamper */
  .word  RTC_IRQHandler            /* 19: RTC */
  .word  FMC_IRQHandler            /* 20: FMC */
  .word  RCU_CTC_IRQHandler        /* 21: RCU & CTC */
  .word  EXTI0_IRQHandler          /* 22: EXTI0 */
  .word  EXTI1_IRQHandler          /* 23: EXTI1 */
  .word  EXTI2_IRQHandler          /* 24: EXTI2 */
  .word  EXTI3_IRQHandler          /* 25: EXTI3 */
  .word  EXTI4_IRQHandler          /* 26: EXTI4 */
  .word  DMA0_Channel0_IRQHandler  /* 27: DMA0 CH0 */
  .word  DMA0_Channel1_IRQHandler  /* 28: DMA0 CH1 */
  .word  DMA0_Channel2_IRQHandler  /* 29: DMA0 CH2 */
  .word  DMA0_Channel3_IRQHandler  /* 30: DMA0 CH3 */
  .word  DMA0_Channel4_IRQHandler  /* 31: DMA0 CH4 */
  .word  DMA0_Channel5_IRQHandler  /* 32: DMA0 CH5 */
  .word  DMA0_Channel6_IRQHandler  /* 33: DMA0 CH6 */
  .word  ADC0_1_IRQHandler         /* 34: ADC0 & ADC1 */
  .word  USBD_HP_CAN0_TX_IRQHandler  /* 35: USBD HP & CAN0 TX */
  .word  USBD_LP_CAN0_RX0_IRQHandler /* 36: USBD LP & CAN0 RX0 */
  .word  CAN0_RX1_IRQHandler       /* 37: CAN0 RX1 */
  .word  CAN0_EWMC_IRQHandler      /* 38: CAN0 EWMC */
  .word  EXTI5_9_IRQHandler        /* 39: EXTI5~9 */
  .word  TIMER0_BRK_IRQHandler     /* 40: TIMER0 Break */
  .word  TIMER0_UP_IRQHandler      /* 41: TIMER0 Update */
  .word  TIMER0_TRG_CMT_IRQHandler /* 42: TIMER0 Trigger */
  .word  TIMER0_Channel_IRQHandler /* 43: TIMER0 Channel */
  .word  TIMER1_IRQHandler         /* 44: TIMER1 */
  .word  TIMER2_IRQHandler         /* 45: TIMER2 */
  .word  TIMER3_IRQHandler         /* 46: TIMER3 */
  .word  I2C0_EV_IRQHandler        /* 47: I2C0 Event */
  .word  I2C0_ER_IRQHandler        /* 48: I2C0 Error */
  .word  I2C1_EV_IRQHandler        /* 49: I2C1 Event */
  .word  I2C1_ER_IRQHandler        /* 50: I2C1 Error */
  .word  SPI0_IRQHandler           /* 51: SPI0 */
  .word  SPI1_IRQHandler           /* 52: SPI1 */
  .word  USART0_IRQHandler         /* 53: USART0 */
  .word  USART1_IRQHandler         /* 54: USART1 */
  .word  USART2_IRQHandler         /* 55: USART2 */
  .word  EXTI10_15_IRQHandler      /* 56: EXTI10~15 */
  .word  RTC_Alarm_IRQHandler      /* 57: RTC Alarm */
  .word  USBD_WKUP_IRQHandler      /* 58: USBD Wakeup */
  .word  TIMER7_BRK_IRQHandler     /* 59: TIMER7 Break */
  .word  TIMER7_UP_IRQHandler      /* 60: TIMER7 Update */
  .word  TIMER7_TRG_CMT_IRQHandler /* 61: TIMER7 Trigger */
  .word  TIMER7_Channel_IRQHandler /* 62: TIMER7 Channel */
  .word  ADC2_IRQHandler           /* 63: ADC2 */
  .word  EXMC_IRQHandler           /* 64: EXMC */
  .word  SDIO_IRQHandler           /* 65: SDIO */
  .word  TIMER4_IRQHandler         /* 66: TIMER4 */
  .word  SPI2_IRQHandler           /* 67: SPI2 */
  .word  UART3_IRQHandler          /* 68: UART3 */
  .word  UART4_IRQHandler          /* 69: UART4 */
  .word  TIMER5_IRQHandler         /* 70: TIMER5 */
  .word  TIMER6_IRQHandler         /* 71: TIMER6 */
  .word  DMA1_Channel0_IRQHandler  /* 72: DMA1 CH0 */
  .word  DMA1_Channel1_IRQHandler  /* 73: DMA1 CH1 */
  .word  DMA1_Channel2_IRQHandler  /* 74: DMA1 CH2 */
  .word  DMA1_Channel3_4_IRQHandler/* 75: DMA1 CH3 & CH4 */

__Vectors_End:
  .equ  __Vectors_Size, __Vectors_End - __Vectors


/* ----- Reset Handler ----- */
  .section  .text.Reset_Handler, "ax", %progbits
  .globl  Reset_Handler
  .type   Reset_Handler, %function
Reset_Handler:
  /* Ensure VTOR points to flash vector table (belt-and-suspenders) */
  ldr   r0, =0xE000ED08
  ldr   r1, =0x08000000
  str   r1, [r0]
  dsb
  isb

  /* Copy .data section from Flash (LMA) to RAM (VMA) */
  ldr   r0, =__data_load__
  ldr   r1, =__data_start__
  ldr   r2, =__data_end__
  subs  r2, r1
  beq   .L_zero_bss
.L_copy_data:
  ldrb  r3, [r0], #1
  strb  r3, [r1], #1
  subs  r2, #1
  bne   .L_copy_data

.L_zero_bss:
  /* Zero .bss section */
  ldr   r0, =__bss_start__
  ldr   r1, =__bss_end__
  subs  r1, r0
  beq   .L_call_init
  movs  r2, #0
.L_zero_bss_loop:
  strb  r2, [r0], #1
  subs  r1, #1
  bne   .L_zero_bss_loop

.L_call_init:
  /* Call SystemInit */
  ldr   r0, =SystemInit
  blx   r0

  /* Call static constructors */
  ldr   r0, =__libc_init_array
  blx   r0

  /* Call main */
  ldr   r0, =main
  blx   r0

.L_loop:
  b     .L_loop
  .size Reset_Handler, . - Reset_Handler


/* ----- Default Exception Handlers ----- */
  .macro  DEFAULT_HANDLER  name
  .section  .text.\name, "ax", %progbits
  .weak   \name
  .type   \name, %function
\name:
  b       .
  .size   \name, . - \name
  .endm

  DEFAULT_HANDLER  NMI_Handler

  /* Provide weak _init / _fini stubs for __libc_init_array */
  .section  .text._init, "ax", %progbits
  .weak   _init
  .type   _init, %function
_init:
  bx      lr
  .size   _init, . - _init

  .section  .text._fini, "ax", %progbits
  .weak   _fini
  .type   _fini, %function
_fini:
  bx      lr
  .size   _fini, . - _fini

  /* HardFault_Handler is overridden by cm_backtrace (cmb_fault.S) */
  .weak   HardFault_Handler
  .thumb_set HardFault_Handler, Default_Handler
  DEFAULT_HANDLER  MemManage_Handler
  DEFAULT_HANDLER  BusFault_Handler
  DEFAULT_HANDLER  UsageFault_Handler
  DEFAULT_HANDLER  SVC_Handler
  DEFAULT_HANDLER  DebugMon_Handler
  DEFAULT_HANDLER  PendSV_Handler
  DEFAULT_HANDLER  SysTick_Handler

/* ----- Default IRQ Handlers ----- */
  .section  .text.Default_Handler, "ax", %progbits
  .globl  Default_Handler
  .type   Default_Handler, %function
Default_Handler:
  b       .
  .size   Default_Handler, . - Default_Handler

  .weak   WWDGT_IRQHandler
  .thumb_set WWDGT_IRQHandler, Default_Handler
  .weak   LVD_IRQHandler
  .thumb_set LVD_IRQHandler, Default_Handler
  .weak   TAMPER_IRQHandler
  .thumb_set TAMPER_IRQHandler, Default_Handler
  .weak   RTC_IRQHandler
  .thumb_set RTC_IRQHandler, Default_Handler
  .weak   FMC_IRQHandler
  .thumb_set FMC_IRQHandler, Default_Handler
  .weak   RCU_CTC_IRQHandler
  .thumb_set RCU_CTC_IRQHandler, Default_Handler
  .weak   EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler, Default_Handler
  .weak   EXTI1_IRQHandler
  .thumb_set EXTI1_IRQHandler, Default_Handler
  .weak   EXTI2_IRQHandler
  .thumb_set EXTI2_IRQHandler, Default_Handler
  .weak   EXTI3_IRQHandler
  .thumb_set EXTI3_IRQHandler, Default_Handler
  .weak   EXTI4_IRQHandler
  .thumb_set EXTI4_IRQHandler, Default_Handler
  .weak   DMA0_Channel0_IRQHandler
  .thumb_set DMA0_Channel0_IRQHandler, Default_Handler
  .weak   DMA0_Channel1_IRQHandler
  .thumb_set DMA0_Channel1_IRQHandler, Default_Handler
  .weak   DMA0_Channel2_IRQHandler
  .thumb_set DMA0_Channel2_IRQHandler, Default_Handler
  .weak   DMA0_Channel3_IRQHandler
  .thumb_set DMA0_Channel3_IRQHandler, Default_Handler
  .weak   DMA0_Channel4_IRQHandler
  .thumb_set DMA0_Channel4_IRQHandler, Default_Handler
  .weak   DMA0_Channel5_IRQHandler
  .thumb_set DMA0_Channel5_IRQHandler, Default_Handler
  .weak   DMA0_Channel6_IRQHandler
  .thumb_set DMA0_Channel6_IRQHandler, Default_Handler
  .weak   ADC0_1_IRQHandler
  .thumb_set ADC0_1_IRQHandler, Default_Handler
  .weak   USBD_HP_CAN0_TX_IRQHandler
  .thumb_set USBD_HP_CAN0_TX_IRQHandler, Default_Handler
  .weak   USBD_LP_CAN0_RX0_IRQHandler
  .thumb_set USBD_LP_CAN0_RX0_IRQHandler, Default_Handler
  .weak   CAN0_RX1_IRQHandler
  .thumb_set CAN0_RX1_IRQHandler, Default_Handler
  .weak   CAN0_EWMC_IRQHandler
  .thumb_set CAN0_EWMC_IRQHandler, Default_Handler
  .weak   EXTI5_9_IRQHandler
  .thumb_set EXTI5_9_IRQHandler, Default_Handler
  .weak   TIMER0_BRK_IRQHandler
  .thumb_set TIMER0_BRK_IRQHandler, Default_Handler
  .weak   TIMER0_UP_IRQHandler
  .thumb_set TIMER0_UP_IRQHandler, Default_Handler
  .weak   TIMER0_TRG_CMT_IRQHandler
  .thumb_set TIMER0_TRG_CMT_IRQHandler, Default_Handler
  .weak   TIMER0_Channel_IRQHandler
  .thumb_set TIMER0_Channel_IRQHandler, Default_Handler
  .weak   TIMER1_IRQHandler
  .thumb_set TIMER1_IRQHandler, Default_Handler
  .weak   TIMER2_IRQHandler
  .thumb_set TIMER2_IRQHandler, Default_Handler
  .weak   TIMER3_IRQHandler
  .thumb_set TIMER3_IRQHandler, Default_Handler
  .weak   I2C0_EV_IRQHandler
  .thumb_set I2C0_EV_IRQHandler, Default_Handler
  .weak   I2C0_ER_IRQHandler
  .thumb_set I2C0_ER_IRQHandler, Default_Handler
  .weak   I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler, Default_Handler
  .weak   I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler, Default_Handler
  .weak   SPI0_IRQHandler
  .thumb_set SPI0_IRQHandler, Default_Handler
  .weak   SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler, Default_Handler
  .weak   USART0_IRQHandler
  .thumb_set USART0_IRQHandler, Default_Handler
  .weak   USART1_IRQHandler
  .thumb_set USART1_IRQHandler, Default_Handler
  .weak   USART2_IRQHandler
  .thumb_set USART2_IRQHandler, Default_Handler
  .weak   EXTI10_15_IRQHandler
  .thumb_set EXTI10_15_IRQHandler, Default_Handler
  .weak   RTC_Alarm_IRQHandler
  .thumb_set RTC_Alarm_IRQHandler, Default_Handler
  .weak   USBD_WKUP_IRQHandler
  .thumb_set USBD_WKUP_IRQHandler, Default_Handler
  .weak   TIMER7_BRK_IRQHandler
  .thumb_set TIMER7_BRK_IRQHandler, Default_Handler
  .weak   TIMER7_UP_IRQHandler
  .thumb_set TIMER7_UP_IRQHandler, Default_Handler
  .weak   TIMER7_TRG_CMT_IRQHandler
  .thumb_set TIMER7_TRG_CMT_IRQHandler, Default_Handler
  .weak   TIMER7_Channel_IRQHandler
  .thumb_set TIMER7_Channel_IRQHandler, Default_Handler
  .weak   ADC2_IRQHandler
  .thumb_set ADC2_IRQHandler, Default_Handler
  .weak   EXMC_IRQHandler
  .thumb_set EXMC_IRQHandler, Default_Handler
  .weak   SDIO_IRQHandler
  .thumb_set SDIO_IRQHandler, Default_Handler
  .weak   TIMER4_IRQHandler
  .thumb_set TIMER4_IRQHandler, Default_Handler
  .weak   SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler, Default_Handler
  .weak   UART3_IRQHandler
  .thumb_set UART3_IRQHandler, Default_Handler
  .weak   UART4_IRQHandler
  .thumb_set UART4_IRQHandler, Default_Handler
  .weak   TIMER5_IRQHandler
  .thumb_set TIMER5_IRQHandler, Default_Handler
  .weak   TIMER6_IRQHandler
  .thumb_set TIMER6_IRQHandler, Default_Handler
  .weak   DMA1_Channel0_IRQHandler
  .thumb_set DMA1_Channel0_IRQHandler, Default_Handler
  .weak   DMA1_Channel1_IRQHandler
  .thumb_set DMA1_Channel1_IRQHandler, Default_Handler
  .weak   DMA1_Channel2_IRQHandler
  .thumb_set DMA1_Channel2_IRQHandler, Default_Handler
  .weak   DMA1_Channel3_4_IRQHandler
  .thumb_set DMA1_Channel3_4_IRQHandler, Default_Handler

  .end
