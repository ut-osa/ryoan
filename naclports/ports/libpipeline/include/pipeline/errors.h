#ifndef _ERRORS_H
#define _ERRORS_H

#include <errno.h>
#include <stdio.h>
extern const char* __error_string;
extern const char* prog_name;

#define eprintf(fmt...) do {\
    fprintf(stderr, "%s: ", prog_name);\
    fprintf(stderr, fmt);\
}while(0)


#define set_error(err) do {\
   __error_string = err;   \
} while(0)

#define clear_error() do {\
   __error_string = NULL; \
} while(0)

#define error(msg) do {                 \
   if (__error_string)   {               \
       fprintf(stderr, "%s: (%s)", prog_name, __error_string);\
   } else { \
       fprintf(stderr, "%s: ", prog_name);\
   }\
   if (errno) {                         \
       perror(msg);                     \
   }                                    \
   else fprintf(stderr, msg"\n");               \
} while(0);
#endif
