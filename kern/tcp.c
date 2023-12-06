#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <kern/inet.h>
#include <kern/tcp.h>
#include <kern/traceopt.h>


const char gifs[][150] = {
    "https://media.tenor.com/4gPD1ccxrVgAAAAC/rick-ashley-dance.gif",
    "https://media.tenor.com/UhTgwMEMDlEAAAAM/frog.gif",
    "https://media.tenor.com/CYvlGxL0i2IAAAAM/sepples-cpp.gif",
    "https://media.tenor.com/NYrgLNGuy7YAAAAM/the-c-programming-language-uncle-dane.gif",
    "https://media.tenor.com/CtdmjqENwtcAAAAM/python-programming.gif",
    "https://media.tenor.com/CZZJVKwzRCMAAAAM/dev-developer.gif",
    "https://media.tenor.com/qNZPYVdPUkoAAAAM/linux-users.gif",
    "https://media.tenor.com/MstC_FIUp6QAAAAM/noobgrammer.gif",
    "https://media.tenor.com/eQJns8923ioAAAAM/hacker-pc.gif",
    "https://media.tenor.com/9HyK4J7t7woAAAAM/coding.gif",
    "https://media.tenor.com/hWVqJl31yA8AAAAM/web-webdevelopper.gif",
    "https://media.tenor.com/Ea1tSJ9vk6EAAAAM/off-from-work-ready.gif",
    "https://media.tenor.com/2iAZpFihOGYAAAAM/loading-error.gif"
};

const char page_top[] = 
"HTTP/1.1 200 OK\n\n \
    <HTML><HEAD><TITLE>title</TITLE></HEAD> \
<BODY> \
    <h1><a href=\"http://192.160.144.2:80\">next</a></h1> \
        <img src=\"";
        
const char page_bot[] = "\" alt=\"funny GIF\" width=\"30%\"></BODY></HTML>\n";

#define DATASIZE ((sizeof(page_top) + sizeof(page_bot))*10)

#define CONN_MAX 5
#define MAX_TCP_PAYLOAD 1000

enum conn_status {
    NOT_CONN,
    WAIT_ACK_OPEN,
    WAIT_DATA,
    WAIT_DATA_ACK,
    WAIT_FIN_ACK,

    WAIT_ACK_CLOSE
};

struct tcp_connection {
    int status;
    uint32_t src_addr;

    uint16_t source_port;
    uint16_t dest_port;

    uint32_t seq;
    uint32_t seq_ack;

    uint16_t window;

    uint8_t rec_data[DATASIZE];
    uint8_t send_data[DATASIZE];

    int rec_data_off;
    int send_data_off, send_data_size;
} connections[CONN_MAX];

struct tcp_connection* alloc_one() {
    for(int i = 0; i < CONN_MAX; i++) {
        if(connections[i].status == NOT_CONN) {
            return &(connections[i]);
        }
    }

    return NULL;
}

struct tcp_connection* find_connection(uint32_t src_addr, uint16_t source_port, uint16_t destination_port) {
    for(int i = 0; i < CONN_MAX; i++) {
        if(connections[i].status != NOT_CONN && 
            connections[i].source_port == source_port &&
            connections[i].dest_port == destination_port &&
            connections[i].src_addr == src_addr) {

            return &(connections[i]);
        }
    }

    return NULL;

}

//void test_chksum();
void setup_tcp() {
    for(int i = 0; i < CONN_MAX; i++) {
        connections[i].status = NOT_CONN;
    }
    //test_chksum();
}

int calc_payload_size(int ip_len) {
    return (ip_len - sizeof(struct ip_hdr) - sizeof(struct tcp_hdr));
}

static void ip2pseudoip(struct tcp_pseudo_hdr* pseudo, struct ip_hdr* pkt) {
    pseudo->src_addr = pkt->ip_source_address;
    pseudo->dst_addr = pkt->ip_destination_address;

    pseudo->reserved = 0;
    pseudo->proto = pkt->ip_protocol;
    pseudo->tcp_len = JHTONS(JNTOHS(pkt->ip_total_length) - sizeof(struct ip_hdr));
}

static void print_pkt(struct tcp_pkt* tpkt, int payload_size) {
    cprintf("=========TCP PACKET=========\n");

    cprintf("SRC: %d, DEST: %d\n", JNTOHS(tpkt->hdr.source_port), JNTOHS(tpkt->hdr.destination_port));
    cprintf("SEQ: %lu, ACK_SEQ: %lu\n", JNTOHL(tpkt->hdr.seq), JNTOHL(tpkt->hdr.ack_seq));
    cprintf("DOFF: %d, payload: %d\n", DATA_OFFSET(tpkt->hdr.data_offset), payload_size);
    if(tpkt->hdr.flags & (1 << TCP_ACK)) cprintf("A ");
    if(tpkt->hdr.flags & (1 << TCP_SYN)) cprintf("S ");
    if(tpkt->hdr.flags & (1 << TCP_FIN)) cprintf("F ");

    cprintf("\n");
    cprintf("FLAGS: 0x%x\n", tpkt->hdr.flags);
    /*
    cprintf("============================\n");
    
    uint8_t* d = (uint8_t*)((void*)tpkt);
    for(int i = 0; i < sizeof(*tpkt); i++) {
        if(i % 16 == 0) cprintf("\n");
        cprintf("%02x ", d[i]);
    }

    cprintf("\n");
    */
    cprintf("============================\n");
}

uint16_t
tcp_checksum(void* vdata, size_t length, void* pseudo) {
    char* data = vdata;
    uint32_t acc = 0xffff;

    char* psd_data = pseudo;
    for (size_t i = 0; i < sizeof(struct tcp_pseudo_hdr); i += 2) {
        uint16_t word;
        memcpy(&word, psd_data + i, 2);
        acc += JNTOHS(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }

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
    return JHTONS(~acc) & 0xffff;
}

static int
tcp_send(struct tcp_connection* connection, const void* data, uint16_t length, 
         bool syn, bool ack, bool fin, bool psh) {
    if (trace_packet_processing) cprintf("Sending TCP packet\n");
    
    struct tcp_pkt pkt;
    struct tcp_hdr* hdr = &pkt.hdr;
    memset(&pkt, 0, sizeof(pkt));
    //print_pkt(&pkt, length + sizeof(struct tcp_hdr));
    hdr->source_port = JHTONS(connection->dest_port);
    hdr->destination_port = JHTONS(connection->source_port);


    hdr->data_offset = (sizeof(struct tcp_hdr)/4) << 4;
    uint8_t flags = 0;

    if(syn) {
        flags |= (1 << TCP_SYN);
    }

    if(ack) {
        flags |= (1 << TCP_ACK);
    }

    if(fin) {
        flags |= (1 << TCP_FIN);
    }

    if(psh) {
        flags |= (1 << TCP_PSH);
    }

    hdr->flags = flags;
    

    hdr->seq = JHTONL(connection->seq);
    hdr->ack_seq = JHTONL(connection->seq_ack);

    memcpy((void*)pkt.data, data, length);
    //print_pkt(&pkt, length + sizeof(struct tcp_hdr));
    
    hdr->checksum = 0;
    hdr->window = JHTONS(connection->window);

    struct ip_pkt result;
    result.hdr.ip_protocol = IP_PROTO_TCP;
    result.hdr.ip_source_address = JHTONL(MY_IP);
    result.hdr.ip_destination_address = JHTONL(HOST_IP);
    result.hdr.ip_total_length = JHTONS(IP_HEADER_LEN + sizeof(struct tcp_hdr) + length);

    struct tcp_pseudo_hdr pseudo;
    ip2pseudoip(&pseudo, &(result.hdr));

    hdr->checksum = tcp_checksum((void*)&pkt, length + sizeof(struct tcp_hdr), &pseudo);

    memcpy((void*)result.data, (void*)&pkt, length + sizeof(struct tcp_hdr));

    print_pkt(&pkt, length + sizeof(struct tcp_hdr));
    return ip_send(&result, length + sizeof(struct tcp_hdr));
}

static int gif_idx = 0;
static int initialize_conn(struct tcp_pkt* pkt, struct tcp_connection* connection, uint32_t src_addr) {
    gif_idx = (gif_idx + 1) % (sizeof(gifs)/sizeof(*gifs));

    connection->src_addr = src_addr;
    connection->status = WAIT_ACK_OPEN;

    connection->source_port = JNTOHS(pkt->hdr.source_port);
    connection->dest_port = JNTOHS(pkt->hdr.destination_port);

    connection->seq_ack = JNTOHL(pkt->hdr.seq) + 1;
    connection->seq = 1000;

    memset(connection->rec_data, 0, sizeof(connection->rec_data));
    connection->rec_data_off = 0;

    const char* gif_ref = gifs[gif_idx];
    
    uint8_t* dest = connection->send_data;
    memcpy((void*)dest, page_top, sizeof(page_top) - 1);
    dest = dest + sizeof(page_top) - 1;

    int ref_len = strlen(gif_ref);

    memcpy((void*)dest, gif_ref, ref_len);
    dest = dest + ref_len;

    strcpy((void*)dest, page_bot);
    dest = dest + sizeof(page_bot);

    connection->send_data_size = sizeof(page_top) - 1 + sizeof(page_bot) + ref_len;
    connection->send_data_off = 0;

    connection->window = JNTOHS(pkt->hdr.window);
    
    return 0;
}

static int tcp_handle(struct tcp_pkt* pkt, int payload_size, struct tcp_pseudo_hdr* pseudo) {
        uint8_t flags = pkt->hdr.flags;

        struct tcp_connection* connection = 
            find_connection(JNTOHL(pseudo->src_addr), 
            JNTOHS(pkt->hdr.source_port), JNTOHS(pkt->hdr.destination_port));

        if(connection == NULL) {
            connection = alloc_one();

            if(connection == NULL) {
                return -1;
            }
        }

        switch (connection->status)
        {
        case NOT_CONN: {
            if(flags & (1 << TCP_SYN)) {
                initialize_conn(pkt, connection, JNTOHL(pseudo->src_addr));

                uint32_t dummy;

                tcp_send(connection, &dummy, 0, 1, 1, 0, 0);
            } else {
                return -1;
            }
            
            break;
        };
        case WAIT_ACK_OPEN: {
            if(flags & (1 << TCP_ACK)) {
                connection->seq++;
                connection->status = WAIT_DATA;
            } else {
                return -1;
            }            
        };
        break;

        case WAIT_DATA: {
            if(connection->seq_ack == JNTOHL(pkt->hdr.seq)) {
                if(flags & (1 << TCP_FIN)) {
                    connection->seq_ack++;
                    //connection.seq++;
                    uint32_t dummy;
                    tcp_send(connection, &dummy, 0, 0, 1, 1, 0);

                    connection->status = WAIT_ACK_CLOSE;
                } else if(flags & (1 << TCP_PSH)) {
                    uint8_t* data = (uint8_t*)(((uint32_t*) pkt) + DATA_OFFSET(pkt->hdr.data_offset));
                    
                    int data_size = payload_size + sizeof(struct tcp_hdr) - DATA_OFFSET(pkt->hdr.data_offset)*4;
                    cprintf("==========rec data=============\n");
                    for (size_t i = 0; i < payload_size; i++) {
                        cprintf("%c", data[i]);
                        connection->rec_data[connection->rec_data_off++] = data[i];
                    }
                    cprintf("=======================\n");
                    
                    connection->seq_ack += data_size;
                    uint32_t dummy;

                    tcp_send(connection, &dummy, 0, 0, 1, 0, 0);
                    if(!strncmp((char*)data, "GET", 3)) {
                        cprintf("get method\n");

                        int data_remain = connection->send_data_size - connection->send_data_off;
                        int send_data = data_remain < MAX_TCP_PAYLOAD ? data_remain : MAX_TCP_PAYLOAD;

                        tcp_send(connection, connection->send_data + connection->send_data_off, send_data, 0, 1, 0, 1);
                        connection->seq += send_data;
                        connection->send_data_off += send_data;

                        connection->status = WAIT_DATA_ACK;
                    } else {
                        const char s[] = "Oops! It's not valid http request!:)\n";
                        tcp_send(connection, s, sizeof(s), 0, 1, 0, 1);
                        connection->send_data_off = connection->send_data_size;
                        connection->seq += sizeof(s);
                        connection->status = WAIT_DATA_ACK;
                    }
                }
            } else if((connection->seq_ack == JNTOHL(pkt->hdr.seq) + 1) && !(flags & (1 << TCP_ACK))) {
                    //keep-alive pocket
                    cprintf("Keep-alive packet\n");
                    uint32_t dummy;
                    tcp_send(connection, &dummy, 0, 0, 1, 0, 0);
            } else {
                return -1;
            }

            break;
        };

        case WAIT_DATA_ACK: {
            if(connection->seq == JNTOHL(pkt->hdr.ack_seq) && (flags & (1 << TCP_ACK))) {
                //close connection
                if(connection->send_data_off < connection->send_data_size) {
                    int data_remain = connection->send_data_size - connection->send_data_off;
                    int send_data = data_remain < MAX_TCP_PAYLOAD ? data_remain : MAX_TCP_PAYLOAD;

                    tcp_send(connection, connection->send_data + connection->send_data_off, send_data, 0, 1, 0, 1);
                    connection->seq += send_data;
                    connection->send_data_off += send_data;

                    connection->status = WAIT_DATA_ACK;
                } else {
                    uint32_t dummy;
                    tcp_send(connection, &dummy, 0, 0, 1, 1, 0);
                    connection->status = WAIT_FIN_ACK;
                }
            }

            break;
        }

        case WAIT_FIN_ACK: {
            if(connection->seq + 1 == JNTOHL(pkt->hdr.ack_seq) && (flags & (1 << TCP_ACK)) && (flags & (1 << TCP_FIN))) {
                //close connection
                connection->seq++;
                connection->seq_ack++;

                uint32_t dummy;
                tcp_send(connection, &dummy, 0, 0, 1, 0, 0);
                connection->status = NOT_CONN;
            }

            break;
        }
    
        case WAIT_ACK_CLOSE: {
            cprintf("close sec %d + 1 == %ld\n", connection->seq, JNTOHL(pkt->hdr.ack_seq));
            if(connection->seq + 1 == JNTOHL(pkt->hdr.ack_seq) && (flags & (1 << TCP_ACK))) {
                connection->status = NOT_CONN;
            } else {
                return -1;
            }

            break;
        };

        default:
            break;
        }

        return 0;
}

void test_chksum() {

    struct ip_pkt pkt;
    
    pkt.hdr.ip_source_address = JHTONL(IP(192, 168, 174, 128));
    pkt.hdr.ip_destination_address = JHTONL(IP(192, 168, 174, 1));

    pkt.hdr.ip_protocol = IP_PROTO_TCP;
    pkt.hdr.ip_total_length = JHTONS(58);

    struct tcp_pseudo_hdr pseudo;
    ip2pseudoip(&pseudo, &(pkt.hdr));

    struct tcp_pkt tpkt;
    memset(&tpkt, 0, sizeof(tpkt));

    tpkt.hdr.source_port = JHTONS(4444);
    tpkt.hdr.destination_port = JHTONS(56506);
    tpkt.hdr.seq = JHTONL(685064666);
    tpkt.hdr.ack_seq = JHTONL(1692953104);
    tpkt.hdr.data_offset = 8 << 4;
    tpkt.hdr.flags = 0x18;
    tpkt.hdr.window = JHTONS(510);
    tpkt.hdr.checksum = JHTONS(0);
    tpkt.hdr.urgent_p = JHTONS(0);

    uint8_t* data = (tpkt.data);
    data[0] = 0x01;
    data[1] = 0x01;

    data[2] = 0x08;
    data[3] = 0x0a;
    data[4] = 0x5c;
    data[5] = 0x86;

    data[6] = 0xc6;
    data[7] = 0xf8;
    data[8] = 0xbd;
    data[9] = 0x62;

    data[10] = 0xe3;
    data[11] = 0x6f;
    data[12] = 0x6c;
    data[13] = 0x69;

    data[14] = 0x64;
    data[15] = 0x6f;
    data[16] = 0x72;
    data[17] = 0x0a;

    int payload_size = calc_payload_size(JNTOHS(pkt.hdr.ip_total_length));

    print_pkt(&tpkt, payload_size);
    cprintf("checksum: 0x%x\n", JNTOHS(tpkt.hdr.checksum));
    tpkt.hdr.checksum = 0;
    print_pkt(&tpkt, payload_size);
    cprintf("calculated: 0x%x\n", tcp_checksum(&tpkt, payload_size + sizeof(tpkt.hdr), &pseudo));
}



int
tcp_recv(struct ip_pkt* pkt) {
    if (trace_packet_processing) cprintf("Processing TCP packet\n");
    struct tcp_pkt tpkt;
    int size = JNTOHS(pkt->hdr.ip_total_length) - IP_HEADER_LEN;
    memcpy((void*)&tpkt, (void*)pkt->data, size);
    struct tcp_hdr* hdr = &tpkt.hdr;

    cprintf("port: %d\n", JNTOHS(hdr->destination_port));



    int payload_size = calc_payload_size(JNTOHS(pkt->hdr.ip_total_length));
    struct tcp_pseudo_hdr pseudo;
    ip2pseudoip(&pseudo, &(pkt->hdr));

    //print_pkt(&tpkt, payload_size);
    tcp_handle(&tpkt, payload_size, &pseudo);

    return 0;
}

