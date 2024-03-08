#ifndef _COMMON_H_
#define _COMMON_H_

// 除法向上取整
#define CEIL(v, k)          (((v) + (1 << (k)) - 1) >> (k))

#define ICEIL(x, y) ((x) / (y) + ((x) % (y) != 0))

#endif // _COMMON_H_