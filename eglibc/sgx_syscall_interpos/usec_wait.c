#include <unistd.h>
#include <stdatomic.h>
#include <fenv.h>

#include "usec_wait.h"

#pragma GCC optimize ("-O0")

size_t usec_wait(double usecs) {
   /*
    size_t i = USEC_ITERATIONS * usecs;
    atomic_ulong  j = 0;
    int tmp = 0;

    while (atomic_fetch_add(&j, 1) < i)
        asm("");
    return j - 1;
    */
}
