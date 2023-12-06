#ifndef JOS_KERN_ICMP_H
#define JOS_KERN_ICMP_H

#include <inc/types.h>
#include <kern/ip.h>

struct icmp_hdr {
    uint8_t msg_type;  // message type
    uint8_t msg_code;  // icmp message code
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence_number;
} __attribute__((packed));


#define ICMP_HEADER_LEN  sizeof(struct icmp_hdr)
#define ICMP_DATA_LEN (IP_DATA_LEN - ICMP_HEADER_LEN)

struct icmp_pkt {
    struct icmp_hdr hdr;
    uint8_t data[ICMP_DATA_LEN];
};

#define ECHO_REQUEST 8
#define ECHO_REPLY 0

int icmp_echo_reply(struct ip_pkt* pkt);

#endif