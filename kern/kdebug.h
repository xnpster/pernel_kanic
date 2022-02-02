#ifndef JOS_KERN_KDEBUG_H
#define JOS_KERN_KDEBUG_H

#include <inc/types.h>

#define RIPDEBUG_BUFSIZ 256

/* Debug information about a particular instruction pointer */
struct Ripdebuginfo {
    /* Source code filename for RIP */
    char rip_file[RIPDEBUG_BUFSIZ];
    /* Source code linenumber for RIP */
    int rip_line;
    /* Name of function containing RIP
    * NOTE Not null terminated */
    char rip_fn_name[RIPDEBUG_BUFSIZ];
    /* Length of function name */
    int rip_fn_namelen;
    /* Address of start of function */
    uintptr_t rip_fn_addr;
    /* Number of function arguments */
    int rip_fn_narg;
};

int debuginfo_rip(uintptr_t eip, struct Ripdebuginfo *info);
uintptr_t find_function(const char *const fname);

#endif
