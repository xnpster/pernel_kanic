#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <kern/inet.h>
#include <kern/udp.h>
#include <kern/traceopt.h>

int
udp_send(void* data, uint16_t length) {
    if (trace_packet_processing) cprintf("Sending UDP packet\n");
    if (length + UDP_HEADER_LEN > UDP_MAX_PACKET_SIZE)
        return -E_INVAL;
    struct udp_pkt pkt;
    struct udp_hdr* hdr = &pkt.hdr;
    hdr->source_port = JHTONS(8081);
    hdr->destination_port = JHTONS(1234);
    hdr->length = JHTONS(length + sizeof(struct udp_hdr));
    hdr->checksum = 0;
    memcpy((void*)pkt.data, data, length);
    struct ip_pkt result;
    result.hdr.ip_protocol = IP_PROTO_UDP;
    result.hdr.ip_source_address = JHTONL(MY_IP);
    result.hdr.ip_destination_address = JHTONL(HOST_IP);
    memcpy((void*)result.data, (void*)&pkt, length + sizeof(struct udp_hdr));
    return ip_send(&result, length + sizeof(struct udp_hdr));
}

int
udp_recv(struct ip_pkt* pkt) {
    if (trace_packet_processing) cprintf("Processing UDP packet\n");
    struct udp_pkt upkt;
    int size = JNTOHS(pkt->hdr.ip_total_length) - IP_HEADER_LEN;
    memcpy((void*)&upkt, (void*)pkt->data, size);
    struct udp_hdr* hdr = &upkt.hdr;
    cprintf("port: %d\n", JNTOHS(hdr->destination_port));
    for (size_t i = 0; i < JNTOHS(hdr->length) - UDP_HEADER_LEN; i++) {
        cprintf("%c", upkt.data[i]);
    }
    cprintf("\n");
    udp_send(upkt.data, JNTOHS(hdr->length) - UDP_HEADER_LEN);
    return 0;
}
