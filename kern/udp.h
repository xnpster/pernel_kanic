#ifndef JOS_KERN_UDP_H
#define JOS_KERN_UDP_H

#include <inc/types.h>
#include <kern/ip.h>

struct udp_hdr {
    uint16_t source_port;
    uint16_t destination_port;
    uint16_t length;  // total length = header_length + data_length
    uint16_t checksum;
} __attribute__((packed));

#define UDP_HEADER_LEN sizeof(struct udp_hdr)
#define UDP_DATA_LENGTH (IP_DATA_LEN - UDP_HEADER_LEN)
#define UDP_MAX_PACKET_SIZE 2000

struct udp_pkt {
    struct udp_hdr hdr;
    uint8_t data[UDP_DATA_LENGTH];
} __attribute__((packed));

int udp_send(void* data, uint16_t length);
int udp_recv(struct ip_pkt* pkt);

#endif
