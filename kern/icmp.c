#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <kern/inet.h>
#include <kern/ethernet.h>
#include <kern/icmp.h>
#include <kern/ip.h>
#include <kern/traceopt.h>

int
icmp_echo_reply(struct ip_pkt* pkt) {
    if (trace_packet_processing) cprintf("Processing ICMP packet\n");
    struct icmp_pkt icmp_packet;
    int size = JNTOHS(pkt->hdr.ip_total_length) - IP_HEADER_LEN;
    memcpy((void*)&icmp_packet, (void*)pkt->data, size);
    struct icmp_hdr* hdr = &icmp_packet.hdr;
    if (hdr->msg_type != ECHO_REQUEST)
        return -E_UNS_ICMP_TYPE;
    if (hdr->msg_code != 0)
        return -E_INV_ICMP_CODE;

    hdr->msg_type = ECHO_REPLY;
    hdr->checksum = JNTOHS(hdr->checksum) + 0x0800;
    hdr->checksum = JHTONS(hdr->checksum);

    struct ip_pkt result;
    result.hdr.ip_protocol = IP_PROTO_ICMP;
    result.hdr.ip_source_address = JHTONL(MY_IP);
    result.hdr.ip_destination_address = JHTONL(HOST_IP);
    memcpy((void*)result.data, (void*)&icmp_packet, size);
    return ip_send(&result, size);
}
