#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "util.h"

int _select_task(task t){
	switch(t){
        case PROCESS:
		case LEARN_SPAM:
		case LEARN_HAM:
			return DSM_PROCESS;
		case CLASSIFY:
		default:
			return DSM_CLASSIFY;
	}
}

int _select_classification(task t){
	switch(t){
		case LEARN_SPAM:
			return DSR_ISSPAM;
		case LEARN_HAM:
			return DSR_ISINNOCENT;
        case PROCESS:
        case CLASSIFY:
		default:
			return DSR_NONE;
	}
}

int _select_source(task t){
	switch(t){
		case LEARN_SPAM:
		case LEARN_HAM:
			return DSS_ERROR;
        case PROCESS:
        case CLASSIFY:
		default:
			return DSS_NONE;
	}
}

int _select_flags(task t){
	switch(t){
		case LEARN_SPAM:
		case LEARN_HAM:
			return 0;
        case PROCESS:
		case CLASSIFY:
		default:
			return DSF_SIGNATURE | DSF_NOISE;
	}
}

DSPAM_CTX* _clone_context(DSPAM_CTX* c, task t){
	if(c == NULL){
		return NULL;
	}
	// dspam_init strdup's these
	return create_context(c->username, c->home, t);
}

DSPAM_CTX* create_tmp_ctx(DSPAM_CTX *saved, const char* username, const char* dspam_home, task t){
	DSPAM_CTX* CTX = NULL;
	CTX = dspam_create(username, NULL, dspam_home, _select_task(t), _select_flags(t));
  if (CTX == NULL) {
    return NULL;
  }
  CTX->storage = saved->storage;
  /* Use graham and robinson algorithms, graham's p-values */
  CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;
  /* Use CHAIN tokenizer */
  CTX->tokenizer = DSZ_CHAIN;
  /* Set up classification and source appropriately */
  CTX->classification = _select_classification(t);
  CTX->source         = _select_source(t);
  CTX->training_mode  = DST_NOTRAIN;
  //memcpy(&CTX->totals, &saved->totals, sizeof(CTX->totals));
  return CTX;
}

DSPAM_CTX* create_context(const char* username, const char* dspam_home, task t){
	static int driver_init = 0;
	static int res = 0;
	if(! driver_init){
		driver_init = 1;
		res = dspam_init_driver (NULL);
    	res = check_res(res, "driver_init");
    	if(res == GENERIC_FAILURE){
      		fprintf (stderr, "ERROR: dspam_init_driver failed!"
      						 "(DSPAM_CTX* was NULL)\n");
        	exit (EXIT_FAILURE);
    	}
	}
	DSPAM_CTX* CTX = NULL;
	CTX = dspam_init(username, NULL, dspam_home, _select_task(t), _select_flags(t));
    if (CTX == NULL) {
        return NULL;
    }
    /* Use graham and robinson algorithms, graham's p-values */
    CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;
    /* Use CHAIN tokenizer */
    CTX->tokenizer = DSZ_CHAIN;
    /* Set up classification and source appropriately */
  	CTX->classification = _select_classification(t);
  	CTX->source         = _select_source(t);
    return CTX;
}

void check_create(DSPAM_CTX* c){
	if (c == NULL) {
        fprintf (stderr, "ERROR: dspam_init failed! (DSPAM_CTX* was NULL)\n");
        exit (EXIT_FAILURE);
    }
}

DSPAM_CTX* run(DSPAM_CTX* c, task t, char* message){
	DSPAM_CTX* ctx = c;
	DSPAM_CTX* temp = NULL;
	int res = 0;
	char* text = message;
	int i;
	switch(t){
		case LEARN_SPAM:
        case LEARN_HAM:
			for( i = 0; i < 10; ++i){
		        temp = ctx;
		        ctx = _clone_context(temp, t);
		        dspam_destroy(temp);
		        /* Make sure we copied right */
		        check_create(ctx);
		        res = dspam_process(ctx, text);
		        res = check_res(res, "learn");
		        if(res == GENERIC_FAILURE){
		            return NULL;
		        }
		    }
		    break;
        case PROCESS:
		case CLASSIFY:
		default:
			res = dspam_process(ctx, text);
			check_res(res, "classify");
			break;
	}
	return ctx;
}

void check_run(DSPAM_CTX* c){
	if (c == NULL){
		fprintf (stderr, "ERROR: dspam_process failed!\n");
        exit (EXIT_FAILURE);
	}
	dspam_destroy(c);
}
