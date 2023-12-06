#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <kern/inet.h>
#include <kern/e1000.h>
#include <kern/ethernet.h>
#include <kern/arp.h>
#include <kern/ip.h>
#include <kern/traceopt.h>

//52:54:00:12:34:56
static const uint8_t qemu_mac[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};

const uint8_t *
get_my_mac(void) {
    return qemu_mac;
}

int
eth_send(struct eth_hdr* hdr, void* data, size_t len) {
    if (trace_packet_processing) cprintf("Sending Ethernet packet\n");
    char buf[ETH_MAX_PACKET_SIZE + 1];
    assert(len <= ETH_MAX_PACKET_SIZE - sizeof(struct eth_hdr));
    memcpy((void*)hdr->eth_source_mac, get_my_mac(), sizeof(hdr->eth_source_mac));
    if (hdr->eth_type == JHTONS(ETH_TYPE_IP)) {
        struct ip_hdr* ip_header = &((struct ip_pkt*)data)->hdr;
        memcpy(hdr->eth_destination_mac, get_mac_by_ip(ip_header->ip_destination_address), 6);
    }
    hdr->eth_type = htons(hdr->eth_type);
    memcpy((void*)buf, (void*)hdr, sizeof(struct eth_hdr));
    memcpy((void*)buf + sizeof(struct eth_hdr), data, len);
    return e1000_transmit(buf, len + sizeof(struct eth_hdr));
}


int
eth_recv(void* data) {
    if (trace_packet_processing) cprintf("Processing Ethernet packet\n");
    char buf[ETH_MAX_PACKET_SIZE + 1];
    struct eth_hdr hdr = {};
    int size = e1000_receive(buf);
    if (size <= 0) return size;
    memcpy((void*)&hdr, (void*)buf, sizeof(struct eth_hdr));
    hdr.eth_type = JNTOHS(hdr.eth_type);
    memcpy(data, (void*)buf + sizeof(struct eth_hdr), size);
    if (hdr.eth_type == ETH_TYPE_IP) {
        if (ip_recv(data) < 0) return -1;
    } else 
    if (hdr.eth_type == ETH_TYPE_ARP) {
        if (arp_resolve(data) < 0) return -1;
    } else {
        return -E_BAD_ETH_TYPE;
    }

    return size;
}
