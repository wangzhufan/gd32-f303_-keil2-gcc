/**
 * @file syscalls.c
 * @brief Newlib system call stubs for GD32F303 bare-metal
 *
 * Required for printf/malloc to work with arm-none-eabi-gcc + newlib-nano
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "gd32f30x.h"

/* -------------------------------------------------------------------------- */
/*  _write  -- retarget printf/sprintf output to USART0                       */
/* -------------------------------------------------------------------------- */
extern int fputc(int ch, void *f);  /* declared in main.c */

int _write(int file, char *ptr, int len)
{
    int i;
    (void)file;
    for (i = 0; i < len; i++) {
        fputc((int)ptr[i], (void *)1);
    }
    return len;
}

/* -------------------------------------------------------------------------- */
/*  _sbrk  -- heap management for malloc/free                                 */
/* -------------------------------------------------------------------------- */
extern char _end;       /* start of heap (set by linker script) */
extern char _estack;    /* top of RAM / bottom of stack          */

/* Reserve 8 KB for stack; adjust if your app needs more/less */
#define STACK_RESERVED  0x2000

void *_sbrk(int incr)
{
    static char *heap_end = NULL;
    char *prev_heap_end;

    if (heap_end == NULL) {
        heap_end = &_end;
    }

    prev_heap_end = heap_end;

    /* Prevent heap from growing into stack area */
    if (heap_end + incr > (&_estack - STACK_RESERVED)) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end += incr;
    return (void *)prev_heap_end;
}

/* -------------------------------------------------------------------------- */
/*  Minimal stubs for other newlib syscalls (avoid linker warnings)           */
/* -------------------------------------------------------------------------- */
int _close(int file)
{
    (void)file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

off_t _lseek(int file, off_t ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    errno = EBADF;
    return -1;
}
