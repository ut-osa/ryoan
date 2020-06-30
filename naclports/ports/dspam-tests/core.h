#ifndef __CORE_H__
#define __CORE_H__

#include <dspam/libdspam.h>

typedef enum {
	CLASSIFY, PROCESS, LEARN_SPAM, LEARN_HAM, UNDEF1
} task;

int _select_task(task t);

int _select_classification(task t);

int _select_source(task t);

int _select_flags(task t);

DSPAM_CTX* _clone_context(DSPAM_CTX* c, task t);

DSPAM_CTX* create_context(const char* username, const char* dspam_home, task t);

void check_create(DSPAM_CTX* c);

DSPAM_CTX* run(DSPAM_CTX* c, task t, char* message);

void check_run(DSPAM_CTX* c);

#endif /* __CORE_H__ */
