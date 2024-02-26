#include <data_structure/kernel_atomic.h>
void test_atomic()
{
    atomic_flag lock = ATOMIC_FLAG_INIT;
  // ...
  while (atomic_flag_test_and_set(&lock)) {
    // 等待锁被释放
  }
  // 临界区代码
  atomic_flag_clear(&lock);
}