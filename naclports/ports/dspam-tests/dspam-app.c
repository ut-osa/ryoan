/**
	An application to wrap around the libdspam to facilitate a commandline
	interface like spamassassin offers.
	Ideally this ends up looking like an argparser that makes various calls
	to functions defined in util.h/util.c - either that or make another
	pair of source/header files to contain those methods.
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dspam/libdspam.h>
#include <argp.h>

#include <pipeline/pipeline.h>
#include <pipeline/worker.h>

#include "email_pipe_defs.h"
#include "util.h"
#include "core.h"

work_ctx pipeline_ctx;
int for_nacl_pipeline = 0;
char *desc_pipeline;
int64_t dlen_pipeline;
char *data_pipeline;
int64_t len_pipeline;
bool output_app_label = false;

const char *prog_name = "dspam-app";

/**
	Defining our input options here.
*/

const char *argp_program_version = "dspam-app 1.0";
const char *argp_program_bug_address = "<ian.glen.neal@utexas.edu>";

/* Program documentation. */
static char doc[] =
    "DSPAM Commandline Application -- an email classifier that uses libdspam to process "
    "email without the bulk/inflexibility of the standard dspam application.";

/* A description of the arguments we accept. */
static char args_doc[] = "[MESSAGE]";

/* The options we understand. */
static struct argp_option options[] = {
    {"process",    'p', 0,      0, "Classify the message as spam or ham, "
                                   "as well as adding it to the user's models"},
    {"classify",   'c', 0,      0, "Classify the message as spam or ham, "
                                   "given the user's learned models - this "
                                   "is the default method of operation if none is chosen"},
    {"inmem",      'm', 0,      0, "Use in memory db (copied from disk)"},
    {"for-nacl-pipeline", 'n',0,0, "For pipeline"},
    {"spam",       'x', 0,      0, "Learn that the given message is spam"},
    {"ham",        'y', 0,      0, "Learn that the given message is ham" },
    {"verbose",    'v', 0,      0, "Produce verbose output" },
    {"quiet",      'q', 0,      0, "Don't produce any output" },
    {"silent",     's', 0,      OPTION_ALIAS },
    {"username",   'u', "NAME", 0, "Name of user to store data as. Defaults to current user" },
    {"dspam-home", 'h', "DIR" , 0, "Location to store data at. Defaults to $(pwd)/etc" },
    {"input",      'i', "FILE", 0, "Input from FILE instead of standard input or MESSAGE argument" },
    {"input-fd",   'f', "FD", 0, "Input from FD instead of standard input or MESSAGE argument" },
    {"output",     'o', "FILE", 0, "Output to FILE instead of standard output" },
    {"output-app-label", 'l', 0,0, "Whether output app label"},
    { 0 } /* end of array of options */
};

/* Used by main to communicate with parse_opt. */
struct arguments {
    int verbose, silent, input_from_file;
    task t;
    char* message;                /* message argument */
    char* username;
    char* home_dir;
    char* input_file;
    char* output_file;
    int fd;
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key) {
        case 'p':
            arguments->t = PROCESS;
            break;
        case 'c':
            arguments->t = CLASSIFY;
            break;
        case 'x':
            arguments->t = LEARN_SPAM;
            break;
        case 'y':
            arguments->t = LEARN_HAM;
            break;
        case 'q':
        case 's':
            arguments->silent = 1;
            break;
        case 'v':
            arguments->verbose = 1;
            break;
        case 'n':
            for_nacl_pipeline = 1;
            break;
        case 'm':
            dspam_load_db_to_mem = 1;
            break;
        case 'u':
            arguments->username = arg;
            break;
        case 'h':
            arguments->home_dir = arg;
            break;
        case 'i':
            /* Redirect IO */
            arguments->input_file = arg;
            stdin = freopen(arg, "r", stdin);
            if (stdin == NULL){
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'f':
            arguments->fd = atoi(arg);
            break;
        case 'o':
            /* Redirect IO */
            arguments->output_file = arg;
            FILE* temp = freopen(arg, "w", stdout);
            if (temp == NULL){
                return ARGP_ERR_UNKNOWN;
            } else {
                stdout = temp;
            }
            break;
        case 'l':
            /* output app label */
            output_app_label = true;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1){
                /* Too many arguments. */
                argp_usage (state);
            }
            arguments->message = arg;
            break;
        case ARGP_KEY_END:
          	/* zero arguments is okay */
          	break;
    	default:
          if (for_nacl_pipeline) {
            return ARGP_ERR_UNKNOWN;
          }
          return 0;
  	}
  	return 0;
}

/* Our argp parser. Also supports --help and --version */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main (int argc, char** argv) {

	struct arguments arguments;
  DSPAM_CTX* context;

  uint8_t* buffer;
  uint64_t size;
  uint64_t start, end;

	/* Default values. */
    arguments.t = CLASSIFY;
    arguments.message = 0;
	arguments.silent = 0;
	arguments.verbose = 0;
    arguments.input_file = 0;
	arguments.output_file = 0;
    arguments.home_dir = "./etc";
    arguments.username = getlogin();

	/*
        error_t argp_parse (const struct argp *argp,
        int argc, char **argv, unsigned flags, int *arg_index, void *input)
	*/
	argp_parse (&argp, argc, argv, 0, 0, &arguments);
    /* Make sure that silent overrides verbose */
    arguments.verbose = arguments.silent ? 0 : arguments.verbose;

    if( arguments.verbose ){
        printf (
            "MESSAGE (%p) = \'%s\'\nDSPAM_HOME = %s\nUSERNAME = %s\n"
            "INPUT_FILE = %s\nOUTPUT_FILE = %s\n"
            "VERBOSE = %s\nSILENT = %s\n"
            "TASK = %s\n",
            arguments.message ? arguments.message : "<from input>",
            arguments.message,
            arguments.home_dir,
            arguments.username,
            arguments.input_file,
            arguments.output_file,
            arguments.verbose ? "yes" : "no",
            arguments.silent ? "yes" : "no",
            arguments.t == UNDEF1 ? "UNDEF" :
                arguments.t == CLASSIFY ? "CLASSIFY" :
                arguments.t == LEARN_SPAM ? "LEARN_SPAM" :
                arguments.t == PROCESS ? "PROCESS" :
                "LEARN_HAM"
        );
    }

    if (for_nacl_pipeline) {
      WorkSpec *spec = WorkSpec_parse(argv[argc - 1]);
      if (!spec || spec->n <= 0) {
        fprintf(stderr, "!no spec\n");
        return 1;
      }
      if (setup_for_work(spec)) {
        fprintf(stderr, "!failed to setup for work\n");
        return 1;
      }
      pipeline_ctx = alloc_ctx();
      if (!pipeline_ctx) {
        fprintf(stderr, "!no pipeline_ctx\n");
        return 1;
      }
    }

    /* Start processing */
    context = create_context(arguments.username, arguments.home_dir, arguments.t);
    check_create(context);

    fprintf(stderr, "dspam: Ready to process\n");
    while (1) {
      DSPAM_CTX* ctx;
      char tmp[sizeof(struct d_info) + 1];
      struct d_info *dinfo = (struct d_info *)(tmp + 1);
      tmp[0] = APP_ID_DSPAM;

      /* Make sure message isn't left as null */
      if (for_nacl_pipeline) {
        memset(dinfo, 0, sizeof(struct d_info));
#ifdef __native_client__
        if (wait_for_chan(output_app_label, NULL, NULL,
                    NULL, NULL, NULL)) {
          fprintf(stderr, "!wait for chan failed\n");
          return 1;
        }
        int not_ready = get_work_desc(pipeline_ctx, &desc_pipeline, &dlen_pipeline, &data_pipeline, &len_pipeline);
        if (not_ready || !desc_pipeline || !data_pipeline || len_pipeline <= 0) {
          fprintf(stderr, "!not ready\n");
          return 2;
        }
#else
        int ret = get_work_desc_single(pipeline_ctx, &desc_pipeline, &dlen_pipeline, &data_pipeline, &len_pipeline);
        if (ret || !desc_pipeline || !data_pipeline || len_pipeline <= 0) {
          fprintf(stderr, "!not ready\n");
          return 2;
        }
#endif
        email_request_info *einfo = (email_request_info *)data_pipeline;
        arguments.message = einfo->data;
        einfo->data[einfo->email_text_len - 1] = 0; // make sure ending with null
      } else if (!arguments.message) {
        if (arguments.input_file != NULL) {
          arguments.message = read_message();
        } else {
          arguments.message = read_message_from_fd(arguments.fd);
        }
      }

      ctx = create_tmp_ctx(context, arguments.username, arguments.home_dir, arguments.t);

      /* abort if context was not created correctly */
      ctx = run(ctx, arguments.t, arguments.message);
      if(arguments.t == CLASSIFY || arguments.t == PROCESS){
        if(ctx->result != DSR_ISINNOCENT){
          if (for_nacl_pipeline)
            sprintf(dinfo->result_str, "spam");
          else
            printf("Dspam-Classification: spam\n");
        } else {
          if (for_nacl_pipeline)
            sprintf(dinfo->result_str, "innocent");
          else
            printf("Dspam-Classification: innocent\n");
        }
        printf("Input: %.*s ...\n", 32, arguments.message);
      }
      ctx->storage = NULL;
      check_run(ctx);

      if (for_nacl_pipeline) {
        put_work_desc(pipeline_ctx, desc_pipeline, dlen_pipeline, tmp, 1 + sizeof(struct d_info));
        free(data_pipeline);
        free(desc_pipeline);
      } else {
        break;
      }
    }
    return 0;
}
