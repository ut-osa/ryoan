#ifndef _TRUSTED_SERVICE_RUNTIME_USEC_WAIT_H
#define _TRUSTED_SERVICE_RUNTIME_USEC_WAIT_H

#include "native_client/src/trusted/service_runtime/sgx_profile.h"

/* determined empirically/manually */
#define USEC_ITERATIONS 127UL

size_t usec_wait(double usecs);

static inline void eenter_wait(void)
{
    usec_wait(active_sgx_profile.eenter_usecs);
}

static inline void eresume_wait(void)
{
    usec_wait(active_sgx_profile.eresume_usecs);
}

static inline void emodpe_wait(void)
{
    usec_wait(active_sgx_profile.emodpe_usecs);
}
#endif
