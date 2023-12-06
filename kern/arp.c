#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <kern/inet.h>
#include <kern/ethernet.h>
#include <kern/arp.h>
#include <kern/traceopt.h>

static struct arp_cache_table arp_table[ARP_TABLE_MAX_SIZE];

uint8_t *
get_mac_by_ip(uint32_t ip) {
    struct arp_cache_table *entry;
    for (int i = 0; i < ARP_TABLE_MAX_SIZE; i++) {
        entry = &arp_table[i];
        if (entry->source_ip == ip) {
            return entry->source_mac;
        }
    }
    return NULL;
}

void
initialize_arp_table() {
    struct arp_cache_table *entry;
    entry = &arp_table[ARP_TABLE_MAX_SIZE - 1]; // default MAC
    entry->source_ip = JHTONL(HOST_IP);
    uint8_t mac[6] = {0xca, 0xfe, 0x33, 0x53, 0x82, 0x87};
    memcpy(entry->source_mac, mac, 6);
    entry->state = STATIC_STATE;
    for (int i = 1; i < ARP_TABLE_MAX_SIZE; i++) {
        entry = &arp_table[i];
        entry->state = FREE_STATE;
    }
}

int
update_arp_table(struct arp_hdr *arp_header) {
    struct arp_cache_table *entry;
    int i;
    for (i = 0; i < ARP_TABLE_MAX_SIZE; i++) {
        entry = &arp_table[i];
        if (entry->state == FREE_STATE) {
            entry->source_ip = arp_header->source_ip;
            memcpy(entry->source_mac, arp_header->source_mac, 6);
            entry->state = DYNAMIC_STATE;
            return 0;
        }

        if (entry->source_ip == arp_header->source_ip) {
            if (entry->state == DYNAMIC_STATE) {
                memcpy(entry->source_mac, arp_header->source_mac, 6);
            }
            break;
        }
    }

    if (i == ARP_TABLE_MAX_SIZE) {
        return -1;
    }
    return 0;
}

int
arp_reply(struct arp_hdr *arp_header) {
    if (trace_packet_processing) cprintf("Sending ARP reply\n");
    arp_header->opcode = ARP_REPLY;
    memcpy(arp_header->target_mac, arp_header->source_mac, 6);
    arp_header->target_ip = arp_header->source_ip;
    memcpy(arp_header->source_mac, get_my_mac(), 6);
    arp_header->source_ip = JHTONL(MY_IP);

    arp_header->opcode = JHTONS(arp_header->opcode);
    arp_header->hardware_type = JHTONS(arp_header->hardware_type);
    arp_header->protocol_type = JHTONS(arp_header->protocol_type);

    struct eth_hdr reply_header;
    memcpy(reply_header.eth_destination_mac, arp_header->target_mac, 6);
    reply_header.eth_type = JHTONS(ETH_TYPE_ARP);
    memcpy(reply_header.eth_destination_mac, get_mac_by_ip(arp_header->target_ip), 6);
    int status = eth_send(&reply_header, arp_header, sizeof(struct arp_hdr));
    if (status < 0) {
        cprintf("Error attempting ARP response.");
        return -1;
    }
    return 0;
}

int
arp_resolve(void* data) {
    if (trace_packet_processing) cprintf("Resolving ARP\n");
    struct arp_hdr *arp_header;

    arp_header = (struct arp_hdr *)data;

    arp_header->hardware_type = JNTOHS(arp_header->hardware_type);
    arp_header->protocol_type = JNTOHS(arp_header->protocol_type);
    arp_header->opcode = JNTOHS(arp_header->opcode);
    arp_header->target_ip = JNTOHL(arp_header->target_ip);

    if (arp_header->hardware_type != ARP_ETHERNET) {
        cprintf("Error! Only ethernet is supporting.");
        return -1;
    }
    if (arp_header->protocol_type != ARP_IPV4) {
        cprintf("Error! Only IPv4 is supported.");
        return -1;
    }

    int status = update_arp_table(arp_header);
    if (status < 0) {
        cprintf("ARP table is filled in");
    }
    if (arp_header->target_ip != MY_IP) {
        cprintf("This is not for me!");
        return -1;
    }
    if (arp_header->opcode != ARP_REQUEST) {
        cprintf("Error! Only arp requests are supported");
        return -1;
    }

    return arp_reply(arp_header);
}
