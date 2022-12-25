#include <stdbool.h>
#include <inc/stdio.h>
#include <inc/assert.h>

#ifndef JOS_INC_TIME_H
#define JOS_INC_TIME_H

struct tm {
    int tm_sec;  /* Seconds.     [0-60] */
    int tm_min;  /* Minutes.     [0-59] */
    int tm_hour; /* Hours.       [0-23] */
    int tm_mday; /* Day.         [1-31] */
    int tm_mon;  /* Month.       [0-11] */
    int tm_year; /* Year - 1900.  */
};

#define MINUTE       (60)
#define HOUR         (60 * 60)
#define DAY          (24 * 60 * 60)
#define ISLEAP(y)    (!((y) % 400) || (!((y) % 4) && ((y) % 100)))
#define Y2DAYS(y)    (365 * (y) + (y - 1) / 4 - (y - 1) / 100 + (y - 1) / 400)
#define M2DAYS(m, y) ((const int[]){0, 31, 59, 90, 120, 151, 181,      \
                                    212, 243, 273, 304, 334, 365}[m] + \
                      (ISLEAP(y) && (m) > 1))

inline static int
timestamp(struct tm *time) {
    return DAY * (Y2DAYS(time->tm_year + 1900) - Y2DAYS(1970) +
                  M2DAYS(time->tm_mon, time->tm_year + 1900) + time->tm_mday - 1) +
           time->tm_hour * HOUR + time->tm_min * MINUTE + time->tm_sec;
}

inline static void
mktime(int time, struct tm *tm) {
    // TODO Support negative timestamps

    int year = 1970 + (time / (DAY * 366 + 1));
    while (time >= DAY * (Y2DAYS(year + 1) - Y2DAYS(1970))) year++;
    tm->tm_year = year - 1900;
    time -= DAY * (Y2DAYS(year) - Y2DAYS(1970));

    int month = time / (DAY * 32);
    while (time >= DAY * (M2DAYS(month + 1, year))) month++;
    tm->tm_mon = month;
    time -= DAY * M2DAYS(month, year);

    tm->tm_mday = time / DAY + 1;
    time %= DAY;
    tm->tm_hour = time / HOUR;
    time %= HOUR;
    tm->tm_min = time / MINUTE;
    time %= MINUTE;
    tm->tm_sec = time;
}

inline static void
print_datetime(struct tm *tm) {
    cprintf("%04d-%02d-%02d %02d:%02d:%02d\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
}

inline static void
snprint_datetime(char *buf, int size, struct tm *tm) {
    assert(size >= 10 + 1 + 8 + 1);
    snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
}

#endif
