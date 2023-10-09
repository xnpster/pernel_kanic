#include <inc/random.h>

static unsigned int next = 1;

static int
rand_r(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed % ((unsigned int)RAND_MAX + 1));
}

int
rand(void) {
    return (rand_r(&next));
}

void
srand(unsigned int seed) {
    next = seed;
}

void
rand_init(unsigned int num) {
    srand(((unsigned int *)_dev_urandom)[num % _dev_urandom_len]);
}
