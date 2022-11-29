#include <inc/lib.h>

/* Maximum number of file descriptors a program may hold open concurrently */
#define MAXFD 32
/* Bottom of file descriptor area */
#define FDTABLE 0xD0000000LL
/* Bottom of file data area.  We reserve one data page for each FD,
 * which devices can use if they choose. */
#define FILEDATA (FDTABLE + MAXFD * PAGE_SIZE)

/* Return the 'struct Fd*' for file descriptor index i */
#define INDEX2FD(i) ((struct Fd *)(FDTABLE + (i)*PAGE_SIZE))
/* Return the file data page for file descriptor index i */
#define INDEX2DATA(i) ((char *)(FILEDATA + (i)*PAGE_SIZE))


/********************File descriptor manipulators***********************/

uint64_t
fd2num(struct Fd *fd) {
    return ((uintptr_t)fd - FDTABLE) / PAGE_SIZE;
}

char *
fd2data(struct Fd *fd) {
    return INDEX2DATA(fd2num(fd));
}

/* Finds the smallest i from 0 to MAXFD-1 that doesn't have
 * its fd page mapped.
 * Sets *fd_store to the corresponding fd page virtual address.
 *
 * fd_alloc does NOT actually allocate an fd page.
 * It is up to the caller to allocate the page somehow.
 * This means that if someone calls fd_alloc twice in a row
 * without allocating the first page we return, we'll return the same
 * page the second time.
 *
 * Hint: Use INDEX2FD.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_MAX_FD: no more file descriptors
 * On error, *fd_store is set to 0. */
int
fd_alloc(struct Fd **fd_store) {
    for (int i = 0; i < MAXFD; i++) {
        struct Fd *fd = INDEX2FD(i);
        if (!(get_prot(fd) & PROT_R)) {
            *fd_store = fd;
            return 0;
        }
    }
    *fd_store = 0;
    return -E_MAX_OPEN;
}

/* Check that fdnum is in range and mapped.
 * If it is, set *fd_store to the fd page virtual address.
 *
 * Returns 0 on success (the page is in range and mapped), < 0 on error.
 * Errors are:
 *  -E_INVAL: fdnum was either not in range or not mapped. */
int
fd_lookup(int fdnum, struct Fd **fd_store) {
    if (fdnum < 0 || fdnum >= MAXFD) {
        if (debug) cprintf("[%08x] bad fd %d\n", thisenv->env_id, fdnum);
        return -E_INVAL;
    }

    struct Fd *fd = INDEX2FD(fdnum);

    if (!(get_prot(fd) & PROT_R)) {
        if (debug) cprintf("[%08x] closed fd %d\n", thisenv->env_id, fdnum);
        return -E_INVAL;
    }

    *fd_store = fd;
    return 0;
}

/* Frees file descriptor 'fd' by closing the corresponding file
 * and unmapping the file descriptor page.
 * If 'must_exist' is 0, then fd can be a closed or nonexistent file
 * descriptor; the function will return 0 and have no other effect.
 * If 'must_exist' is 1, then fd_close returns -E_INVAL when passed a
 * closed or nonexistent file descriptor.
 * Returns 0 on success, < 0 on error. */
int
fd_close(struct Fd *fd, bool must_exist) {
    int res;

    struct Fd *fd2;
    if ((res = fd_lookup(fd2num(fd), &fd2)) < 0 || fd != fd2) {
        return (must_exist ? res : 0);
    }

    struct Dev *dev;
    if ((res = dev_lookup(fd->fd_dev_id, &dev)) >= 0) {
        res = dev->dev_close ? (*dev->dev_close)(fd) : 0;
    }

    /* Make sure fd is unmapped.  Might be a no-op if
     * (*dev->dev_close)(fd) already unmapped it. */
    USED(sys_unmap_region(0, fd, PAGE_SIZE));
    return res;
}

/**********************File functions***************************/

static struct Dev *devtab[] = {
        &devfile,
        &devpipe,
        &devcons,
        NULL};

int
dev_lookup(int dev_id, struct Dev **dev) {
    for (size_t i = 0; devtab[i]; i++) {
        if (devtab[i]->dev_id == dev_id) {
            *dev = devtab[i];
            return 0;
        }
    }
    cprintf("[%08x] unknown device type %d\n", thisenv->env_id, dev_id);
    *dev = 0;
    return -E_INVAL;
}

int
close(int fdnum) {
    struct Fd *fd;
    int res = fd_lookup(fdnum, &fd);
    if (res < 0) return res;

    return fd_close(fd, 1);
}

void
close_all(void) {
    for (int i = 0; i < MAXFD; i++) close(i);
}

/* Make file descriptor 'newfdnum' a duplicate of file descriptor 'oldfdnum'.
 * For instance, writing onto either file descriptor will affect the
 * file and the file offset of the other.
 * Closes any previously open file descriptor at 'newfdnum'.
 * This is implemented using virtual memory tricks (of course!). */
int
dup(int oldfdnum, int newfdnum) {
    struct Fd *oldfd, *newfd;

    int res;
    if ((res = fd_lookup(oldfdnum, &oldfd)) < 0) return res;
    close(newfdnum);

    newfd = INDEX2FD(newfdnum);
    char *oldva = fd2data(oldfd);
    char *newva = fd2data(newfd);

    int prot = get_prot(oldva);
    if (prot & PROT_R) {
        if ((res = sys_map_region(0, oldva, 0, newva, PAGE_SIZE, prot)) < 0) goto err;
    }
    prot = get_prot(oldfd);
    if ((res = sys_map_region(0, oldfd, 0, newfd, PAGE_SIZE, prot)) < 0) goto err;

    return newfdnum;

err:
    sys_unmap_region(0, newfd, PAGE_SIZE);
    sys_unmap_region(0, newva, PAGE_SIZE);
    return res;
}

ssize_t
read(int fdnum, void *buf, size_t n) {
    int res;

    struct Fd *fd;
    if ((res = fd_lookup(fdnum, &fd)) < 0) return res;

    struct Dev *dev;
    if ((res = dev_lookup(fd->fd_dev_id, &dev)) < 0) return res;

    if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
        cprintf("[%08x] read %d -- bad mode\n", thisenv->env_id, fdnum);
        return -E_INVAL;
    }

    if (!dev->dev_read) return -E_NOT_SUPP;

    return (*dev->dev_read)(fd, buf, n);
}

ssize_t
readn(int fdnum, void *buf, size_t n) {
    int inc = 1, res = 0;
    for (; inc && res < n; res += inc) {
        inc = read(fdnum, (char *)buf + res, n - res);
        if (inc < 0) return inc;
    }
    return res;
}

ssize_t
write(int fdnum, const void *buf, size_t n) {
    int res;

    struct Fd *fd;
    if ((res = fd_lookup(fdnum, &fd)) < 0) return res;

    struct Dev *dev;
    if ((res = dev_lookup(fd->fd_dev_id, &dev)) < 0) return res;

    if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
        cprintf("[%08x] write %d -- bad mode\n", thisenv->env_id, fdnum);
        return -E_INVAL;
    }

    if (debug) {
        cprintf("write %d %p %lu via dev %s\n",
                fdnum, buf, (unsigned long)n, dev->dev_name);
    }

    if (!dev->dev_write) return -E_NOT_SUPP;

    return (*dev->dev_write)(fd, buf, n);
}

int
seek(int fdnum, off_t offset) {
    int res;
    struct Fd *fd;

    if ((res = fd_lookup(fdnum, &fd)) < 0) return res;

    fd->fd_offset = offset;
    return 0;
}

int
ftruncate(int fdnum, off_t newsize) {
    int res;

    struct Fd *fd;
    if ((res = fd_lookup(fdnum, &fd)) < 0) return res;

    struct Dev *dev;
    if ((res = dev_lookup(fd->fd_dev_id, &dev)) < 0) return res;

    if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
        cprintf("[%08x] ftruncate %d -- bad mode\n",
                thisenv->env_id, fdnum);
        return -E_INVAL;
    }

    if (!dev->dev_trunc) return -E_NOT_SUPP;

    return (*dev->dev_trunc)(fd, newsize);
}

int
fstat(int fdnum, struct Stat *stat) {
    int res;

    struct Fd *fd;
    if ((res = fd_lookup(fdnum, &fd)) < 0) return res;

    struct Dev *dev;
    if ((res = dev_lookup(fd->fd_dev_id, &dev)) < 0) return res;

    if (!dev->dev_stat) return -E_NOT_SUPP;

    stat->st_name[0] = 0;
    stat->st_size = 0;
    stat->st_isdir = 0;
    stat->st_dev = dev;

    return (*dev->dev_stat)(fd, stat);
}

int
stat(const char *path, struct Stat *stat) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return fd;

    int res = fstat(fd, stat);
    close(fd);

    return res;
}
