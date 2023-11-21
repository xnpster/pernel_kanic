#include "nvme.h"
#include <inc/x86.h>
#include <inc/lib.h>

static int nvme_acmd_get_features(struct NvmeController *ctl, int nsid, int fid, uint64_t prp1, uint64_t prp2, uint32_t *res);
static int nvme_acmd_set_features(struct NvmeController *ctl, int nsid, int fid, uint64_t prp1, uint64_t prp2, uint32_t *res);
static int nvme_acmd_create_cq(struct NvmeController *ctl, struct NvmeQueueAttributes *ioq, uint64_t prp);
static int nvme_acmd_create_sq(struct NvmeController *ctl, struct NvmeQueueAttributes *ioq, uint64_t prp);
static int nvme_acmd_identify(struct NvmeController *ctl, int nsid, uint64_t prp1, uint64_t prp2);
static int nvme_cmd_rw(struct NvmeController *ctl, struct NvmeQueueAttributes *ioq, int opc, int nsid, uint64_t slba, int nlb, uint64_t prp1, uint64_t prp2);

/* NVMe Controller structure */
static struct NvmeController nvme;

static int
nvme_map(struct NvmeController *ctl) {
    ctl->mmio_base_addr = (volatile uint8_t *)NVME_VADDR;

    /* Map BAR0 register to ctl->mmio_base_addr as readable, writable
     * and non-cacheable memory. (Map at most NVME_MAX_MAP_MEM bytes here.)
     * TIP: Functions get_bar_address(), get_bar_size()
     *      and sys_map_physical_region() might be useful here */
    // LAB 10: Your code here


    DEBUG("NVMe MMIO base = %p, size = %x, pa = %lx", ctl->mmio_base_addr, memsize, nvme_pa);

    return NVME_OK;
}

/* Allocate memory for NVMe queues and store its virtual address */
static int
nvme_alloc_queues(struct NvmeController *ctl) {
    ctl->buffer = (void *)NVME_QUEUE_VADDR;

    int r = sys_alloc_region(0, ctl->buffer, 4 * PAGE_SIZE, PROT_RW | PROT_CD);
    if (r < 0)
        panic("queue alloc failed");

    DEBUG("NVMe page buffer allocated with pages:");

    /* Touch buffer pages so that they does not change their addresses.
     * They will be already zeroed by the kernel otherwise */
    for (size_t i = 0; i < 6; i++) {
        volatile char *page = (volatile char *)ctl->buffer + NVME_PAGE_SIZE * i;
        *page = 0;
        DEBUG("    va=%p, pa=%lx", page, get_phys_addr((char *)page));
    }

    return NVME_OK;
}

static void
nvme_dump_status(struct NvmeController *ctl) {
    volatile uint8_t *base = ctl->mmio_base_addr;
    cprintf("NVMe registers dump:\n");
    cprintf("  MMIO base address = %p\n", base);
    cprintf("  Supported specification version = %01d.%0d\n",
            (*NVME_REG32(base, NVME_REG_VS) >> 16) & 1,
            (*NVME_REG32(base, NVME_REG_VS) >> 8) & 0xf);
    cprintf("  CAP = %016lx\n", *NVME_REG64(base, NVME_REG_CAP));
    cprintf("  VS = %08x\n", *NVME_REG32(base, NVME_REG_VS));
    cprintf("  INTMS = %08x\n", *NVME_REG32(base, NVME_REG_INTMS));
    cprintf("  INTMC = %08x\n", *NVME_REG32(base, NVME_REG_INTMC));
    cprintf("  CC = %08x\n", *NVME_REG32(base, NVME_REG_CC));
    cprintf("  CSTS = %08x\n", *NVME_REG32(base, NVME_REG_CSTS));
    cprintf("  NSSR = %08x\n", *NVME_REG32(base, NVME_REG_NSSR));
    cprintf("  AQA = %08x\n", *NVME_REG32(base, NVME_REG_AQA));
    cprintf("  ASQ = %016lx\n", *NVME_REG64(base, NVME_REG_ASQ));
    cprintf("  ACQ = %016lx\n", *NVME_REG64(base, NVME_REG_ACQ));
}

static inline void
nvme_wait_ready(struct NvmeController *ctl) {
    while ((*NVME_REG32(ctl->mmio_base_addr, NVME_REG_CSTS) & NVME_CSTS_RDY) == 0)
        asm volatile("pause");
}

static int
nvme_setup_admin_queue(struct NvmeController *ctl, int size, void *sqbuff, void *cqbuff) {
    volatile uint8_t *base = ctl->mmio_base_addr;

    assert(((uintptr_t)sqbuff & (NVME_PAGE_SIZE - 1)) == 0);
    assert(((uintptr_t)cqbuff & (NVME_PAGE_SIZE - 1)) == 0);

    ctl->adminq = (struct NvmeQueueAttributes){
            .id = 0,
            .size = size,
            .sq = sqbuff,
            .cq = cqbuff,
            .sq_doorbell = NVME_SQnTDBL(ctl, 0),
            .cq_doorbell = NVME_CQnHDBL(ctl, 0),
            .sq_tail = 0,
            .cq_head = 0,
    };

    /* Initialize NVMe contoller Admin Queue Attributes (AQA), Admin Submission
     * Queue Base Address (ASQ), and Admin Completion Queue Base Address (ACQ)
     * to appropriate values */

    /* We have 1 completion and 1 submission admin queue */
    *NVME_REG32(base, NVME_REG_AQA) = ((NVME_QUEUE_SIZE - 1) << 16) + NVME_QUEUE_SIZE - 1;

    struct NvmeQueueAttributes *adminq = &ctl->adminq;

    /* Set admin submission and completion queues' physical addresses */
    *NVME_REG64(base, NVME_REG_ASQ) = get_phys_addr(adminq->sq);
    *NVME_REG64(base, NVME_REG_ACQ) = get_phys_addr(adminq->cq);

    DEBUG("Admin queue size = %d, sq va = %p, sq pa = %lx, cq va = %p, cq pa = %lx, cq_doorbell = %x, sq_doorbell = %x",
          adminq->size, adminq->sq, get_phys_addr(adminq->sq),
          adminq->cq, get_phys_addr(adminq->sq), adminq->cq_doorbell, adminq->sq_doorbell);

    return NVME_OK;
}

static int
nvme_setup_caps(struct NvmeController *ctl) {
    volatile uint8_t *base = ctl->mmio_base_addr;
    union NvmeCapability cap = {.value = *NVME_REG64(base, NVME_REG_CAP)};

    /* Verify that NVMe command set is supported */
    if (!(cap.css & 1)) {
        ERROR("NVMe command set unsupported!");
        return -NVME_UNSUPPORTED;
    }

    ctl->ci.timeout = cap.to;    /* in 500ms units */
    ctl->ci.mpsmin = cap.mpsmin; /* 2 ^ (12 + MPSMIN) */
    ctl->ci.mpsmax = cap.mpsmax; /* 2 ^ (12 + MPSMAX) */
    ctl->ci.pageshift = 12 + ctl->ci.mpsmin;
    ctl->ci.pagesize = 1 << ctl->ci.pageshift;
    ctl->ci.maxqsize = cap.mqes + 1;
    ctl->ci.dbstride = 4 << cap.dstrd; /* in bytes */

    /* Set limit to 1 PRP list page per IO submission */
    ctl->ci.maxppio = ctl->ci.pagesize / sizeof(uint64_t);

    DEBUG("cap=%#lx mps=%u-%u to=%u maxqs=%u dbs=%u",
          cap.value, cap.mpsmin, cap.mpsmax, cap.to,
          ctl->ci.maxqsize, ctl->ci.dbstride);

    /* Set IO command set and 4KB I/O entry size */
    uint32_t new_cc = (NVME_CSS_IO_SET << NVME_CC_CSS) |
                      ((ctl->ci.pageshift - 12) << NVME_CC_MPS) |
                      (4 << NVME_CC_IOCQES) |
                      (6 << NVME_CC_IOSQES);
    *NVME_REG32(base, NVME_REG_CC) = new_cc;

    return NVME_OK;
}

static int
nvme_device_init(struct NvmeController *ctl) {
    int err;
    volatile uint8_t *base = ctl->mmio_base_addr;
    if (!base) {
        cprintf("NVMe device is not mapped!\n");
        return -NVME_UNSUPPORTED;
    }

    nvme_wait_ready(ctl);

    /* Disable NVMe Contoller */
    *NVME_REG32(base, NVME_REG_CC) &= ~NVME_CC_EN;

    /* Setup controller configuration */
    err = nvme_setup_caps(ctl);
    if (err)
        return err;

    /* Setup admin queues */
    err = nvme_setup_admin_queue(ctl, NVME_QUEUE_SIZE, ctl->buffer, ctl->buffer + NVME_PAGE_SIZE);
    if (err)
        return err;

    /* Enable the controller */
    *NVME_REG32(base, NVME_REG_CC) |= NVME_CC_EN;
    nvme_wait_ready(ctl);

    cprintf("NVMe enabled\n");
    return NVME_OK;
}

static inline void
copy_trimmed(char *dst, char *src, size_t len) {
    memcpy(dst, src, len);
    for (size_t i = len - 1; i > 0 && dst[i] == ' '; i--)
        dst[i] = 0;
}

static int
nvme_identify_controller(struct NvmeController *ctl) {
    uint8_t *buff = ctl->buffer + NVME_PAGE_SIZE * 2;
    uintptr_t buff_pa = get_phys_addr(buff);

    /* Reuse submission queue buffer as result
     * buffer for the identify command */
    if (nvme_acmd_identify(ctl, 0, buff_pa, 0)) {
        ERROR("nvme identify command failed");
        return -NVME_UNSUPPORTED;
    }

    struct NvmeIdentifyControllerData *idc = (void *)buff;
    struct NvmeContollerInfo *ci = &ctl->ci;
    ci->nscount = idc->nn;
    ci->vid = idc->vid;

    copy_trimmed(ci->mn, ci->mn, sizeof(ci->mn));
    copy_trimmed(ci->sn, ci->sn, sizeof(ci->sn));
    copy_trimmed(ci->fr, ci->fr, sizeof(ci->fr));

    if (idc->mdts) {
        int mp = 2 << (idc->mdts - 1);
        if (ci->maxppio > mp)
            ci->maxppio = mp;
    }

    union NvmeFeatureQNum qnum;

    if (nvme_acmd_get_features(ctl, 0, NVME_FEATURE_NUM_QUEUES, 0, 0, &qnum.value)) {
        ERROR("nvme_acmd_get_features number of queues failed");
        return -NVME_UNSUPPORTED;
    }

    ci->maxqcount = (qnum.nsq < qnum.ncq ? qnum.nsq : qnum.ncq) + 1;
    ci->qcount = MIN(ci->maxqcount, NVME_QUEUE_COUNT);
    ci->qsize = MIN(ci->maxqsize, NVME_QUEUE_SIZE);

    DEBUG("maxcount = %d, qcount = %d, qsize = %d",
          ci->maxqcount, ci->qcount, ci->qsize);

    return NVME_OK;
}

static int
nvme_identify_namespace(struct NvmeController *ctl, uint32_t id) {
    uint8_t *buff = ctl->buffer + NVME_PAGE_SIZE * 2;
    uintptr_t buff_pa = get_phys_addr(buff);

    if (nvme_acmd_identify(ctl, id, buff_pa, 0)) {
        ERROR("Identify cmd failed");
        return -NVME_ACMD_FAILED;
    }

    struct NvmeNamespaceInfo *ns = &ctl->nsi;
    struct NvmeIdentifyNamespaceData *idns = (void *)buff;

    ns->id = id;
    ns->blockcount = idns->ncap;
    ns->blockshift = idns->lbaf[idns->flbas & 0xF].lbads;
    ns->blocksize = 1 << ns->blockshift;
    ns->bpshift = ctl->ci.pageshift - ns->blockshift;
    ns->nbpp = 1 << ns->bpshift;
    ns->pagecount = ns->blockcount >> ns->bpshift;
    ns->maxbpio = ctl->ci.maxppio << ns->bpshift;

    DEBUG("Namespace initialized: bcnt = %ld, bshift = %d, bsize = %d, bpshift = %d, nbpp = %d, pagecnt = %ld, maxbpio = %d",
          ns->blockcount, ns->blockshift,
          ns->blocksize, ns->bpshift,
          ns->nbpp, ns->pagecount, ns->maxbpio);

    return 0;
}

/* Create an I/O queue with qid (starting with 0). */
static int
nvme_setup_io_queue(struct NvmeController *ctl, int qid) {
    DEBUG("Creating io queue %d", qid + 1);
    struct NvmeQueueAttributes *ioq = &ctl->ioq[qid];

    uint8_t *sqbuff = ctl->buffer + NVME_PAGE_SIZE * (2 * (qid + 1));
    uint8_t *cqbuff = ctl->buffer + NVME_PAGE_SIZE * (2 * (qid + 1) + 1);

    DEBUG("sqbuff %p, cqbuff %p", sqbuff, cqbuff);

    *ioq = (struct NvmeQueueAttributes){
            .id = qid + 1,
            .sq = (void *)sqbuff,
            .cq = (void *)cqbuff,
            .size = NVME_QUEUE_SIZE,
            .sq_doorbell = NVME_SQnTDBL(ctl, qid + 1),
            .cq_doorbell = NVME_CQnHDBL(ctl, qid + 1),
    };

    int err = nvme_acmd_create_cq(ctl, ioq, get_phys_addr(cqbuff));
    if (err) {
        ERROR("nvme_acmd_create_cq() failed. quid=%d, err=%d", qid + 1, err);
        return -NVME_ACMD_FAILED;
    }

    err = nvme_acmd_create_sq(ctl, ioq, get_phys_addr(sqbuff));
    if (err) {
        ERROR("nvme_acmd_create_sq() failed. quid=%d, err=%d", qid + 1, err);
        return -NVME_ACMD_FAILED;
    }

    return 0;
}

int
nvme_init(void) {
    struct NvmeController *ctl = &nvme;
    int err;

    struct PciDevice *pcidevice = find_pci_dev(1, 8);
    if (pcidevice == NULL)
        panic("NVMe device not found\n");

    ctl->pcidev = pcidevice;

    err = nvme_map(ctl);
    if (err)
        panic("NVMe registers mapping failed\n");

    err = nvme_alloc_queues(ctl);
    if (err)
        panic("Unable to allocate NVMe queues\n");

    err = nvme_device_init(ctl);
    if (err)
        panic("NVMe device initialization failed\n");

    err = nvme_identify_controller(ctl);
    if (err)
        panic("NVMe contoller identification failed\n");

    err = nvme_identify_namespace(ctl, NVME_NSID);
    if (err)
        panic("NVMe namespace identification failed\n");

    err = nvme_setup_io_queue(ctl, 0);
    if (err)
        panic("NVMe queue initialization failed\n");

#ifdef PCIE_DEBUG
    nvme_dump_status(ctl);
#endif

    DEBUG("NVMe initialization completed!");

    return NVME_OK;
}


/**
 * Check a completion queue and return the completed command id and status.
 * @param   ctl         nvme device context
 * @param   q           queue
 * @param   stat        completion status returned
 * @param   cqe_cs      CQE command specific DW0 returned
 * @return  the completed command id or -1 if there's no completion.
 */
static int
nvme_check_completion(struct NvmeController *ctl, struct NvmeQueueAttributes *q, int *stat, uint32_t *cqe_cs) {
    *stat = 0;
    volatile struct NvmeCQE *cqe = &q->cq[q->cq_head];

    if (cqe->p == q->cq_phase)
        return -1;

    *stat = cqe->psf & 0xFFFE;

    if (++q->cq_head == q->size) {
        q->cq_head = 0;
        q->cq_phase = !q->cq_phase;
    }

    if (cqe_cs)
        *cqe_cs = cqe->cs;

    *NVME_REG32(ctl->mmio_base_addr, q->cq_doorbell) = q->cq_head;

    if (*stat == 0) {
        DEBUG("q=%d cq=%d sq=%d-%d cid=%#x (C)", q->id, q->cq_head, q->sq_head, q->sq_tail, cqe->cid);
    } else {
        ERROR("q=%d cq=%d sq=%d-%d cid=%#x stat=%#x (dnr=%d m=%d sct=%d sc=%#x) (C)",
              q->id, q->cq_head, q->sq_head, q->sq_tail, cqe->cid, *stat, cqe->dnr, cqe->m, cqe->sct, cqe->sc);
    }

    return cqe->cid;
}

/**
 * Wait for a given command completion until timeout.
 * @param   ctl         nvme device context
 * @param   q           queue
 * @param   cid         cid
 * @param   timeout     timeout in seconds
 * @return  completion status (0 if ok).
 */
static int
nvme_wait_completion(struct NvmeController *ctl, struct NvmeQueueAttributes *q, int cid, int timeout) {
    uint64_t endtsc = 0;

    do {
        int stat;
        int ret = nvme_check_completion(ctl, q, &stat, NULL);
        if (ret >= 0) {
            if (ret == cid && stat == 0) {
                return NVME_OK;
            }

            if (ret != cid) {
                ERROR("cid wait=%#x recv=%#x", cid, ret);
                stat = -1;
            }

            return stat;
        } else if (endtsc == 0) {
            endtsc = read_tsc() + (uint64_t)timeout * tsc_freq;
        }
    } while (read_tsc() < endtsc);

    return -NVME_CMD_TIMEOUT;
}

static int
nvme_submit_cmd(struct NvmeController *ctl, struct NvmeQueueAttributes *q) {
    DEBUG("sq_tail = %d, base_addr = %p, drbl = %x",
          q->sq_tail, ctl->mmio_base_addr, q->sq_doorbell);
    q->sq_tail = (q->sq_tail + 1) % q->size;
    *NVME_REG32(ctl->mmio_base_addr, q->sq_doorbell) = q->sq_tail;

    return NVME_OK;
}

static int
nvme_acmd_identify(struct NvmeController *ctl, int nsid, uint64_t prp1, uint64_t prp2) {
    struct NvmeQueueAttributes *adminq = &ctl->adminq;
    int cid = adminq->sq_tail;
    struct NvmeACmdIdentify *cmd = &adminq->sq[cid].identify;

    DEBUG("sq = %d - %d cid = %#x nsid = %d, prp1 = %lx, prp2 = %lx", adminq->sq_head, adminq->sq_tail, cid, nsid, prp1, prp2);

    memset(cmd, 0, sizeof(*cmd));
    cmd->common.opc = NVME_ACMD_IDENTIFY;
    cmd->common.cid = cid;
    cmd->common.nsid = nsid;
    cmd->common.prp[0] = prp1;
    cmd->common.prp[1] = prp2;
    cmd->cns = nsid == 0 ? 1 : 0;

    int err = nvme_submit_cmd(ctl, adminq);
    if (err)
        return err;

    err = nvme_wait_completion(ctl, adminq, cid, 900);
    return err;
}

/**
 * NVMe get features command.
 * Submit the command and wait for completion.
 * @param   ctl         nvme device context
 * @param   nsid        namespace id
 * @param   fid         feature id
 * @param   prp1        PRP1 address
 * @param   prp2        PRP2 address
 * @param   res         dword 0 value returned
 * @return  completion status (0 if ok).
 */
static int
nvme_acmd_get_features(struct NvmeController *ctl, int nsid, int fid, uint64_t prp1, uint64_t prp2, uint32_t *res) {
    struct NvmeQueueAttributes *adminq = &ctl->adminq;
    int cid = adminq->sq_tail;
    struct NvmeACmdGetFeatures *cmd = &adminq->sq[cid].get_features;

    memset(cmd, 0, sizeof(*cmd));
    cmd->common.opc = NVME_ACMD_GET_FEATURES;
    cmd->common.cid = cid;
    cmd->common.nsid = nsid;
    cmd->common.prp[0] = prp1;
    cmd->common.prp[1] = prp2;
    cmd->fid = fid;
    *res = -1;

    DEBUG("sq=%d-%d cid=%#x fid=%d", adminq->sq_head, adminq->sq_tail, cid, fid);

    int err = nvme_submit_cmd(ctl, adminq);
    if (err)
        return err;

    err = nvme_wait_completion(ctl, adminq, cid, 30);
    if (err)
        return err;

    *res = adminq->cq[cid].cs;
    return err;
}

/**
 * NVMe set features command.
 * Submit the command and wait for completion.
 * @param   ctl         nvme device context
 * @param   nsid        namespace id
 * @param   fid         feature id
 * @param   prp1        PRP1 address
 * @param   prp2        PRP2 address
 * @param   res         dword 0 value returned
 * @return  completion status (0 if ok).
 */
static int
nvme_acmd_set_features(struct NvmeController *ctl, int nsid, int fid, uint64_t prp1, uint64_t prp2, uint32_t *res) {
    struct NvmeQueueAttributes *adminq = &ctl->adminq;
    int cid = adminq->sq_tail;
    struct NvmeACmdSetFeatures *cmd = &adminq->sq[cid].set_features;

    memset(cmd, 0, sizeof(*cmd));
    cmd->common.opc = NVME_ACMD_SET_FEATURES;
    cmd->common.cid = cid;
    cmd->common.nsid = nsid;
    cmd->common.prp[0] = prp1;
    cmd->common.prp[1] = prp2;
    cmd->fid = fid;
    cmd->value = *res;
    *res = -1;

    DEBUG("t=%d h=%d cid=%#x fid=%d", adminq->sq_tail, adminq->sq_head, cid, fid);

    int err = nvme_submit_cmd(ctl, adminq);
    if (err)
        return err;

    err = nvme_wait_completion(ctl, adminq, cid, 30);
    if (err)
        return err;

    *res = adminq->cq[cid].cs;
    return err;
}

/**
 * NVMe create I/O completion queue command.
 * Submit the command and wait for completion.
 * @param   ioq         io queue
 * @param   prp         PRP1 address
 * @return  0 if ok, else -1.
 */
static int
nvme_acmd_create_cq(struct NvmeController *ctl, struct NvmeQueueAttributes *ioq, uint64_t prp) {
    struct NvmeQueueAttributes *adminq = &ctl->adminq;
    int cid = adminq->sq_tail;
    struct NvmeACmdCreateCompletionQueue *cmd = &adminq->sq[cid].create_cq;

    memset(cmd, 0, sizeof(*cmd));
    cmd->common.opc = NVME_ACMD_CREATE_CQ;
    cmd->common.cid = cid;
    cmd->common.prp[0] = prp;
    cmd->pc = 1;
    cmd->qid = ioq->id;
    cmd->qsize = ioq->size - 1;

    DEBUG("sq=%d-%d cid=%#x cq=%d qs=%d", adminq->sq_head, adminq->sq_tail, cid, ioq->id, ioq->size);

    int err = nvme_submit_cmd(ctl, adminq);
    if (err)
        return err;

    err = nvme_wait_completion(ctl, adminq, cid, 300);

    return err;
}

/**
 * NVMe create I/O submission queue command.
 * Submit the command and wait for completion.
 * @param   ioq         io queue
 * @param   prp         PRP1 address
 * @return  0 if ok, else -1.
 */
static int
nvme_acmd_create_sq(struct NvmeController *ctl, struct NvmeQueueAttributes *ioq, uint64_t prp) {
    struct NvmeQueueAttributes *adminq = &ctl->adminq;
    int cid = adminq->sq_tail;
    struct NvmeACmdCreateSubmissionQueue *cmd = &adminq->sq[cid].create_sq;

    memset(cmd, 0, sizeof(*cmd));

    cmd->common.opc = NVME_ACMD_CREATE_SQ;
    cmd->common.cid = cid;
    cmd->common.prp[0] = prp;
    cmd->pc = 1;
    cmd->qprio = 2; // 0=urgent 1=high 2=medium 3=low
    cmd->qid = ioq->id;
    cmd->cqid = ioq->id;
    cmd->qsize = ioq->size - 1;

    DEBUG("sq = %d - %d, cid = %#x, cq = %d, qs = %d", adminq->sq_head, adminq->sq_tail, cid, ioq->id, ioq->size);

    int err = nvme_submit_cmd(ctl, adminq);
    if (err)
        return err;

    err = nvme_wait_completion(ctl, adminq, cid, 30);

    return err;
}

/**
 * NVMe submit a read write command.
 * @param   ioq         io queue
 * @param   opc         op code
 * @param   cid         command id
 * @param   nsid        namespace
 * @param   slba        starting logical block address
 * @param   nlb         number of logical blocks
 * @param   prp1        PRP1 address
 * @param   prp2        PRP2 address
 * @return  0 if ok else errcode != 0.
 */
static int
nvme_cmd_rw(struct NvmeController *ctl, struct NvmeQueueAttributes *ioq, int opc,
            int nsid, uint64_t slba, int nlb, uint64_t prp1, uint64_t prp2) {
    /* Create new NvmeCmdRW in ctl->ioq[0].
     * TIP: Look at the definition of the struct NvmeCmdRW for description of fields.
     *      Note the 'minus 1' for nlbs.
     * TIP: Fields common.fuse, common.psdt, mptr, prinfo, fua, lr, dsm, eilbrt, elbat
     *      and elbatm should remain zeroed. They are not used here.
     * TIP: Use ioq->sq_tail as cid like it is done in other commands for simplicity. */
    // LAB 10: Your code here

    DEBUG("q = %d, sq = %d - %d, cid = %#x, nsid = %d, lba = %#lx, nb = %#x, prp = %#lx.%#lx (%c)",
          ioq->id, ioq->sq_head, ioq->sq_tail, cid, nsid, slba, nlb, prp1, prp2,
          opc == NVME_CMD_READ ? 'R' : 'W');

    /* Submit the command and synchronously wait for its completion
     * TIP: Use nvme_submit_cmd() and nvme_wait_completion(). Don't
     *      forget to check for potential errors! */
    // LAB 10: Your code here

    int err = -NVME_IOCMD_FAILED;

    return err;
}

int
nvme_write(uint64_t secno, const void *src, size_t nsecs) {
    if (!src)
        return -NVME_BAD_ARG;

    return nvme_cmd_rw(&nvme, &nvme.ioq[0], NVME_CMD_WRITE,
                       nvme.nsi.id, secno, nsecs, get_phys_addr((void *)src), 0);
}


int
nvme_read(uint64_t secno, void *dst, size_t nsecs) {
    if (!dst)
        return -NVME_BAD_ARG;

    /* Submit NVME_CMD_READ to ioq[0].
     * TIP: This is achieved in exactly the same way as the write command.
     *      Remember that the command takes physical address as an argument
     *      and 'dst' is a virtual address. */
    // LAB 10: Your code here

    return -1;
}
