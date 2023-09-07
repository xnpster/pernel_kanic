#ifndef JOS_INC_STRING_H
#define JOS_INC_STRING_H

#include <inc/types.h>

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t size);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t size);
char *strcat(char *dst, const char *src);
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *restrict dst, const char *restrict src, size_t maxlen);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t size);
char *strchr(const char *s, int c);
char *strfind(const char *s, int c);

void *memset(void *dst, int c, size_t len);
void *memcpy(void *restrict dst, const void *restrict src, size_t len);
void *memmove(void *dst, const void *src, size_t len);
int memcmp(const void *s1, const void *s2, size_t len);
void *memfind(const void *s, int c, size_t len);

long strtol(const char *s, char **endptr, int base);

#endif /* not JOS_INC_STRING_H */
