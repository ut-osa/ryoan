#ifndef _UTIL_H_
#define _UTIL_H_
#include <stdint.h>

typedef struct translation_request_info {
  int32_t text_len;
  char text[0];
} translation_request_info;

#endif
