#include <inc/string.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <kern/inet.h>
#include <kern/ethernet.h>
#include <kern/ip.h>
#include <kern/icmp.h>
#include <kern/udp.h>
#include <kern/tcp.h>
#include <kern/traceopt.h>

void
num2ip(int32_t num) {
    cprintf(" %d.%d.%d.%d ", (num >> 24) & 0xFF, (num >> 16) & 0xFF, (num >> 8) & 0xFF, num & 0xFF);
}

uint16_t
ip_checksum(void* vdata, size_t length) {
    char* data = vdata;
    uint32_t acc = 0xffff;
    for (size_t i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += JNTOHS(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }
    // Handle any partial block at the end of the data.
    if (length & 1) {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += JNTOHS(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }
    // Return the checksum in network byte order.
    return JHTONS(~acc);
}


int
ip_send(struct ip_pkt* pkt, uint16_t length) {
    if (trace_packet_processing) cprintf("Sending IP packet\n");
    static uint16_t packet_id = 0;
    struct eth_hdr e_hdr;
    struct ip_hdr* hdr = &pkt->hdr;
    hdr->ip_verlen = IP_VER_LEN;
    hdr->ip_tos = 0;
    hdr->ip_total_length = JHTONS(length + IP_HEADER_LEN);
    hdr->ip_id = JHTONS(packet_id);
    hdr->ip_flags_offset = 0;
    hdr->ip_ttl = IP_TTL;
    hdr->ip_header_checksum = 0;
    hdr->ip_header_checksum = ip_checksum((void*)pkt, IP_HEADER_LEN);
    packet_id++;
    e_hdr.eth_type = JHTONS(ETH_TYPE_IP);
    return eth_send(&e_hdr, (void*)pkt, sizeof(struct ip_hdr) + length);
}

int
ip_recv(struct ip_pkt* pkt) {
    if (trace_packet_processing) cprintf("Processing IP packet\n");
    struct ip_hdr* hdr = &pkt->hdr;
    if (hdr->ip_verlen != IP_VER_LEN) {
        return -E_UNS_VER;
    }
    uint16_t checksum = hdr->ip_header_checksum;
    hdr->ip_header_checksum = 0;
    if (checksum != ip_checksum((void*)pkt, IP_HEADER_LEN)) {
        return -E_INV_CHS;
    }
    if (hdr->ip_protocol == IP_PROTO_UDP) {
        return udp_recv(pkt);
    } else if (hdr->ip_protocol == IP_PROTO_TCP) {
        return tcp_recv(pkt);
    } else if (hdr->ip_protocol == IP_PROTO_ICMP) {
        return icmp_echo_reply(pkt);
    }

    return 0;
}
