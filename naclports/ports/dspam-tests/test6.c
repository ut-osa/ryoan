#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>
#include "core.h"
#include "util.h"

#define USERNAME "testuser6"
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
    CTX = create_context(USERNAME, HOME, CLASSIFY);
    check_create(CTX);

    CTX = run(CTX, CLASSIFY, message);

    if(CTX->result != DSR_ISSPAM){
        fprintf(stderr, "Error: message not classified as spam\n");
        fprintf(stderr, "Probability: %2.4f Confidence: %2.4f, Result: %s\n",
            CTX->probability,
            CTX->confidence,
            (CTX->result == DSR_ISSPAM) ? "Spam" : "Innocent");
        dspam_destroy(CTX);
        return GENERIC_FAILURE;
    }

    check_run(CTX);
    printf("Passed\n");
    return 0;
}
