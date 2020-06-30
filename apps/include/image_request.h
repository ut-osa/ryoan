#ifndef _REQUEST_H_
#define _REQUEST_H_

typedef struct health_request_info {
  uint32_t disease_id;
  uint32_t data_len;
  char data[0];
} health_request_info;

#endif
