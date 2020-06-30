#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_CACHE_MONITOR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_CACHE_MONITOR_H_ 1

#include <stdio.h> /* for FILE */

int cache_monitor_init(void);
void cache_monitor_fini(void);
int cache_monitor_enable(void);
int cache_monitor_disable(void);
void cache_monitor_report(FILE *);

#endif
