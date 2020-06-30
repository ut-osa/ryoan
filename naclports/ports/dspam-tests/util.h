#ifndef __UTIL_H__
#define __UTIL_H__

#include <dspam/libdspam.h>

#define GENERIC_FAILURE -1
#define GENERIC_SUCCESS  0

char* read_message(void);

char* read_message_from_fd(int fd);

int check_res(int res, const char* op);

void print_report(DSPAM_CTX* CTX);

#endif /* __UTIL_H__ */
