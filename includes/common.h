#ifndef _COMMON_H_
#define _COMMON_H_

// 除法向上取整
#define CEIL(v, k)          (((v) + (1 << (k)) - 1) >> (k))

#define ICEIL(x, y) ((x) / (y) + ((x) % (y) != 0))

// 获取v最近的最大k倍数 (k=2^n)
#define ROUNDUP(v, k) (((v) + (k)-1) & ~((k)-1))

#define ASSERT(condition, msg) if((condition)==0){kernel_log(ERROR, msg); while(1);}

#endif // _COMMON_H_