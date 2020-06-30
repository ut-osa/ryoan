#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>

#include "util.h"

#define USERNAME "testuser1"

int main (int argc, char **argv) {
    char dspam_home[] = "etc";       /* Our dspam data home */
    char* message = 0;
    int res = 0;
    int i = 0;

    DSPAM_CTX *CTX;                         /* DSPAM Context */

    message = read_message();

    res = dspam_init_driver (NULL);

    res = check_res(res, "driver_init");

    if(res == GENERIC_FAILURE){
        return GENERIC_FAILURE;
    }

    for( i = 0; i < 10; ++i){
        CTX = dspam_init(USERNAME, NULL, dspam_home, DSM_PROCESS, 0);
        
        if (CTX == NULL) {
            fprintf (stderr, "ERROR: dspam_init failed!\n");
            exit (EXIT_FAILURE);
        }

        /* Use graham and robinson algorithms, graham's p-values */
        CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;
        /* Use CHAIN tokenizer */
        CTX->tokenizer = DSZ_CHAIN;
        /* Classify the message as spam */
        CTX->classification = DSR_ISSPAM;
        CTX->source         = DSS_ERROR;

        res = dspam_process(CTX, message);

        res = check_res(res, "process");

        dspam_destroy(CTX);

        if(res == GENERIC_FAILURE){
            return GENERIC_FAILURE;
        }
    }
    

    /* Now check if we classify the message as spam */
    CTX = dspam_init(USERNAME, NULL, dspam_home, DSM_CLASSIFY, DSF_SIGNATURE | DSF_NOISE);
    if (CTX == NULL) {
        fprintf (stderr, "ERROR: dspam_init failed!\n");
        exit (EXIT_FAILURE);
    }

    CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;
    CTX->tokenizer = DSZ_CHAIN;
    CTX->classification = DSR_NONE;
    CTX->source         = DSS_NONE;

    res = dspam_process(CTX, message);

    res = check_res(res, "process");

    if(res == GENERIC_FAILURE){
        dspam_destroy(CTX);
        return GENERIC_FAILURE;
    }

    if(CTX->result != DSR_ISSPAM){
        fprintf(stderr, "Error: message not classified as spam\n");
        fprintf(stderr, "Probability: %2.4f Confidence: %2.4f, Result: %s\n",
            CTX->probability,
            CTX->confidence,
            (CTX->result == DSR_ISSPAM) ? "Spam" : "Innocent");
        dspam_destroy(CTX);
        return GENERIC_FAILURE;
    }

    dspam_destroy(CTX);
    printf("Passed\n");
    return 0;
}
