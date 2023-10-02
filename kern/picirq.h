/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_PICIRQ_H
#define JOS_KERN_PICIRQ_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

/* Number of IRQs */
#define MAX_IRQS 16

/* I/O Addresses of the two 8259A programmable interrupt controllers */
/* Master (IRQs 0-7) */
#define IO_PIC1 0x20
/* Slave (IRQs 8-15) */
#define IO_PIC2 0xA0

#define IO_PIC1_CMND IO_PIC1
#define IO_PIC1_DATA IO_PIC1 + 1

#define IO_PIC2_CMND IO_PIC2
#define IO_PIC2_DATA IO_PIC2 + 1

/* IRQ at which slave connects to master */
#define IRQ_SLAVE 2

#define PIC_EOI 0x20

/* ICW1:  0001g0hi
 *    g:  0 = edge triggering, 1 = level triggering
 *    h:  0 = cascaded PICs, 1 = master only
 *    i:  0 = no ICW4, 1 = ICW4 required */
#define ICW1_ICW4      0x01 /* ICW4 (not) needed */
#define ICW1_SINGLE    0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL     0x08 /* Level triggered (edge) mode */
#define ICW1_INIT      0x10 /* Initialization - required! */

/* ICW4:  000nbmap
 *    n:  1 = special fully nested mode
 *    b:  1 = buffered mode
 *    m:  0 = slave PIC, 1 = master PIC
 *      (ignored when b is 0, as the master/slave role can be hardwired).
 *    a:  1 = Automatic EOI mode
 *    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
 * NOTE Automatic EOI mode doesn't tend to work on the slave.
 * Linux source code says it's "to be investigated" */
#define ICW4_8086       0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM       0x10 /* Special fully nested (not) */

/* OCW3:  0ef01prs
 *   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
 *    p:  0 = no polling, 1 = polling mode
 *   rs:  0x = NOP, 10 = read IRR, 11 = read ISR */
#define OCW3       0x08
#define OCW3_CLEAR 0x40
#define OCW3_SET   0x60
#define OCW3_POLL  0x04
#define OCW3_IRR   0x02
#define OCW3_ISR   0x03

#ifndef __ASSEMBLER__

#include <inc/types.h>
#include <inc/x86.h>

void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_irq_mask(uint8_t mask);
void pic_irq_unmask(uint8_t mask);

#endif /* !__ASSEMBLER__ */

#endif /* !JOS_KERN_PICIRQ_H */
