/* Basic string routines.  Not hardware optimized, but not shabby. */

#include <inc/string.h>

/* Using assembly for memset/memmove
 * makes some difference on real hardware,
 * but it makes an even bigger difference on bochs.
 * Primespipe runs 3x faster this way */

#define ASM 1

size_t
strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

size_t
strnlen(const char *s, size_t size) {
    size_t n = 0;
    while (n < size && *s++) n++;
    return n;
}

char *
strcpy(char *dst, const char *src) {
    char *res = dst;
    while ((*dst++ = *src++)) /* nothing */
        ;
    return res;
}

char *
strcat(char *dst, const char *src) {
    size_t len = strlen(dst);
    strcpy(dst + len, src);
    return dst;
}

char *
strncpy(char *dst, const char *src, size_t size) {
    char *ret = dst;
    while (size-- > 0) {
        *dst++ = *src;
        /* If strlen(src) < size, null-pad
         * 'dst' out to 'size' chars */
        if (*src) src++;
    }
    return ret;
}

size_t
strlcpy(char *dst, const char *src, size_t size) {
    char *dst_in = dst;
    if (size) {
        while (--size > 0 && *src)
            *dst++ = *src++;
        *dst = '\0';
    }
    return dst - dst_in;
}

size_t
strlcat(char *restrict dst, const char *restrict src, size_t maxlen) {
    const size_t srclen = strlen(src);
    const size_t dstlen = strnlen(dst, maxlen);

    if (dstlen == maxlen) return maxlen + srclen;

    if (srclen < maxlen - dstlen) {
        memcpy(dst + dstlen, src, srclen + 1);
    } else {
        memcpy(dst + dstlen, src, maxlen - 1);
        dst[dstlen + maxlen - 1] = '\0';
    }
    return dstlen + srclen;
}

int
strcmp(const char *p, const char *q) {
    while (*p && *p == *q) p++, q++;
    return (int)((unsigned char)*p - (unsigned char)*q);
}

int
strncmp(const char *p, const char *q, size_t n) {
    while (n && *p && *p == *q) n--, p++, q++;

    if (!n) return 0;

    return (int)((unsigned char)*p - (unsigned char)*q);
}

/* Return a pointer to the first occurrence of 'c' in 's',
 *  * or a null pointer if the string has no 'c' */
char *
strchr(const char *str, int c) {
    for (; *str; str++) {
        if (*str == c) {
            return (char *)str;
        }
    }
    return NULL;
}

/* Return a pointer to the first occurrence of 'c' in 's',
 *  * or a pointer to the string-ending null character if the string has no 'c' */
char *
strfind(const char *str, int ch) {
    for (; *str && *str != ch; str++) /* nothing */
        ;
    return (char *)str;
}


#if ASM
void *
memset(void *v, int c, size_t n) {
    uint8_t *ptr = v;
    ssize_t ni = n;

    if (__builtin_expect((ni -= ((8 - ((uintptr_t)v & 7))) & 7) < 0, 0)) {
        while (n-- > 0) *ptr++ = c;
        return v;
    }

    uint64_t k = 0x101010101010101ULL * (c & 0xFFU);

    if (__builtin_expect((uintptr_t)ptr & 7, 0)) {
        if ((uintptr_t)ptr & 1) *ptr = k, ptr += 1;
        if ((uintptr_t)ptr & 2) *(uint16_t *)ptr = k, ptr += 2;
        if ((uintptr_t)ptr & 4) *(uint32_t *)ptr = k, ptr += 4;
    }

    if (__builtin_expect(ni >> 3, 1)) {
        asm volatile("cld; rep stosq\n" ::"D"(ptr), "a"(k), "c"(ni >> 3)
                     : "cc", "memory");
        ni &= 7;
    }

    if (__builtin_expect(ni, 0)) {
        if (ni & 4) *(uint32_t *)ptr = k, ptr += 4;
        if (ni & 2) *(uint16_t *)ptr = k, ptr += 2;
        if (ni & 1) *ptr = k;
    }

    return v;
}

void *
memmove(void *dst, const void *src, size_t n) {
    const char *s = src;
    char *d = dst;

    if (s < d && s + n > d) {
        s += n;
        d += n;
        if (!(((intptr_t)s & 7) | ((intptr_t)d & 7) | (n & 7))) {
            asm volatile("std; rep movsq\n" ::"D"(d - 8), "S"(s - 8), "c"(n / 8)
                         : "cc", "memory");
        } else {
            asm volatile("std; rep movsb\n" ::"D"(d - 1), "S"(s - 1), "c"(n)
                         : "cc", "memory");
        }
        /* Some versions of GCC rely on DF being clear */
        asm volatile("cld" ::
                             : "cc");
    } else {
        if (!(((intptr_t)s & 7) | ((intptr_t)d & 7) | (n & 7))) {
            asm volatile("cld; rep movsq\n" ::"D"(d), "S"(s), "c"(n / 8)
                         : "cc", "memory");
        } else {
            asm volatile("cld; rep movsb\n" ::"D"(d), "S"(s), "c"(n)
                         : "cc", "memory");
        }
    }
    return dst;
}

#else

void *
memset(void *v, int c, size_t n) {
    char *ptr = v;

    while (n-- > 0) *ptr++ = c;

    return v;
}

void *
memmove(void *dst, const void *src, size_t n) {
    const char *s = src;
    char *d = dst;

    if (s < (char *)dst && s + n > (char *)dst) {
        s += n, d += n;
        while (n-- > 0) *--d = *--s;
    } else {
        while (n-- > 0) *d++ = *s++;
    }

    return dst;
}
#endif

void *
memcpy(void *dst, const void *src, size_t n) {
    return memmove(dst, src, n);
}

int
memcmp(const void *v1, const void *v2, size_t n) {
    const uint8_t *s1 = (const uint8_t *)v1;
    const uint8_t *s2 = (const uint8_t *)v2;

    while (n-- > 0) {
        if (*s1 != *s2) {
            return (int)*s1 - (int)*s2;
        }
        s1++, s2++;
    }

    return 0;
}

void *
memfind(const void *src, int c, size_t n) {
    const void *end = (const char *)src + n;
    for (; src < end; src++) {
        if (*(const unsigned char *)src == (unsigned char)c) break;
    }
    return (void *)src;
}

long
strtol(const char *s, char **endptr, int base) {
    /* Gobble initial whitespace */
    while (*s == ' ' || *s == '\t') s++;

    bool neg = *s == '-';

    /* Plus/minus sign */
    if (*s == '+' || *s == '-') s++;

    /* Hex or octal base prefix */
    if ((!base || base == 16) && (s[0] == '0' && s[1] == 'x')) {
        base = 16;
        s += 2;
    } else if (!base && s[0] == '0') {
        base = 8;
        s++;
    } else if (!base) {
        base = 10;
    }

    /* Digits */
    long val = 0;
    for (;;) {
        uint8_t dig = *s++;

        if (dig - '0' < 10)
            dig -= '0';
        else if (dig - 'a' < 27)
            dig -= 'a' - 10;
        else if (dig - 'A' < 27)
            dig -= 'A' - 10;
        else
            break;

        if (dig >= base) break;

        /* We don't properly detect overflow! */
        val = val * base + dig;
    }

    if (endptr) *endptr = (char *)s;

    return (neg ? -val : val);
}
