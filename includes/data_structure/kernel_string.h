#ifndef __KERNEL_STRING_H
#define __KERNEL_STRING_H 1

#include <stdint.h>

static inline char *strstr(const char *buf, const char *sub)
{
    register char *bp;
    register char *sp;

    if (!*sub)
        return buf;
    while (*buf)
    {
        bp = buf;
        sp = sub;
        do {
            if (!*sp)
                return buf;
        } while (*bp++ == *sp++);
        buf += 1;
    }
    return 0;
}

#endif