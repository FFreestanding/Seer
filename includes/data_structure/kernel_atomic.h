#ifndef __KERNEL_MUTEX_H
#define __KERNEL_MUTEX_H 1

#include <stdatomic.h>

typedef struct mutex_t {
    _Atomic unsigned int counter;
} mutex_t;

static inline void mutex_init(mutex_t *mutex) {
    mutex->counter = ATOMIC_VAR_INIT(1);
}

static inline unsigned int mutex_on_hold(mutex_t *mutex) {
    return !atomic_load(&mutex->counter);
}

static inline void mutex_lock(mutex_t *mutex) {
    while (!atomic_load(&mutex->counter));
    atomic_fetch_sub(&mutex->counter, 1);
}

static inline void mutex_unlock(mutex_t *mutex) {
    atomic_fetch_add(&mutex->counter, 1);
}


#endif
