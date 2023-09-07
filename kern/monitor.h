#ifndef JOS_KERN_MONITOR_H
#define JOS_KERN_MONITOR_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

struct Trapframe;

/* Activate the kernel monitor,
 * optionally providing a trap frame indicating the current state
 * (NULL if none) */
void monitor(struct Trapframe *tf);

#endif /* !JOS_KERN_MONITOR_H */
