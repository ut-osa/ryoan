#ifndef _UTIL_H_
#define _UTIL_H_

#define REQ_TYPE_EMPTY 0  // skip
#define REQ_TYPE_TEXT_AND_ATTACHMENT 1
#define REQ_TYPE_TEXT_ONLY 2
#define REQ_TYPE_ATTACHMENT_ONLY 3
#define REQ_TYPE_END 4  // stop

#define APP_ID_DSPAM 'd'
#define APP_ID_CLAMAV 'c'

typedef struct email_request_info {
  int32_t req_type;
  int32_t email_text_len;
  int32_t attachment_len;
  unsigned char data[0];
} email_request_info;

// dspam info
struct d_info {
  char result_str[16];
};

// clamav info
struct c_info {
  uint16_t sigs;    /* number of signatures */
  uint16_t files;   /* number of scanned files */
  uint16_t ifiles;  /* number of infected files */
  uint16_t errors;  /* number of errors */
  uint32_t blocks;  /* number of *scanned* 16kb blocks */
  uint32_t rblocks; /* number of *read* 16kb blocks */
};

struct pipeline_result {
  struct d_info dinfo;
  struct c_info cinfo;
};
#endif
