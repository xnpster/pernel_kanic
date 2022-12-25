
#include <inc/string.h>
#include <inc/lib.h>

void
cputchar(int ch) {
    char c = (char)ch;

    /* Unlike standard Unix's putchar,
     * the cputchar function _always_ outputs to the system console. */
    sys_cputs(&c, 1);
}

int
getchar(void) {
    unsigned char c;

    /* JOS does, however, support standard _input_ redirection,
     * allowing the user to redirect script files to the shell and such.
     * getchar() reads a character from file descriptor 0. */

    int res = read(0, &c, 1);
    return res < 0 ? res : res ? c :
                                 -E_EOF;
}

/* "Real" console file descriptor implementation.
 * The putchar/getchar functions above will still come here by default,
 * but now can be redirected to files, pipes, etc., via the fd layer. */

static ssize_t devcons_read(struct Fd *, void *, size_t);
static ssize_t devcons_write(struct Fd *, const void *, size_t);
static int devcons_close(struct Fd *);
static int devcons_stat(struct Fd *, struct Stat *);

struct Dev devcons = {
        .dev_id = 'c',
        .dev_name = "cons",
        .dev_read = devcons_read,
        .dev_write = devcons_write,
        .dev_close = devcons_close,
        .dev_stat = devcons_stat};

int
iscons(int fdnum) {
    struct Fd *fd;
    int res = fd_lookup(fdnum, &fd);
    if (res < 0) return res;

    return fd->fd_dev_id == devcons.dev_id;
}

int
opencons(void) {
    int res;
    struct Fd *fd;
    if ((res = fd_alloc(&fd)) < 0) return res;

    if ((res = sys_alloc_region(0, fd, PAGE_SIZE, PROT_RW | PROT_SHARE)) < 0) return res;

    fd->fd_dev_id = devcons.dev_id;
    fd->fd_omode = O_RDWR;

    return fd2num(fd);
}

static ssize_t
devcons_read(struct Fd *fd, void *vbuf, size_t n) {
    if (!n) return 0;

    int c;
    while (!(c = sys_cgetc())) sys_yield();
    if (c < 0) return c;

    /* Ctrl-D is eof */
    if (c == 0x04) return 0;

    *(char *)vbuf = (char)c;
    return 1;
}

#define WRITEBUFSZ 128

static ssize_t
devcons_write(struct Fd *fd, const void *vbuf, size_t n) {
    int res = 0;

    int inc;
    char buf[WRITEBUFSZ];
    /* Mistake: Have to nul-terminate arg to sys_cputs,
     * so we have to copy vbuf into buf in chunks and nul-terminate. */
    for (res = 0; res < n; res += inc) {
        inc = MIN(n - res, WRITEBUFSZ - 1);
        memmove(buf, (char *)vbuf + res, inc);
        sys_cputs(buf, inc);
    }

    return res;
}

static int
devcons_close(struct Fd *fd) {
    USED(fd);

    return 0;
}

static int
devcons_stat(struct Fd *fd, struct Stat *stat) {
    strcpy(stat->st_name, "<cons>");
    return 0;
}
