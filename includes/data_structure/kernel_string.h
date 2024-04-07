#ifndef __KERNEL_STRING_H
#define __KERNEL_STRING_H 1

#include <stdint.h>

static inline char *strchr(const char *buf, const char *sub)
{
    if (NULL == buf)
        return NULL;
    const char *p = buf;
    while ('\0' != *p)
    {
        if (*p == *sub)
        {
            return (char *)p;
        }
        ++p;
    }
}

static inline char *strstr(const char *buf, const char *sub)
{
    register char *bp;
    register char *sp;

    if (*sub=='\0')
        return 0;
    while (*buf)
    {
        bp = buf;
        sp = sub;
        do {
            if (*sp=='\0')
                return buf;
        } while (*(bp++) == *(sp++));
        buf += 1;
    }
    return 0;
}

static inline char **strsplit(char *str, const char *delim)
{
    char *pos = str;
    unsigned int sub_n = 0;
    char **ret = valloc(sizeof(char *)*16);

    ret[sub_n] = str;
    while(pos = strchr(pos, delim))
    {
        *pos = '\0';
        pos = pos + 1;
        if (sub_n==16){return ret;}
        if (*pos == '\0') {
            ret[sub_n] = NULL;
        }
        else {
            ret[sub_n] = pos;
        }
        ++sub_n;
    }

    return ret;
}

static inline int strncmp(const char *s1, const char *s2, uint32_t n)
{
    if (n==0) { return -1; }
    uint32_t i = 0;
    for (i = 0; i < n; ++i) {
        if (*(s1+i) != *(s2+i))
        {
            break;
        }
    }
    if (i==n) { return 0; }
    else { return -1; }
}

// length includes '\0'
static inline uint32_t strlen(const char *s)
{
    uint32_t n = 1;
    while (*s != '\0')
    {
        ++n;
        ++s;
    }
    return n;
}

#endif