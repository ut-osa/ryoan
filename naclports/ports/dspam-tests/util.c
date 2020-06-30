#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>
#include <unistd.h>
#include <errno.h>

#include "util.h"

char* read_message(void){
    char buffer[1024];
    char *message = malloc (1);
    long len = 1;
    /* read in the message from stdin */
    message[0] = 0;
    while (fgets (buffer, sizeof (buffer), stdin) != NULL) {
        len += strlen (buffer);
        message = realloc (message, len);
        if (message == NULL) {
            fprintf (stderr, "out of memory!");
            free(message);
            exit (EXIT_FAILURE);
        }
        strcat (message, buffer);
    }
    return message;
}

char* read_message_from_fd(int fd) {
    char buffer[1024];
    char *message = malloc(1);
    message[0] = 0;
    long len  = 1;
    int res;
    while ((res = read (fd, buffer, sizeof(buffer))) != 0) {
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else {
                free(message);
                perror("read");
                exit(EXIT_FAILURE);
            }
        }
        len += res;
        message = realloc (message, len);
        if (message == NULL) {
            fprintf(stderr, "out of memory");
            free(message);
            exit (EXIT_FAILURE);
        }
        strncat(message, buffer, res);
    }
    return message;
}

int check_res(int res, const char* op){
    if(res == EFAILURE){
        fprintf (stderr, 
            "In operation %s: The operation itself has failed\n", op); 
    } else if (res == EINVAL) {
        fprintf (stderr, 
            "In operation %s: An invalid call or invalid parameter used.\n", op);
    } else if (res == EUNKNOWN) {
        fprintf (stderr, 
            "In operation %s: Unexpected error, such as malloc() failure\n", op);
    } else if (res == EFILE) {
        fprintf (stderr, 
            "In operation %s: Error opening or writing to a file or file handle\n", op);
    } else if (res == ELOCK) {
        fprintf (stderr, 
            "In operation %s: Locking failure\n", op);
    } else {
        return GENERIC_SUCCESS;
    }
    return GENERIC_FAILURE;
}

void print_report(DSPAM_CTX* CTX){
    int res = CTX->result;
    printf("Is spam?: %d\n", res == DSR_ISSPAM);
    printf("Is innocent?: %d\n", res == DSR_ISINNOCENT);
}
