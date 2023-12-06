#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
    int sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct hostent *me = gethostbyname("192.160.144.1");
    struct sockaddr_in my_addr = {
        .sin_family = AF_INET,
        .sin_addr = *(struct in_addr *)me->h_addr_list[0],
        .sin_port = htons(1234)
    };
    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));

    struct hostent *jos = gethostbyname("192.160.144.2");
    struct sockaddr_in jos_addr = {
        .sin_family = AF_INET,
        .sin_addr = *(struct in_addr *)jos->h_addr_list[0],
        .sin_port = htons(8888)
    };

    char buf[] = "HELLO";
    sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&jos_addr, sizeof(jos_addr));

    char reply[1024] = {};
    recvfrom(sockfd, reply, sizeof(reply), 0, NULL, NULL);
    printf("%s", reply);

    close(sockfd);
    return 0;
}
