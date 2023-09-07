#ifndef JOS_INC_TRACEOPT_H
#define JOS_INC_TRACEOPT_H

#if LAB == 8
#define trace_traps 1
#elif !defined(trace_traps)
#define trace_traps 0
#endif

#ifndef trace_traps_more
#define trace_traps_more 0
#endif

#ifndef trace_memory
/* TIP: set this to (!!curenv)
 *      to enable logging after initiallization */
#define trace_memory 0
#endif

#ifndef trace_memory_more
#define trace_memory_more 0
#endif

#if LAB == 9 || LAB == 8
#undef trace_pagefaults
#define trace_pagefaults 1
#elif !defined(trace_pagefaults)
#define trace_pagefaults 0
#endif

#if LAB == 9 || LAB == 8 || LAB == 3
#undef trace_envs
#define trace_envs 1
#elif !defined(trace_envs)
#define trace_envs 0
#endif

#if LAB == 4 || LAB == 3
#undef trace_envs_more
#define trace_envs_more 1
#elif !defined(trace_envs_more)
#define trace_envs_more 0
#endif

#ifndef trace_spinlock
#define trace_spinlock 0
#endif

#ifndef trace_init
#define trace_init 1
#endif

#endif
