/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_KCLOCK_H
#define JOS_KERN_KCLOCK_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <stdint.h>
#include <inc/x86.h>

#define RTC_NON_RATE_MASK(X)          (X & 0xF0)
#define RTC_SET_NEW_RATE(input, rate) (RTC_NON_RATE_MASK(input) | rate)
#define RTC_500MS_RATE                0x0F

#define RTC_AREG 0x0A
#define RTC_BREG 0x0B
#define RTC_CREG 0x0C
#define RTC_DREG 0x0D

#define RTC_SEC  0x00
#define RTC_MIN  0x02
#define RTC_HOUR 0x04

#define RTC_DAY       0x07
#define RTC_MON       0x08
#define RTC_YEAR      0x09
#define RTC_YEAR_HIGH 0x32

#define RTC_UPDATE_IN_PROGRESS 0x80
#define RTC_BINARY             0x4
#define RTC_12H                0x2

#define RTC_PIE 0x40
#define RTC_AIE 0x20
#define RTC_UIE 0x10

void rtc_timer_init(void);
uint8_t rtc_check_status(void);
void rtc_timer_pic_interrupt(void);
void rtc_timer_pic_handle(void);

#define CMOS_START 0xE /* start of CMOS: offset 14 */
#define CMOS_SIZE  50  /* 50 bytes of CMOS */

/* CMOS bytes 7 & 8: base memory size */
#define CMOS_BASELO (CMOS_START + 7) /* low byte; RTC off. 0x15 */
#define CMOS_BASEHI (CMOS_START + 8) /* high byte; RTC off. 0x16 */

/* CMOS bytes 9 & 10: extended memory size */
#define CMOS_EXTLO (CMOS_START + 9)  /* low byte; RTC off. 0x17 */
#define CMOS_EXTHI (CMOS_START + 10) /* high byte; RTC off. 0x18 */

/* CMOS bytes 34 and 35: extended memory POSTed size */
#define CMOS_PEXTLO (CMOS_START + 38) /* low byte; RTC off. 0x34 */
#define CMOS_PEXTHI (CMOS_START + 39) /* high byte; RTC off. 0x35 */

/* CMOS byte 36: current century.  (please increment in Dec99!) */
#define CMOS_CENTURY (CMOS_START + 36) /* RTC offset 0x32 */

uint8_t cmos_read8(uint8_t reg);
void cmos_write8(uint8_t reg, uint8_t datum);
uint16_t cmos_read16(uint8_t reg);

int gettime(void);

#define BCD2BIN(bcd) ({uint8_t bcd__ = (bcd); ((bcd__ & 15) + (bcd__ >> 4) * 10); })

#endif /* !JOS_KERN_KCLOCK_H */
