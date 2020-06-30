#ifndef _TRUSTED_SERVICE_RUNTIME_PARAMETER_SERVER_H_
#define _TRUSTED_SERVICE_RUNTIME_PARAMETER_SERVER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <python3.5/Python.h>

struct parameter_server;
typedef struct parameter_server ParamServer;

struct param {
   float *raw;
   size_t count;
   char *py_str;
};
struct param_set {
   size_t count;
   struct param* data;
};
typedef struct param_set ParamSet;

struct NaClApp;
ParamServer *init_parameter_server(struct NaClApp *nap,
                                               const char *parameter_server_string);
int send_parameters(ParamServer *srv, ParamSet *p);
int recv_parameters(ParamServer *srv, ParamSet *p);
int send_sparse_parameter_updates(ParamServer *srv, ParamSet *set, int **indices, float **values, int *counts, size_t num_params, bool incr);
int send_parameter_updates(ParamServer *srv, ParamSet *p, long long staleness, bool incr);
long long recv_iter(ParamServer *srv);

static inline
int do_param_diff(ParamSet *d, ParamSet *l, ParamSet *r)
{
   size_t i,j;
   for (i = 0; i < d->count; i++) {
      for (j = 0; j < d->data[i].count; j++)
         d->data[i].raw[j] = l->data[i].raw[j] - r->data[i].raw[j];
   }
   return 0;
}

static inline
ParamSet *malloc_param_set(size_t count)
{
   ParamSet *res;

   res = malloc(sizeof(*res) + count * sizeof(struct param));
   if (!res)
      return NULL;
   res->count = count;
	res->data = (struct param *)(((uint8_t *)res) + sizeof(*res));
   return res;
}

static inline
void free_param_set(ParamSet *set)
{
   free(set);
}


#endif
