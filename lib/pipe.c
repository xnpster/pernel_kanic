#include <inc/lib.h>

static ssize_t devpipe_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devpipe_write(struct Fd *fd, const void *buf, size_t n);
static int devpipe_stat(struct Fd *fd, struct Stat *stat);
static int devpipe_close(struct Fd *fd);

struct Dev devpipe = {
        .dev_id = 'p',
        .dev_name = "pipe",
        .dev_read = devpipe_read,
        .dev_write = devpipe_write,
        .dev_close = devpipe_close,
        .dev_stat = devpipe_stat,
};

#define PIPEBUFSIZ (PAGE_SIZE - 2 * sizeof(off_t))

struct Pipe {
    off_t p_rpos;              /* read position */
    off_t p_wpos;              /* write position */
    uint8_t p_buf[PIPEBUFSIZ]; /* data buffer */
};

int
pipe(int pfd[2]) {
    int res;
    struct Fd *fd0, *fd1;
    void *va;

    static_assert(sizeof(struct Pipe) <= PAGE_SIZE, "PIPEBUFSIZ is too large");


    /* Allocate the file descriptor table entries */
    if ((res = fd_alloc(&fd0)) < 0 ||
        (res = sys_alloc_region(0, fd0, PAGE_SIZE, PROT_RW | PROT_SHARE)) < 0) goto err;

    if ((res = fd_alloc(&fd1)) < 0 ||
        (res = sys_alloc_region(0, fd1, PAGE_SIZE, PROT_RW | PROT_SHARE)) < 0) goto err1;

    /* allocate the pipe structure as first data page in both */
    va = fd2data(fd0);
    if ((res = sys_alloc_region(0, va, PAGE_SIZE, PROT_RW | PROT_SHARE)) < 0) goto err2;
    if ((res = sys_map_region(0, va, 0, fd2data(fd1), PAGE_SIZE, PROT_RW | PROT_SHARE)) < 0) goto err3;

    assert(sys_region_refs(va, PAGE_SIZE) == 2);

    /* set up fd structures */
    fd0->fd_dev_id = devpipe.dev_id;
    fd0->fd_omode = O_RDONLY;

    fd1->fd_dev_id = devpipe.dev_id;
    fd1->fd_omode = O_WRONLY;

    if (debug) {
        cprintf("[%08x] pipecreate %08lx\n",
                thisenv->env_id, get_uvpt_entry(va));
    }

    pfd[0] = fd2num(fd0);
    pfd[1] = fd2num(fd1);
    return 0;

err3:
    sys_unmap_region(0, va, PAGE_SIZE);
err2:
    sys_unmap_region(0, fd1, PAGE_SIZE);
err1:
    sys_unmap_region(0, fd0, PAGE_SIZE);
err:
    return res;
}

static int
_pipeisclosed(struct Fd *fd, struct Pipe *p) {
    return !sys_region_refs2(fd, PAGE_SIZE, p, PAGE_SIZE);
}

int
pipeisclosed(int fdnum) {
    struct Fd *fd;
    int res = fd_lookup(fdnum, &fd);
    if (res < 0) return res;

    struct Pipe *pip = (struct Pipe *)fd2data(fd);
    return _pipeisclosed(fd, pip);
}

static ssize_t
devpipe_read(struct Fd *fd, void *vbuf, size_t n) {
    struct Pipe *p = (struct Pipe *)fd2data(fd);
    if (debug) {
        cprintf("[%08x] devpipe_read %08lx %lu rpos %ld wpos %ld\n",
                thisenv->env_id, (unsigned long)get_uvpt_entry(p),
                (unsigned long)n, (long)p->p_rpos, (long)p->p_wpos);
    }

    uint8_t *buf = vbuf;
    for (size_t i = 0; i < n; i++) {
        while (p->p_rpos == p->p_wpos) /* pipe is empty */ {
            /* If we got any data, return it */
            if (i > 0) return i;

            /* If all the writers are gone, note eof */
            if (_pipeisclosed(fd, p)) return 0;

            /* Yield and see what happens */
            if (debug) cprintf("devpipe_read yield\n");
            sys_yield();
        }

        /* There's a byte. Take it.
         * Wait to increment rpos until the byte is taken! */
        buf[i] = p->p_buf[p->p_rpos % PIPEBUFSIZ];
        p->p_rpos++;
    }

    return n;
}

static ssize_t
devpipe_write(struct Fd *fd, const void *vbuf, size_t n) {
    struct Pipe *p = (struct Pipe *)fd2data(fd);
    if (debug) {
        cprintf("[%08x] devpipe_write %08lx %lu rpos %ld wpos %ld\n",
                thisenv->env_id, get_uvpt_entry(p),
                (unsigned long)n, (long)p->p_rpos, (long)p->p_wpos);
    }

    const uint8_t *buf = vbuf;
    for (size_t i = 0; i < n; i++) {
        while (p->p_wpos >= p->p_rpos + sizeof(p->p_buf)) /* pipe is full */ {
            /* If all the readers are gone
             * (it's only writers like us now),
             * note eof */
            if (_pipeisclosed(fd, p)) return 0;

            /* Yield and see what happens */
            if (debug) cprintf("devpipe_write yield\n");
            sys_yield();
        }
        /* There's room for a byte. Store it.
         * Wait to increment wpos until the byte is stored! */
        p->p_buf[p->p_wpos % PIPEBUFSIZ] = buf[i];
        p->p_wpos++;
    }

    return n;
}

static int
devpipe_stat(struct Fd *fd, struct Stat *stat) {
    struct Pipe *p = (struct Pipe *)fd2data(fd);
    strcpy(stat->st_name, "<pipe>");
    stat->st_size = p->p_wpos - p->p_rpos;
    stat->st_isdir = 0;
    stat->st_dev = &devpipe;
    return 0;
}

static int
devpipe_close(struct Fd *fd) {
    USED(sys_unmap_region(0, fd, PAGE_SIZE));
    return sys_unmap_region(0, fd2data(fd), PAGE_SIZE);
}
