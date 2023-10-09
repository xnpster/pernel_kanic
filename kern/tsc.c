/* This file mostly get from linux: arch/x86/kern/tsc.c */

#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/string.h>

#include <kern/tsc.h>
#include <kern/timer.h>

/* The clock frequency of the i8253/i8254 PIT */
#define PIT_TICK_RATE 1193182ul
#define DEFAULT_FREQ  2500000
#define TIMES         100

struct Timer timer_pit = {
        .timer_name = "pit",
        .get_cpu_freq = tsc_calibrate};

/* Theres more commands but they are not used */

/*
 * This reads the current MSB of the PIT counter, and
 * checks if we are running on sufficiently fast and
 * non-virtualized hardware.
 *
 * Our expectations are:
 *
 *  - the PIT is running at roughly 1.19MHz
 *
 *  - each IO is going to take about 1us on real hardware,
 *    but we allow it to be much faster (by a factor of 10) or
 *    _slightly_ slower (ie we allow up to a 2us read+counter
 *    update - anything else implies a unacceptably slow CPU
 *    or PIT for the fast calibration to work.
 *
 *  - with 256 PIT ticks to read the value, we have 214us to
 *    see the same MSB (and overhead like doing a single TSC
 *    read per MSB value etc).
 *
 *  - We're doing 2 reads per loop (LSB, MSB), and we expect
 *    them each to take about a microsecond on real hardware.
 *    So we expect a count value of around 100. But we'll be
 *    generous, and accept anything over 50.
 *
 *  - if the PIT is stuck, and we see *many* more reads, we
 *    return early (and the next caller of pit_expect_msb()
 *    then consider it a failure when they don't see the
 *    next expected value).
 *
 * These expectations mean that we know that we have seen the
 * transition from one expected value to another with a fairly
 * high accuracy, and we didn't miss any events. We can thus
 * use the TSC value at the transitions to calculate a pretty
 * good value for the TSC frequencty.
 */
static inline int
pit_verify_msb(unsigned char val) {
    /* Ignore LSB */
    inb(PIT_IO_CHANNEL_2);
    return inb(PIT_IO_CHANNEL_2) == val;
}

static inline int
pit_expect_msb(unsigned char val, uint64_t *tscp, unsigned long *deltap) {
    uint64_t tsc = 0;

    int count = 0;
    while (count++ < 50000) {
        if (!pit_verify_msb(val)) break;
        tsc = read_tsc();
    }

    *deltap = read_tsc() - tsc;
    *tscp = tsc;

    /* We require _some_ success, but the quality control
     * will be based on the error terms on the TSC values. */
    return count > 6;
}

/* How many MSB values do we want to se? We aim for
 * a maximum error rate of 500ppm (in practice the
 * real error is much smaller), but refuse to spend
 * more than 25ms on it */

#define MAX_QUICK_PIT_MS         25
#define MAX_QUICK_PIT_ITERATIONS (MAX_QUICK_PIT_MS * PIT_TICK_RATE / 1000 / 256)

static unsigned long
quick_pit_calibrate(void) {
    int i;
    uint64_t tsc, delta;
    unsigned long d1, d2;

    /* Set the Gate high, disable speaker */
    outb(0x61, (inb(0x61) & ~0x02) | 0x01);

    /* Counter 2, mode 0 (one-shot), binary count
     *
     * NOTE! Mode 2 decrements by two (and then the
     * output is flipped each time, giving the same
     * final output frequency as a decrement-by-one),
     * so mode 0 is much better when looking at the
     * individual counts */
    outb(PIT_IO_CMD, PIT_CMD_CHANNEL_2 | PIT_ACC_LOHI);

    /* Start at 0xffff */
    outb(PIT_IO_CHANNEL_2, 0xFF);
    outb(PIT_IO_CHANNEL_2, 0xFF);

    /* The PIT starts counting at the next edge, so we
     * need to delay for a microsecond. The easiest way
     * to do that is to just read back the 16-bit counter
     * once from the PIT */
    pit_verify_msb(0);
    if (pit_expect_msb(0xFF, &tsc, &d1)) {
        for (i = 1; i <= MAX_QUICK_PIT_ITERATIONS; i++) {

            if (!pit_expect_msb(0xFF - i, &delta, &d2)) break;

            /* Iterate until the error is less than 500 ppm */
            delta -= tsc;
            if (d1 + d2 >= delta >> 11) continue;

            /*
             * Check the PIT one more time to verify that
             * all TSC reads were stable wrt the PIT.
             *
             * This also guarantees serialization of the
             * last cycle read ('d2') in pit_expect_msb.
             */
            if (!pit_verify_msb(0xFE - i)) break;

            goto success;
        }
    }
    return 0;

success:
    /*
     * Ok, if we get here, then we've seen the
     * MSB of the PIT decrement 'i' times, and the
     * error has shrunk to less than 500 ppm.
     *
     * As a result, we can depend on there not being
     * any odd delays anywhere, and the TSC reads are
     * reliable (within the error). We also adjust the
     * delta to the middle of the error bars, just
     * because it looks nicer.
     *
     * kHz = ticks / time-in-seconds / 1000;
     * kHz = (t2 - t1) / (I * 256 / PIT_TICK_RATE) / 1000
     * kHz = ((t2 - t1) * PIT_TICK_RATE) / (I * 256 * 1000)
     */

    delta += (long)(d2 - d1) / 2;
    delta *= PIT_TICK_RATE;
    delta /= i * 256 * 1000;

    return delta;
}

uint64_t
tsc_calibrate(void) {
    static uint64_t cpu_freq;

    if (!cpu_freq) {
        int i = TIMES;
        while (--i > 0) {
            if ((cpu_freq = quick_pit_calibrate())) break;
        }
        if (!i) {
            cpu_freq = DEFAULT_FREQ;
            cprintf("Can't calibrate pit timer. Using default frequency\n");
        }
    }

    return cpu_freq * 1000;
}

void
print_time(unsigned seconds) {
    cprintf("%u\n", seconds);
}

void
print_timer_error(void) {
    cprintf("Timer Error\n");
}

/* Use print_time function to print timert result
 * Use print_timer_error function to print error. */

// LAB 5: Your code here:

static bool timer_started = 0;
static int timer_id = -1;
static uint64_t timer = 0;
static uint64_t freq = 0;

void
timer_start(const char *name) {
    (void)timer_started;
    (void)timer_id;
    (void)timer;
    (void)freq;
}

void
timer_stop(void) {
}

void
timer_cpu_frequency(const char *name) {
}
