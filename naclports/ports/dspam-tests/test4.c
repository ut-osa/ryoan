#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>

#include "core.h"
#include "util.h"

#define USERNAME "testuser4"
#define HOME "./etc"

int main (int argc, char **argv) {
    char* mess = 0;
    int res = 0;

    DSPAM_CTX* CTX = create_context(USERNAME, HOME, CLASSIFY);
    check_create(CTX);

    mess = read_message();

    check_run(run(CTX, CLASSIFY, mess));

    printf("Passed\n");
    return 0;
}
