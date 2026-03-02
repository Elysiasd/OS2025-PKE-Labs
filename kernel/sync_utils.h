#ifndef _SYNC_UTILS_H_
#define _SYNC_UTILS_H_

// barrier: wait until all NCPU cores reach this point.
static inline void sync_barrier(volatile int *counter, int all) {
  int local;
  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(1)
               : "memory");
  if (local + 1 < all) {
    do {
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    } while (local < all);
  }
}

// spinlock: mutual exclusion using amoswap. added @lab2_challenge3
typedef struct {
  volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT {.locked = 0}

static inline void spinlock_lock(spinlock_t *lk) {
  int tmp = 1;
  do {
    asm volatile("amoswap.w.aq %0, %0, (%1)\n"
                 : "+r"(tmp)
                 : "r"(&lk->locked)
                 : "memory");
  } while (tmp != 0);
}

static inline void spinlock_unlock(spinlock_t *lk) {
  asm volatile("amoswap.w.rl zero, zero, (%0)\n"
               :
               : "r"(&lk->locked)
               : "memory");
}

#endif