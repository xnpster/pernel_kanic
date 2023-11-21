#ifndef NVME_H
#define NVME_H
#include "pci.h"

#define PACKED     __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))

#define NVME_MAX_MAP_MEM 0x8000

/* NVMe registers */
#define NVME_REG_CAP    0x0
#define NVME_REG_VS     0x8
#define NVME_REG_INTMS  0xC
#define NVME_REG_INTMC  0xF
#define NVME_REG_CC     0x14
#define NVME_REG_CSTS   0x1c
#define NVME_REG_NSSR   0x20
#define NVME_REG_AQA    0x24
#define NVME_REG_ASQ    0x28
#define NVME_REG_ACQ    0x30
#define NVME_REG_CMBLOC 0x38
#define NVME_SQ0TDBL    0x1000

#define NVME_SQnTDBL(ctl, n) (NVME_SQ0TDBL + (2 * (n) * (ctl)->ci.dbstride))
#define NVME_CQnHDBL(ctl, n) (NVME_SQ0TDBL + ((2 * (n) + 1) * (ctl)->ci.dbstride))

/* CSTS */
#define NVME_CSTS_RDY 0x1

/* CAP */
#define NVME_CAP_CSS 0x7

/* CC */
#define NVME_CC_CRIME  24
#define NVME_CC_AMS    11
#define NVME_CC_PMS    7
#define NVME_CC_CSS    4
#define NVME_CC_IOCQES 20
#define NVME_CC_IOSQES 16
#define NVME_CC_MPS    7

/* Configuration options */
#define NVME_CC_EN        1
#define NVME_CSS_ADM_ONLY 0x7
#define NVME_CSS_IO_SET   0x6
#define NVME_CSS_NVME_SET 0x0
#define NVME_AMS_RR       0x0
#define NVME_INT_MASK     0xFFFFFFFF

/* NVMe options */
#define NVME_MAX_QUEUES  2
#define NVME_QUEUE_SIZE  32
#define NVME_QUEUE_COUNT 1
#define NVME_AQSIZE      16
#define NVME_PAGE_SIZE   4096

#define NVME_REG32(reg, offset) (volatile uint32_t *)((uint8_t *)(reg) + offset)
#define NVME_REG64(reg, offset) (volatile uint64_t *)((uint8_t *)(reg) + offset)

#define NVME_NSID 1

enum nvme_admin_cmd {
    NVME_ACMD_DELETE_SQ = 0x0,    /* Delete io submission queue */
    NVME_ACMD_CREATE_SQ = 0x1,    /* Create io submission queue */
    NVME_ACMD_GET_LOG_PAGE = 0x2, /* Get log page */
    NVME_ACMD_DELETE_CQ = 0x4,    /* Delete io completion queue */
    NVME_ACMD_CREATE_CQ = 0x5,    /* Create io completion queue */
    NVME_ACMD_IDENTIFY = 0x6,     /* Identify */
    NVME_ACMD_ABORT = 0x8,        /* Abort */
    NVME_ACMD_SET_FEATURES = 0x9, /* Set features */
    NVME_ACMD_GET_FEATURES = 0xA, /* Get features */
    NVME_ACMD_ASYNC_EVENT = 0xC,  /* Asynchronous event */
    NVME_ACMD_FW_ACTIVATE = 0x10, /* Firmware activate */
    NVME_ACMD_FW_DOWNLOAD = 0x11, /* Firmware image download */
};

/* NVMe feature identifiers */
enum nvme_feature {
    NVME_FEATURE_ARBITRATION = 0x1,     /* Arbitration */
    NVME_FEATURE_POWER_MGMT = 0x2,      /* Power management */
    NVME_FEATURE_LBA_RANGE = 0x3,       /* LBA range type */
    NVME_FEATURE_TEMP_THRESHOLD = 0x4,  /* Temperature threshold */
    NVME_FEATURE_ERROR_RECOVERY = 0x5,  /* Error recovery */
    NVME_FEATURE_WRITE_CACHE = 0x6,     /* Volatile write cache */
    NVME_FEATURE_NUM_QUEUES = 0x7,      /* Number of queues */
    NVME_FEATURE_INT_COALESCING = 0x8,  /* Interrupt coalescing */
    NVME_FEATURE_INT_VECTOR = 0x9,      /* Interrupt vector config */
    NVME_FEATURE_WRITE_ATOMICITY = 0xA, /* Write atomicity */
    NVME_FEATURE_ASYNC_EVENT = 0xB,     /* Async event config */
};

/* NVMe command op code */
enum nvme_cmd {
    NVME_CMD_FLUSH = 0x0,       /* Flush */
    NVME_CMD_WRITE = 0x1,       /* Write */
    NVME_CMD_READ = 0x2,        /* Read */
    NVME_CMD_WRITE_UNCOR = 0x4, /* Write uncorrectable */
    NVME_CMD_COMPARE = 0x5,     /* Compare */
    NVME_CMD_DS_MGMT = 0x9,     /* Dataset management */
};

/* NVMe error codes */
enum nvme_error {
    NVME_OK = 0,              /* OK */
    NVME_BAD_ARG = 1,         /* Bad argument */
    NVME_NO_DEVICE = 2,       /* No NVMe device */
    NVME_MAP_ERR = 3,         /* Unable to map NVMe device */
    NVME_UNSUPPORTED = 4,     /* Nvme device unsupported */
    NVME_ACMD_FAILED = 5,     /* Admin command failed */
    NVME_IOQ_INIT_FAILED = 6, /* I/O queue initialization failed */
    NVME_IOCMD_FAILED = 7,    /* I/O command failed */
    NVME_ALLOC_FAILED = 8,    /* Failed to allocate buffer */
    NVME_CMD_TIMEOUT = 9      /* Waiting time for command completion has expired */
};

/* Submission queue entry, common part */
struct NvmeCmdCommon {
    uint8_t opc;       /* CDW 0: Opcode */
    uint8_t fuse : 2;  /*        Fused operation */
    uint8_t rsvd1 : 4; /*        Reserved */
    uint8_t psdt : 2;  /*        PRP or SGL for data transfer */
    uint16_t cid;      /*        Command identifier */

    uint32_t nsid;   /* CWD 1: Namespace identifier */
    uint64_t rsvd2;  /* CDW 2-3: Reserved */
    uint64_t mptr;   /* CDW 4-5: metadata pointer */
    uint64_t prp[2]; /* CDW 6-9: First and second PRP entries */
} PACKED;

/* Command: Identify */
struct NvmeACmdIdentify {
    struct NvmeCmdCommon common; /* CDW 0-9: Common part */
    uint32_t cns;                /* CDW 10: Controller or namespace */
    uint32_t rsvd1[5];           /* CDW 11-15: Reserved */
} PACKED ALIGNED(64);

/* Command: Read/Write */
struct NvmeCmdRW {
    struct NvmeCmdCommon common; /* CDW 0-9: Common part */
    uint64_t slba;               /* CDW 10-11: Starting LBA */
    uint16_t nlb;                /* CDW 12: Number of logical blocks minus 1 */
    uint16_t rsvd1 : 10;         /*         Reserved */
    uint16_t prinfo : 4;         /*         Protection information field */
    uint16_t fua : 1;            /*         Force unit access */
    uint16_t lr : 1;             /*         Limited retry */
    uint8_t dsm;                 /* CDW 13: Dataset management */
    uint8_t rsvd2[3];            /*         Reserved */
    uint32_t eilbrt;             /* CDW 14: Exp. initial block reference tag */
    uint16_t elbat;              /* CDW 15: Exp. logical block app tag */
    uint16_t elbatm;             /*         Exp. logical block app tag mask */
} PACKED ALIGNED(64);

/* Admin command: Get Features */
struct NvmeACmdGetFeatures {
    struct NvmeCmdCommon common; /* CDW 0-9: Common part */
    uint8_t fid;                 /* CDW 10: Feature ID */
    uint8_t rsvd1[3];            /*         Reserved */
} PACKED ALIGNED(64);

/* Admin command: Set Features */
struct NvmeACmdSetFeatures {
    struct NvmeCmdCommon common; /* CDW 0-9: Common part */
    uint8_t fid;                 /* CDW 10: Feature ID */
    uint8_t rsvd1[3];            /*         Reserved */
    uint32_t value;              /* CDW 11: Value */
} PACKED ALIGNED(64);

/* Admin command: Create I/O Submission Queue */
struct NvmeACmdCreateSubmissionQueue {
    struct NvmeCmdCommon common; /* CDW 0-9: Common part */
    uint16_t qid;                /* CDW 10: Queue ID */
    uint16_t qsize;              /*         Queue size */
    uint16_t pc : 1;             /* CDW 11: Queue is physically contiguous */
    uint16_t qprio : 2;          /*         Interrupts are enabled */
    uint16_t rsvd1 : 13;         /*         Reserved */
    uint16_t cqid;               /*         Associated completion queue ID */
    uint32_t rsvd2[4];           /* CDW 12-15: Reserved */
} PACKED ALIGNED(64);

/* Admin command:  Create I/O Completion Queue */
struct NvmeACmdCreateCompletionQueue {
    struct NvmeCmdCommon common; /* CDW 0-9: Common part */
    uint16_t qid;                /* CDW 10: Queue ID */
    uint16_t qsize;              /*         Queue size */
    uint16_t pc : 1;             /* CDW 11: Queue is physically contiguous */
    uint16_t ien : 1;            /*         Interrupt are enabled */
    uint16_t rsvd1 : 14;         /*         Reserved */
    uint16_t iv;                 /*         Interrupt vector */
    uint32_t rsvd2[4];           /* CDW 12-15: Reserved */
} PACKED ALIGNED(64);

/* Admin feature: Number of Queues */
union NvmeFeatureQNum {
    uint32_t value;
    struct {
        uint16_t nsq; /* Number of submission queues */
        uint16_t ncq; /* Number of completion queues */
    };
};

/* Submission Queue Entry */
union NvmeSQE {
    struct NvmeCmdRW rw;
    struct NvmeACmdIdentify identify;
    struct NvmeACmdGetFeatures get_features;
    struct NvmeACmdSetFeatures set_features;
    struct NvmeACmdCreateSubmissionQueue create_sq;
    struct NvmeACmdCreateCompletionQueue create_cq;
} PACKED ALIGNED(64);

/* Completion Queue Entry */
struct NvmeCQE {
    uint32_t cs;    /* CDW 0 */
    uint32_t rsvd1; /* CDW 1: Reserved */
    uint16_t sqhd;  /* CDW 2: Submission queue head pointer */
    uint16_t sqid;  /*        Submission queue identifier */
    uint16_t cid;   /* CDW 3: Command ID */
    union {
        uint16_t psf; /* Phase bit and status field */
        struct {
            uint16_t p : 1;     /* Phase tag ID */
            uint16_t sc : 8;    /* Status code */
            uint16_t sct : 3;   /* Status code type */
            uint16_t rsvd3 : 2; /* Reserved */
            uint16_t m : 1;     /* More */
            uint16_t dnr : 1;   /* Do not retry */
        };
    };
} PACKED ALIGNED(16);

union NvmeCapability {
    uint64_t value; /* Raw value */
    struct {
        uint16_t mqes;       /* Max queue entries supported */
        uint8_t cqr : 1;     /* Contiguous queues required */
        uint8_t ams : 2;     /* Arbitration mechanism supported */
        uint8_t rsvd1 : 5;   /* Reserved */
        uint8_t to;          /* Timeout */
        uint32_t dstrd : 4;  /* Doorbell stride */
        uint32_t nssrs : 1;  /* NVM subsystem reset supported */
        uint32_t css : 8;    /* Command set supported */
        uint32_t rsvd2 : 3;  /* Reserved */
        uint32_t mpsmin : 4; /* Memory page size minimum */
        uint32_t mpsmax : 4; /* Memory page size maximum */
        uint32_t rsvd3 : 8;  /* Reserved */
    };
} PACKED ALIGNED(8);

struct NvmeLBAFormat {
    uint16_t ms;      /* Metadata size */
    uint8_t lbads;    /* LBA data size */
    uint8_t rp : 2;   /* Relative performance */
    uint8_t rsvd : 6; /* Reserved */
} PACKED;

/* Identify Namespace */
struct NvmeIdentifyNamespaceData {
    uint64_t nsze;                 /* Namespace size */
    uint64_t ncap;                 /* Namespace capacity */
    uint64_t nuse;                 /* Namespace utilization */
    uint8_t nsfeat;                /* Namespace features */
    uint8_t nlbaf;                 /* Number of LBA formats */
    uint8_t flbas;                 /* Formatted LBA size */
    uint8_t mc;                    /* Metadata capabilities */
    uint8_t dpc;                   /* Data protection capabilities */
    uint8_t dps;                   /* Data protection settings */
    uint8_t rsvd1[98];             /* Reserved (30-127) */
    struct NvmeLBAFormat lbaf[16]; /* Lba format support */
    uint8_t rsvd2[192];            /* Reserved (383-192) */
    uint8_t vs[3712];              /* Vendor specific */
} PACKED ALIGNED(NVME_PAGE_SIZE);

/* Admin data: Identify Controller */
struct NvmeIdentifyControllerData {
    uint16_t vid;          /* PCI vendor id */
    uint16_t ssvid;        /* PCI subsystem vendor id */
    char sn[20];           /* Serial number */
    char mn[40];           /* Model number */
    char fr[8];            /* Firmware revision */
    uint8_t rab;           /* Recommended arbitration burst */
    uint8_t ieee[3];       /* IEEE OUI identifier */
    uint8_t mic;           /* Multi-interface capabilities */
    uint8_t mdts;          /* Max data transfer size */
    uint8_t rsvd78[178];   /* Reserved (78-255) */
    uint16_t oacs;         /* Optional admin command support */
    uint8_t acl;           /* Abort command limit */
    uint8_t aerl;          /* Async event request limit */
    uint8_t frmw;          /* Firmware updates */
    uint8_t lpa;           /* Log page attributes */
    uint8_t elpe;          /* Error log page entries */
    uint8_t npss;          /* Number of power states support */
    uint8_t avscc;         /* Admin vendor specific config */
    uint8_t rsvd265[247];  /* Reserved (265-511) */
    uint8_t sqes;          /* Submission queue entry size */
    uint8_t cqes;          /* Completion queue entry size */
    uint8_t rsvd514[2];    /* Reserved (514-515) */
    uint32_t nn;           /* Number of namespaces */
    uint16_t oncs;         /* Optional NVM command support */
    uint16_t fuses;        /* Fused operation support */
    uint8_t fna;           /* Format NVM attributes */
    uint8_t vwc;           /* Volatile write cache */
    uint16_t awun;         /* Atomic write unit normal */
    uint16_t awupf;        /* Atomic write unit power fail */
    uint8_t nvscc;         /* NVM vendor specific config */
    uint8_t rsvd531[173];  /* Reserved (531-703) */
    uint8_t rsvd704[1344]; /* Reserved (704-2047) */
    uint8_t psd[1024];     /* Power state 0-31 descriptors */
    uint8_t vs[1024];      /* Vendor specific */
} PACKED ALIGNED(NVME_PAGE_SIZE);

struct NvmeQueueAttributes {
    uint32_t id;   /* Queue ID */
    uint32_t size; /* Queue size */

    union NvmeSQE *sq;  /* Submission Queue Virtual Address */
    struct NvmeCQE *cq; /* Completion Queue Virtual Address */

    uint32_t sq_doorbell; /* Submission queue doorbell */
    uint32_t cq_doorbell; /* Completion queue doorbell */
    uint32_t sq_head;     /* Submission queue head */
    uint32_t sq_tail;     /* Submission queue tail */
    uint32_t cq_head;     /* Completion queue head */
    bool cq_phase;        /* Completion queue phase bit */
};

struct NvmeContollerInfo {
    uint16_t vid;       /* Vendor ID */
    char mn[40];        /* Model number */
    char sn[20];        /* Serial number */
    char fr[8];         /* Firmware revision */
    uint16_t pagesize;  /* Page size */
    uint16_t pageshift; /* Page size shift value */
    uint16_t maxppio;   /* Max number of pages per I/O */
    uint16_t nscount;   /* Number of namespaces available */
    uint32_t qcount;    /* Number of I/O queues */
    uint32_t qsize;     /* I/O queue size */
    uint32_t maxqcount; /* Max number of queues supported */
    uint32_t maxqsize;  /* Max queue size supported */
    uint16_t dbstride;  /* Doorbell stride (in bytes) */
    uint16_t timeout;   /* In 500 ms units */
    uint16_t mpsmin;    /* MPSMIN */
    uint16_t mpsmax;    /* MPSMAX */
};

struct NvmeNamespaceInfo {
    uint16_t id;         /* Namespace ID */
    uint64_t blockcount; /* Total number of available blocks */
    uint64_t pagecount;  /* Total number of available pages */
    uint16_t blocksize;  /* Logical block size */
    uint16_t blockshift; /* Block size shift value */
    uint16_t bpshift;    /* Block to page shift */
    uint16_t nbpp;       /* Number of blocks per page */
    uint16_t maxbpio;    /* Max number of blocks per I/O */
};

struct NvmeController {
    struct PciDevice *pcidev; /* Associated PCI device */

    /* NVME device data */
    struct NvmeContollerInfo ci;  /* Controller info*/
    struct NvmeNamespaceInfo nsi; /* I/O namespace info */

    volatile uint8_t *mmio_base_addr;

    /* This buffer is used for all NVMe command queues, one 4KB page per queue:
     * 1st 4kB boundary is the start of the admin submission queue.
     * 2nd 4kB boundary is the start of the admin completion queue.
     * 3rd 4kB boundary is the start of I/O submission queue #1.
     * 4th 4kB boundary is the start of I/O completion queue #1. */
    uint8_t *buffer;

    struct NvmeQueueAttributes adminq;
    struct NvmeQueueAttributes ioq[NVME_QUEUE_COUNT];
};


int nvme_init(void);

int nvme_write(uint64_t secno, const void *src, size_t nsecs);
int nvme_read(uint64_t secno, void *dst, size_t nsecs);
#endif
