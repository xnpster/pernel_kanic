#include <inc/fs.h>
#include <inc/lib.h>

#define SECTSIZE 512                  /* bytes per disk sector */
#define BLKSECTS (BLKSIZE / SECTSIZE) /* sectors per block */

/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP + (n*BLKSIZE). */
#define DISKMAP 0x10000000

/* Maximum disk size we can handle (3GB) */
#define DISKSIZE 0xC0000000

extern struct Super *super; /* superblock */
extern uint32_t *bitmap;    /* bitmap blocks mapped in memory */

/* bc.c */
void *diskaddr(blockno_t blockno);
void flush_block(void *addr);
void bc_init(void);

/* fs.c */
void fs_init(void);
int file_get_block(struct File *f, blockno_t file_blockno, char **pblk);
int file_create(const char *path, struct File **f);
int file_block_walk(struct File *f, blockno_t filebno, blockno_t **ppdiskbno, bool alloc);
int file_open(const char *path, struct File **f);
ssize_t file_read(struct File *f, void *buf, size_t count, off_t offset);
ssize_t file_write(struct File *f, const void *buf, size_t count, off_t offset);
int file_set_size(struct File *f, off_t newsize);
void file_flush(struct File *f);
int file_remove(const char *path);
void fs_sync(void);

bool block_is_free(blockno_t blockno);
blockno_t alloc_block(void);

/* test.c */
void fs_test(void);
