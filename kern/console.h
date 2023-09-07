/* See COPYRIGHT for copyright information. */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

#define CRT_ROWS    25
#define CRT_COLS    80
#define CRT_SIZE    (CRT_ROWS * CRT_COLS)
#define SYMBOL_SIZE 8

void cons_init(void);
void fb_init(void);
int cons_getc(void);

/* IRQ1 */
void kbd_intr(void);
/* IRQ4 */
void serial_intr(void);

#endif /* _CONSOLE_H_ */
