#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>
#include "core.h"
#include "util.h"

#define USERNAME "testuser5"
#define HOME "./etc"

int main (int argc, char **argv) {
    char* message = 0;
    int res = 0;
    int i = 0;

    DSPAM_CTX* CTX = create_context(USERNAME, HOME, LEARN_SPAM);
    check_create(CTX);

    message = read_message();

    check_run(run(CTX, LEARN_SPAM, message));

    /* Now check if we classify the message as spam */
    CTX = dspam_init(USERNAME, NULL, HOME, DSM_CLASSIFY, DSF_SIGNATURE | DSF_NOISE);
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
