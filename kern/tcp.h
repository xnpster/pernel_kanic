#ifndef JOS_KERN_TCP_H
#define JOS_KERN_TCP_H

#include <inc/types.h>
#include <kern/ip.h>


struct tcp_pseudo_hdr
{
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t reserved;
    uint8_t proto;
    uint16_t tcp_len;
} __attribute__((packed));

struct tcp_hdr
{

  uint16_t source_port;
  uint16_t destination_port;
  uint32_t seq;
  uint32_t ack_seq;
  uint8_t  data_offset;  // 4 bits
  uint8_t  flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent_p;
}  __attribute__((packed));

#define DATA_OFFSET(data_offset) (((data_offset) >> 4) & 0xF)
#define TCP_DATA_LENGTH 1024

enum {
    TCP_FIN = 0,
    TCP_SYN,
    TCP_RST,
    TCP_PSH,
    TCP_ACK,
    TCP_URG,
    TCP_ECE,
    TCP_CWR,
    TCP_NS
};

struct tcp_pkt {
    struct tcp_hdr hdr;
    uint8_t data[TCP_DATA_LENGTH];
}  __attribute__((packed));

int
tcp_recv(struct ip_pkt* pkt);

void setup_tcp();

#endif