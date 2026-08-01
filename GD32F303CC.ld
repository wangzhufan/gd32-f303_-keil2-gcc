/**
 ******************************************************************************
 * @file      GD32F303CC.ld
 * @brief     Linker script for GD32F303CCT6 (Cortex-M4F)
 *
 *   Memory layout:
 *     Flash (IROM):  0x08000000, 256 KB
 *     SRAM  (IRAM):  0x20000000, 48 KB
 ******************************************************************************
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory definition */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

/* Highest address of the user stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* end of RAM */

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size  = 0x00000400;   /* required amount of heap  */
_Min_Stack_Size = 0x00001D00;   /* required amount of stack */

SECTIONS
{
  /* The program code and other data into FLASH */
  .text :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))            /* Vector table */
    *(.text)                        /* .text sections (code) */
    *(.text*)                       /* .text* sections (code) */
    *(.glue_7)                      /* glue arm to thumb code */
    *(.glue_7t)                     /* glue thumb to arm code */
    *(.eh_frame)                    /* Exception handling frame */

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;                     /* end of .text (cm_backtrace needs this) */
  } > FLASH

  /* Start of code in flash (cm_backtrace needs this) */
  _stext = ORIGIN(FLASH);
  /* Start of main stack (bottom of RAM, stack grows down) */
  _sstack = ORIGIN(RAM);

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)                      /* .rodata sections (constants) */
    *(.rodata*)                     /* .rodata* sections (constants) */
    . = ALIGN(4);
  } > FLASH

  /* ARM exception unwinding */
  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* Preinit/init/fini arrays for C++ constructors */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH
  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /* Used by the startup to initialize .data */
  . = ALIGN(4);
  PROVIDE_HIDDEN(__data_source = LOADADDR(.data));

  /* Initialized data section into RAM, load LMA copy after code in FLASH */
  .data :
  {
    . = ALIGN(4);
    PROVIDE(__data_start__ = .);
    *(.data)                        /* .data sections */
    *(.data*)                       /* .data* sections */
    . = ALIGN(4);
    PROVIDE(__data_end__ = .);
  } > RAM AT > FLASH

  __data_load__ = LOADADDR(.data);

  /* Uninitialized data section */
  .bss :
  {
    . = ALIGN(4);
    PROVIDE(__bss_start__ = .);
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    PROVIDE(__bss_end__ = .);
  } > RAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

}
