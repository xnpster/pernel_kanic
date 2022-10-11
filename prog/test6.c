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

    if (!(rand() % 47)) {
        sys_yield();
    }

    buf = test_alloc(rand() % 200);
    if (buf) {
        if (deep <= 173 && (rand() % 41)) {
            test_rec();
        }
        test_free(buf);
    } else {
        if (deep <= 173 && (rand() % 29)) {
            test_rec();
        }
    }

    --deep;
    return;
}

void
umain(int argc, char **argv) {
    rand_init(5);
    for (;;) {
        test_rec();
    }
}
