/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TSC_H
#define JOS_KERN_TSC_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <stdint.h>

#define PIT_READB_OUT_PIN     0x80
#define PIT_READB_NULL_COUNT  0x40
#define PIT_MODE_MASK         0x0E
#define PIT_MODE_INTR         0x00
#define PIT_MODE_HARD_ONESHOT 0x02
#define PIT_MODE_RATE         0x04
#define PIT_MODE_SQUARE       0x06
#define PIT_MODE_SOFT_STROBE  0x08
#define PIT_MODE_HARD_STROBE  0x0A
#define PIT_MODE_RATE2        0x0C
#define PIT_MODE_SQUARE2      0x0E
#define PIT_ACC_MASK          0x30
#define PIT_ACC_LATCH         0x00
#define PIT_ACC_LO            0x10
#define PIT_ACC_HI            0x20
#define PIT_ACC_LOHI          0x30
#define PIT_BCD               0x01

#define PIT_CMD_READB_LATCH_COUNT  0x20
#define PIT_CMD_READB_LATCH_STATUS 0x10
#define PIT_CMD_READB_0            0xC8
#define PIT_CMD_READB_1            0xC4
#define PIT_CMD_READB_2            0xC2

#define PIT_CMD_CHANNEL_0 0x00
#define PIT_CMD_CHANNEL_1 0x40
#define PIT_CMD_CHANNEL_2 0x80
#define PIT_CMD_READBACK  0xC0

#define PIT_IO_CHANNEL_0 0x40
#define PIT_IO_CHANNEL_1 0x41
#define PIT_IO_CHANNEL_2 0x42
#define PIT_IO_CMD       0x43

uint64_t tsc_calibrate(void);
void timer_start(const char *name);
void timer_stop(void);
void timer_cpu_frequency(const char *name);

#endif /* !JOS_KERN_TSC_H */
