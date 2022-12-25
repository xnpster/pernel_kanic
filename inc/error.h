/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ERROR_H
#define JOS_INC_ERROR_H

enum {
    /* Kernel error codes -- keep in sync with list in lib/printfmt.c. */
    E_UNSPECIFIED = 1,   /* Unspecified or unknown problem */
    E_BAD_ENV = 2,       /* Environment doesn't exist or otherwise */
                         /* cannot be used in requested action */
    E_INVAL = 3,         /* Invalid parameter */
    E_NO_MEM = 4,        /* Request failed due to memory shortage */
    E_NO_FREE_ENV = 5,   /* Attempt to create a new environment beyond */
                         /* the maximum allowed */
    E_BAD_DWARF = 6,     /* Incorrect DWARF debug information */
    E_FAULT = 7,         /* Memory fault */
    E_INVALID_EXE = 8,   /* Invalid executable */
    E_NO_SYS = 9,        /* Unimplemented syscall */
    E_NO_ENT = 10,       /* Not found */
    E_IPC_NOT_RECV = 11, /* Attempt to send to env that is not recving */
    E_EOF = 12,          /* Unexpected end of file */
    /* File system error codes -- only seen in user-level */
    E_NO_DISK = 13,     /* No free space left on disk */
    E_MAX_OPEN = 14,    /* Too many files are open */
    E_NOT_FOUND = 15,   /* File or block not found */
    E_BAD_PATH = 16,    /* Bad path */
    E_FILE_EXISTS = 17, /* File already exists */
    E_NOT_EXEC = 18,    /* File not a valid executable */
    E_NOT_SUPP = 19,    /* Operation not supported */
    MAXERROR
};

#endif /* !JOS_INC_ERROR_H */
