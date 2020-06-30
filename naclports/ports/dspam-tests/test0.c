#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>

#include "util.h"

#define USERNAME "testuser0"

int main (int argc, char **argv) {
    char dspam_home[] = "etc";       /* Our dspam data home */
    char* mess = 0;
    int res = 0;

    DSPAM_CTX* CTX;                         /* DSPAM Context */

    mess = read_message();

    res = dspam_init_driver (NULL);

    res = check_res(res, "driver_init");

    if(res == GENERIC_FAILURE){
        return GENERIC_FAILURE;
    }

    CTX = dspam_init(USERNAME, NULL, dspam_home, DSM_PROCESS, DSF_SIGNATURE | DSF_NOISE);
    
    if (CTX == NULL) {
        fprintf (stderr, "ERROR: dspam_init failed!\n");
        exit (EXIT_FAILURE);
    }

    /* Use graham and robinson algorithms, graham's p-values */
    CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;

    /* Use CHAIN tokenizer */
    CTX->tokenizer = DSZ_CHAIN;

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
