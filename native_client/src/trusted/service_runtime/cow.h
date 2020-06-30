#ifndef _NACL_COW_H_
#define _NACL_COW_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct NaClApp;
struct CowState;

void start_cow(struct NaClApp *nap);
void measure_size(struct NaClApp *nap);
void note_shm_size(size_t pgs);
void start_clock(void);
bool try_cow_recover(struct NaClApp *nap, uintptr_t addr);


void stop_mode_clock_start(void);
void stop_mode_clock_stop(unsigned long mode);
#endif
