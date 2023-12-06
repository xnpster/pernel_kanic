#ifndef JOS_KERN_IP_H
#define JOS_KERN_IP_H

#include <inc/types.h>
#include <kern/ethernet.h>

struct ip_hdr {
    uint8_t ip_verlen;  // 4-bit IPv4 version 4-bit header length (in 32-bit words)
    uint8_t ip_tos;  // IP type of service
    uint16_t ip_total_length;  // Total length
    uint16_t ip_id;  // Unique identifier
    uint16_t ip_flags_offset;  // Flags + fragment offset field
    uint8_t ip_ttl;  // Time to live
    uint8_t ip_protocol;  // Protocol(TCP,UDP etc)
    uint16_t ip_header_checksum;  // IP checksum
    uint32_t ip_source_address;  // Source address
    uint32_t ip_destination_address;  // Destination address
} __attribute__((packed));

#define IP_HEADER_LEN  sizeof(struct ip_hdr)
#define IP_DATA_LEN (ETH_MAX_PACKET_SIZE - ETH_HEADER_LEN - IP_HEADER_LEN)

struct ip_pkt {
    struct ip_hdr hdr;
    uint8_t data[IP_DATA_LEN];
};

uint32_t ip2num(int8_t ip[4]);
void num2ip(int32_t num);
uint16_t ip_checksum(void* vdata, size_t length);
int ip_send(struct ip_pkt* pkt, uint16_t length);
int ip_recv(struct ip_pkt* pkt);

#define IP_VER 0x4
#define IP_HLEN    (IP_HEADER_LEN / sizeof(uint32_t))
#define IP_VER_LEN (IP_VER << 4 | IP_HLEN)
#define IP_TTL 64

#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP  17
#define IP_PROTO_TCP  6

#define IP(a, b, c, d) ( ( ( (uint32_t)( a ) ) << 24 ) + ( (uint32_t)( b ) << 16 ) + ( (uint32_t)( c ) << 8 ) + (uint32_t)( d ) )
#define MY_IP IP(192, 160, 144, 2)
#define HOST_IP IP(192, 160, 144, 1)

#endif
