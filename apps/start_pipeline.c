/*
 * start_pipeline.c
 *
 * Spawns and connects necessary processes to operate on input
 * Takes a list of pathnames as arguments, spawns the necessary
 * processes as enclaves.
 */
#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <openssl/sha.h>
#include "lqueue.h"
#include "pipeline/pipeline.h"

#include "sgx.h"
#define WHITESPACE " \n\t\r"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define error(...) eprintf("problem with " __VA_ARGS__)
#define set_error(...) error(__VA_ARGS__)
#define MAX_CONNS 64

int user_in_fd_from = -1;
int user_in_fd_to = -1;
int user_out_fd_from = -1;
int user_out_fd_to = -1;
char *user_out_size_func = NULL;

const char *prog_name = "START_PIPELINE";
void handler(int sig) {
  void *array[64];
  size_t size = backtrace(array, 64);
  fprintf(stderr, "%s Error: signal %d:\n", prog_name, sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

struct pipeline_conf {
  int nenclaves;
  FittingSpec *specs[16];
};

void enclave_exit(void) __attribute__((section(".eexit")));
void enclave_exit(void) {
  /* unwind the stack so that we can just jump back
   * to where we were called from */
}

void *addr;
void *aep;
char **__argv;

void errhand(void) {
  SETM(_RBX, addr);
  SETM(_RCX, aep);
  ENCLU(ERESUME);
}

bool add_string(char ***args, int *count, const char *new_str) {
  char *copy;
  char **p;

  if (args == NULL || new_str == NULL ||
      (copy = (char *)calloc(strlen(new_str) + 1, sizeof(char))) == NULL)
    return false;

  strncpy(copy, new_str, strlen(new_str));

  if ((p = (char **)realloc(*args, (*count + 2) * sizeof(char *))) == NULL) {
    free(copy);
    return false;
  }

  *args = p;
  (*args)[(*count)] = copy;
  *count += 1;
  return true;
}

void do_nacl_fd_args(char **argv, char ***args, int *count, int argc) {
  int j;
  WorkSpec *ws = WorkSpec_parse(argv[argc]);
  for (j = 0; j < ws->n; j++) {
    ChannelDesc *cd = ws->channels[j];
    char desc_in_fd[64] = {0};
    char desc_out_fd[64] = {0};
    snprintf(desc_in_fd, 64, "%d:%d", cd->in_fd, cd->in_fd);
    // Hard code the write function for testing
    if (cd->dir == IN) {
      snprintf(desc_out_fd, 64, "%d:%d:a0b0", cd->out_fd, cd->out_fd);
    } else {
      snprintf(desc_out_fd, 64, "%d:%d:%s", cd->out_fd, cd->out_fd,
               cd->size_func);
    }
    add_string(args, count, "-r");
    add_string(args, count, desc_in_fd);
    add_string(args, count, "-w");
    add_string(args, count, desc_out_fd);
  }
}

int launch_enclave(FittingSpec *enclave, int possition) {
  long ret = 0;
  if (!enclave->argv || !enclave->argv[0]) {
    return -1;
  }

  int i, count = 0;
  char **args = NULL;
  for (i = 0; i < enclave->argc; i++) {
    add_string(&args, &count, enclave->argv[i]);
    if (strstr(enclave->argv[i], "sel_ldr"))
      do_nacl_fd_args(enclave->argv, &args, &count, enclave->argc);
  }

  /*
   * sel_ldr is actually a string likely to be found in the spec so do this
   * out here
   */
  add_string(&args, &count, enclave->argv[enclave->argc]);
  for (i = 0; i <= enclave->argc; i++) {
    free(enclave->argv[i]);
  }

  free(enclave->argv);
  enclave->argv = args;
  enclave->argc = count - 1;

#ifndef NOT_SGX
  SGX_init_state state;
  const char *status = (possition == 0 ? "top" : "not");
  char *argv[] = {path, (char *)status, 0};
  if (enclaveinit(path, argv, NULL, &state)) {
    set_error("enclaveinit");
    return -1;
  }
  addr = (void *)ENCLAVE_START + TCS_OFFSET;
  aep = &errhand;
  void *syscall_buf = malloc(SYSCALL_BUFFER_SIZE);

  eenter(&state, &errhand, addr, syscall_buf, SYSCALL_BUFFER_SIZE);
  asm volatile("mov %%r12, %0;" : "=m"(ret) :);
#else
  // printf("executing %s\n", enclave->argv[0]);
  // char* envp[] = {"NACLVERBOSITY=-10", 0};
  // char* envp[] = {0};
  setenv("NACLVERBOSITY", "-10", 1);
  setenv("NO_CHIRON", "TRUE", 1);
  enclave->argv[enclave->argc + 1] = 0;
  printf("\n");
  const char *name;
  for (i = 0; enclave->argv[i]; i++) {
    printf("%s ", enclave->argv[i]);
    if (strstr(enclave->argv[i], ".nexe")) name = enclave->argv[i];
  }
  printf("\n");
  printf("XXXXXXXXXXXXXXXXXXXXX %u, %s\n", getpid(), name);
  fflush(stdout);
  execv(enclave->argv[0], enclave->argv);
  perror("WHAT?!?!? I'M ALIVE?");
#endif
  return ret;
}

static char *alloc_string(const char *other) {
  char *ret;
  size_t len;
  if (!other) return NULL;
  if (!(len = strlen(other))) return NULL;
  if ((ret = malloc(++len))) memcpy(ret, other, len);
  return ret;
}

int chain(const char *curr_name, const char *next_name, const char *size_func,
          WorkSpec *curr_work, WorkSpec *next_work, Channel_Type type) {
  int curr_to_next[2], next_to_curr[2];
  ChannelDesc *curr, *next;

  if (pipe(curr_to_next) || pipe(next_to_curr)) {
    return -1;
  }

  curr = curr_work->channels[curr_work->n++] = malloc(sizeof(ChannelDesc));
  next = next_work->channels[next_work->n++] = malloc(sizeof(ChannelDesc));

  if (!curr || !next) goto err;
  curr->size_func = alloc_string(size_func);
  next->size_func = NULL;

  curr->other_path = alloc_string(next_name);
  next->other_path = alloc_string(curr_name);
  if (!curr->other_path || !next->other_path) goto err;

  curr->dir = OUT;
  next->dir = IN;

  curr->type = next->type = type;

  curr->in_fd = next_to_curr[0];
  curr->out_fd = curr_to_next[1];
  next->in_fd = curr_to_next[0];
  next->out_fd = next_to_curr[1];

  return 0;

err:
  if (next->other_path) free(next->other_path);
  if (curr->other_path) free(curr->other_path);
  if (curr) free(curr);
  if (next) free(next);
  curr_work->n--;
  next_work->n--;
  return -1;
}

void close_all_fds(WorkSpec *spec) {
  ChannelDesc **ch;
  for (ch = spec->channels; ch < spec->channels + spec->n; ++ch) {
    close((*ch)->in_fd);
    close((*ch)->out_fd);
  }
}

int spawn_enclaves(FittingSpec *commands[], FittingSpec *id_map[], pid_t pids[],
                   unsigned n) {
  struct link {
    FittingSpec *spec;
    struct lqueue_link ql;
  };
  struct fd_link {
    int fd;
    struct lqueue_link ql;
  };

  struct link *cur, *new_link;
  int i;
  FittingSpec *next_spec, *spec;
  WorkSpec **work, *cur_work;

  LQUEUE(struct link, ql) node_queue = LQUEUE_INIT;

  work = malloc(sizeof(WorkSpec) * n);
  for (i = 0; i < n; ++i) {
    work[i] = malloc(sizeof(WorkSpec));
    work[i]->n = 0;
    work[i]->channels = calloc(sizeof(ChannelDesc *), MAX_CONNS);
  }

  /*
   * I assume:
   *  - every node is reachable
   *  - node descriptions are consistent
   *  - graph is a dag
   *
   * We first find all source nodes (no in edges)
   * Then to BFS to attach all enclaves to the pipeline
   * If my assumptions above hold this should result in a correctly configured
   * pipeline
   */

  /* find source nodes */
  for (i = 0; i < n; ++i) {
    commands[i]->visited = 0;
    if (!commands[i]->nin) {
      cur = malloc(sizeof(*cur));
      cur->spec = commands[i];
      lqueue_enqueue(&node_queue, cur);
    }
  }

  /* BFS */
  while ((cur = lqueue_dequeue(&node_queue))) {
    spec = cur->spec;
    if (spec->visited) continue;
    spec->visited = 1;
    cur_work = work[spec->id];
    if (spec->nout == 0 && user_out_fd_from >= 0) {
      ChannelDesc *currd = cur_work->channels[cur_work->n++] =
          malloc(sizeof(ChannelDesc));
      if (!currd) return -1;
      currd->other_path = alloc_string("user_out");
      if (!currd->other_path) return -1;
      currd->dir = OUT;
      currd->type = PLAIN_CHANNEL;
      currd->in_fd = user_out_fd_from;
      currd->out_fd = user_out_fd_to;
      if (user_out_size_func) {
        currd->size_func = malloc(strlen(user_out_size_func));
        strcpy(currd->size_func, user_out_size_func);
      } else {
        currd->size_func = NULL;
      }
    }
    if (spec->nin == 0 && user_in_fd_from >= 0) {
      ChannelDesc *currd = cur_work->channels[cur_work->n++] =
          malloc(sizeof(ChannelDesc));
      if (!currd) return -1;
      currd->size_func = NULL;
      currd->other_path = alloc_string("user_in");
      if (!currd->other_path) return -1;
      currd->dir = IN;
      currd->type = PLAIN_CHANNEL;
      currd->in_fd = user_in_fd_from;
      currd->out_fd = user_in_fd_to;
    }
    for (i = 0; i < spec->nout; ++i) {
      char *tmp;
      int outid = strtol(spec->out_ids[i], &tmp, 0);
      next_spec = id_map[outid];
      tmp++;

      /*if we reach a node that was visited our dag assumption was
        violated so nothing means anything anymore */
      assert(!next_spec->visited);

      new_link = malloc(sizeof(*new_link));
      new_link->spec = id_map[outid];
      lqueue_enqueue(&node_queue, new_link);
      fflush(stdout);
      chain(spec->argv[0], next_spec->argv[0], tmp, cur_work,
            work[next_spec->id], PLAIN_CHANNEL);
    }
    spec->argv[spec->argc] = WorkSpec_serialize(cur_work);
    if (!(pids[--n] = fork())) {
      /* CHILD */
      for (i = 0; i < spec->nout; ++i) {
        int outid = strtol(spec->out_ids[i], NULL, 0);
        close_all_fds(work[outid]);
      }
      launch_enclave(spec, 0);
      abort(); /* should not get here*/
    }
    close_all_fds(cur_work);
    WorkSpec_free(cur_work);
    free(cur);
  }

  return 0;
}

char *generate_data(int size, unsigned long seed) {
  static char vischars[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
  size_t i;
  srand(seed);
  char *data = malloc(size);
  if (!data) {
    error("malloc data");
    return NULL;
  }
  for (i = 0; i < size; ++i) {
    data[i] = vischars[rand() % sizeof(vischars) - 2];
  }
  return data;
}

int get_multiplier(const char *data) {
  int len = strlen(data);
  const char *suffix = data + len - 2;
  if (strcmp(suffix, "KB") == 0) {
    return 1024;
  }
  if (strcmp(suffix, "MB") == 0) {
    return 1024 * 1024;
  }
  if (strcmp(suffix, "GB") == 0) {
    return 1024 * 1024 * 1024;
  }
  return 1;
}

int parse_data_size(const char *data) {
  int mult;

  mult = get_multiplier(data);
  return atoi(data) * mult;
}

FittingSpec *parse_cmd_line(char *line) { return FittingSpec_parse(line); }

int parse_config(FILE *fp, struct pipeline_conf *conf, FittingSpec *id_map[]) {
  ssize_t read;
  FittingSpec **cur_spec;
  char *line = NULL;
  size_t len = 0;

  cur_spec = conf->specs;
  while ((read = getline(&line, &len, fp)) > 0) {
    if (!(*cur_spec = parse_cmd_line(line))) {
      continue;
    }
    if ((*cur_spec)->argv && (*cur_spec)->argv[0]) {
      if (access((*cur_spec)->argv[0], X_OK) == 0) {
        id_map[(*cur_spec)->id] = *cur_spec;
        cur_spec++;
      } else {
        perror((*cur_spec)->argv[0]);
        abort();
      }
    } else {
      FittingSpec_free(*cur_spec);
    }
  }

  conf->nenclaves = cur_spec - conf->specs;
  if (line) {
    free(line);
  }
  return 0;
}
static void kill_others(pid_t *pids, int n) {
  while (n-- > 0) {
    kill(pids[n], SIGKILL);
  }
}

void usage(char *argv[]) {
  eprintf(
      "Useage: %s <pipeline_desc.json> [--in=<fd from>,<fd to>] "
      " [--out=<fd from>,<fd to> --out_size_func=<e.g., a0b100>]\n",
      argv[0]);
}

int main(int argc, char *argv[]) {
  pid_t *pids;
  int ret;
  int n, i;
  struct pipeline_conf conf = {0};
  FittingSpec *id_to_spec[16];

  __argv = argv;
  signal(SIGSEGV, handler);

  if (argc < 2) {
    usage(argv);
    return 1;
  }

  {
    FILE *conf_fp;
    conf_fp = fopen(argv[1], "r");
    if (conf_fp < 0) {
      perror("config");
      return 1;
    }

    if (parse_config(conf_fp, &conf, id_to_spec)) {
      return 1;
    }
    fclose(conf_fp);
  }
  if (!conf.nenclaves) {
    eprintf("No enclaves to spawn\n");
    return 1;
  }
  n = conf.nenclaves;
  eprintf("%d total enclaves\n", n);

  for (i = 2; i < argc; i++) {
    int fdfrom, fdto;
    if (sscanf(argv[i], "--in=%d,%d", &fdfrom, &fdto) == 2) {
      user_in_fd_from = fdfrom;
      user_in_fd_to = fdto;
      eprintf("user_in_fd: from %d to %d\n", fdfrom, fdto);
    } else if (sscanf(argv[i], "--out=%d,%d", &fdfrom, &fdto) == 2) {
      user_out_fd_from = fdfrom;
      user_out_fd_to = fdto;
      eprintf("user_out_fd: from %d to %d\n", fdfrom, fdto);
    } else if (memcmp(argv[i], "--out_size_func=",
                      sizeof("--out_size_func=") - 1) == 0) {
      user_out_size_func = argv[i] + sizeof("--out_size_func=") - 1;
    } else {
      usage(argv);
      return 1;
    }
  }

  if (user_out_fd_to >= 0 && strlen(user_out_size_func) == 0) {
    eprintf("no user_out_size_func\n");
    usage(argv);
    return 1;
  }

  pids = malloc(sizeof(pid_t) * n);
  if (!pids) {
    ret = 1;
    perror("malloc pids");
    goto done;
  }

  ret = spawn_enclaves(conf.specs, id_to_spec, pids, n);

  if (ret) {
    ret = 1;
    error("spawn_enclaves");
    goto done;
  }

  n = conf.nenclaves;
  while (--n > -1) {
    int status;
    if (waitpid(pids[n], &status, 0) < 0) {
      perror("waidpid");
    } else {
      if (WIFEXITED(status)) {
        if (WEXITSTATUS(status)) {
          ret = WEXITSTATUS(status);
          kill_others(pids, n);
          fprintf(stderr, "death caused by: %s (returned %d)\n",
                  conf.specs[n]->argv[0], ret);
          break;
        }
        if (WIFSIGNALED(status)) {
          ret = -WTERMSIG(status);
          kill_others(pids, n);
          fprintf(stderr, "death caused by: %s (got signal: (%d)\n",
                  conf.specs[n]->argv[0], -ret);
          break;
        }
      }
    }
  }
  eprintf("done\n");

done:
  free(pids);
  return ret;
}
