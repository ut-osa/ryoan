#ifndef _TRUSTED_SERVICE_RUNTIME_USEC_WAIT_H
#define _TRUSTED_SERVICE_RUNTIME_USEC_WAIT_H

/* determined empirically/manually */
#define USEC_ITERATIONS 127UL

/* measured on harware */
#define EENTER_DELAY 3.9
#define ERESUME_DELAY 3.14
#define EMODPE_DELAY (EENTER_DELAY / 10)

size_t usec_wait(double usecs);

static inline void eenter_wait(void)
{
    usec_wait(EENTER_DELAY);
}

static inline void eresume_wait(void)
{
    usec_wait(ERESUME_DELAY);
}

static inline void emodpe_wait(void)
{
    usec_wait(EMODPE_DELAY);
}
#endif
