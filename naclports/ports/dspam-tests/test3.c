#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>

#include "core.h"
#include "util.h"

#define USERNAME "testuser3"
#define HOME "./etc"

int main (int argc, char **argv) {
    char* mess = 0;
    int res = 0;

    DSPAM_CTX* CTX = create_context(USERNAME, HOME, CLASSIFY);
    check_create(CTX);

    mess = read_message();

    res = dspam_process(CTX, mess);

    res = check_res(res, "process");

    if(res == GENERIC_FAILURE){
        dspam_destroy(CTX);
        return GENERIC_FAILURE;
    }

    if(CTX->result != DSR_ISINNOCENT){
        fprintf(stderr, "Error: message not classified as ham");
        dspam_destroy(CTX);
        return GENERIC_FAILURE;
    }

    dspam_destroy(CTX);
    printf("Passed\n");
    return 0;
}
