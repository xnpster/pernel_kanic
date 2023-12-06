#ifndef INET_H
#define INET_H


#if BYTE_ORDER == BIG_ENDIAN

#define HTONS(n) (n)
#define NTOHS(n) (n)
#define HTONL(n) (n)
#define NTOHL(n) (n)

#else

#define HTONS(n) (((((unsigned short)(n)&0xFF)) << 8) | (((unsigned short)(n)&0xFF00) >> 8))
#define NTOHS(n) (((((unsigned short)(n)&0xFF)) << 8) | (((unsigned short)(n)&0xFF00) >> 8))

#define HTONL(n) (((((unsigned long)(n)&0xFF)) << 24) |    \
                  ((((unsigned long)(n)&0xFF00)) << 8) |   \
                  ((((unsigned long)(n)&0xFF0000)) >> 8) | \
                  ((((unsigned long)(n)&0xFF000000)) >> 24))

#define NTOHL(n) (((((unsigned long)(n)&0xFF)) << 24) |    \
                  ((((unsigned long)(n)&0xFF00)) << 8) |   \
                  ((((unsigned long)(n)&0xFF0000)) >> 8) | \
                  ((((unsigned long)(n)&0xFF000000)) >> 24))
#endif

#if 1

#define JHTONS(n) (((((unsigned short)(n)&0xFF)) << 8) | (((unsigned short)(n)&0xFF00) >> 8))
#define JNTOHS(n) (((((unsigned short)(n)&0xFF)) << 8) | (((unsigned short)(n)&0xFF00) >> 8))

#define JHTONL(n) (((((unsigned long)(n)&0xFF)) << 24) |    \
                  ((((unsigned long)(n)&0xFF00)) << 8) |   \
                  ((((unsigned long)(n)&0xFF0000)) >> 8) | \
                  ((((unsigned long)(n)&0xFF000000)) >> 24))

#define JNTOHL(n) (((((unsigned long)(n)&0xFF)) << 24) |    \
                  ((((unsigned long)(n)&0xFF00)) << 8) |   \
                  ((((unsigned long)(n)&0xFF0000)) >> 8) | \
                  ((((unsigned long)(n)&0xFF000000)) >> 24))

unsigned short htons(unsigned short n);
unsigned short ntohs(unsigned short n);
unsigned long htonl(unsigned long n);
unsigned long ntohl(unsigned long n);

#define htons(n) HTONS(n)
#define ntohs(n) NTOHS(n)

#define htonl(n) HTONL(n)
#define ntohl(n) NTOHL(n)

#endif

#endif
