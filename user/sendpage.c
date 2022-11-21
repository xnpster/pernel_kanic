/* Test Conversation between parent and child environment
 * Contributed by Varun Agrawal at Stony Brook */

#include <inc/lib.h>

const char *str1 = "hello child environment! how are you?";
const char *str2 = "hello parent environment! I'm good.";

#define TEMP_ADDR       ((char *)0xa00000)
#define TEMP_ADDR_CHILD ((char *)0xb00000)

void
umain(int argc, char **argv) {
    envid_t who;

    if ((who = fork()) == 0) {
        /* Child */
        size_t sz = PAGE_SIZE;
        ipc_recv(&who, TEMP_ADDR_CHILD, &sz, 0);
        cprintf("%x got message: %s\n", who, TEMP_ADDR_CHILD);
        if (strncmp(TEMP_ADDR_CHILD, str1, strlen(str1)) == 0)
            cprintf("child received correct message\n");

        memcpy(TEMP_ADDR_CHILD, str2, strlen(str2) + 1);
        ipc_send(who, 0, TEMP_ADDR_CHILD, PAGE_SIZE, PROT_RW);
        return;
    }

    /* Parent */
    sys_alloc_region(thisenv->env_id, TEMP_ADDR, PAGE_SIZE, PROT_RW);
    memcpy(TEMP_ADDR, str1, strlen(str1) + 1);
    ipc_send(who, 0, TEMP_ADDR, PAGE_SIZE, PROT_RW);

    size_t sz = PAGE_SIZE;
    ipc_recv(&who, TEMP_ADDR, &sz, 0);
    cprintf("%x got message: %s\n", who, TEMP_ADDR);
    if (strncmp(TEMP_ADDR, str2, strlen(str2)) == 0)
        cprintf("parent received correct message\n");
    return;
}
