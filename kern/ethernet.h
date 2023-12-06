#ifndef JOS_KERN_ETH_H
#define JOS_KERN_ETH_H

#include <inc/types.h>
#include <kern/e1000.h>

struct eth_hdr {
    uint8_t eth_destination_mac[6];
    uint8_t eth_source_mac[6];
    uint16_t eth_type;
} __attribute__((packed));

const uint8_t *get_my_mac(void);
int eth_send(struct eth_hdr* hdr, void* data, size_t len);
int eth_recv(void* data);

#define ETH_MAX_PACKET_SIZE 1500
#define ETH_HEADER_LEN sizeof(struct eth_hdr)
#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

#endif
