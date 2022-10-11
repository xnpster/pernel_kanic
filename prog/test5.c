#include <stdint.h>
#include <inc/random.h>

int (*volatile cprintf)(const char *fmt, ...);
void *(*volatile test_alloc)(uint8_t nbytes);
void (*volatile test_free)(void *ap);

void (*volatile sys_yield)(void);

unsigned int deep = 0;

void
test_rec(void) {
    void *buf;
    ++deep;

    if (!(rand() % 79)) {
        sys_yield();
    }

    buf = test_alloc(rand() % 300);
    if (buf) {
        if (deep < 200 && (rand() % 53)) {
            test_rec();
        }
        test_free(buf);
    } else {
        if (deep < 200 && (rand() % 17)) {
            test_rec();
        }
    }

    --deep;
    return;
}

void
umain(int argc, char **argv) {
    rand_init(4);
    for (;;) {
        test_rec();
    }
}
