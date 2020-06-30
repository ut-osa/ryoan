#include "native_client/src/trusted/service_runtime/parameter_server.h"
#include "native_client/src/trusted/service_runtime/trusted_exec.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "redis-3.2.8/deps/hiredis/hiredis.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#define HOST_IP_BUFSIZE 256

struct parameter_server {
   redisContext *c;
   bool no_encrypt;
   int port;
   char hostip[HOST_IP_BUFSIZE];
};

static redisContext *connect_to_server(char *host_ip, int port)
{
   redisContext *res;
   struct timeval timeout = { 1, 500000 }; // 1.5 seconds

   res = redisConnectWithTimeout(host_ip, port, timeout);
   if (res && res->err) {
      te_err("Parameter Server Connection error: %s\n", res->errstr);
      redisFree(res);
      res = NULL;
   }
   return res;
}

static int parse_server_arg(ParamServer *srv)
{
   int i;
   char *port_string = NULL;

   for (i = 0; srv->hostip[i]; i++) {
      if (srv->hostip[i] == ':') {
         srv->hostip[i] = 0;
         port_string = srv->hostip + i + 1;
         break;
      }
   }
   if (!port_string)
      return -1;

   srv->port = (int)strtol(port_string, NULL, 0);
   if (srv->port < 1)
      return -1;
   return 0;
}

ParamServer *init_parameter_server(struct NaClApp *nap, const char *parameter_server_string)
{
   ParamServer *result;

   result = malloc(sizeof(ParamServer));
   if (!result) {
      perror("malloc parameter_server");
      return NULL;
   }
   memset(result, 0, sizeof(*result));

   strncpy(result->hostip, parameter_server_string, HOST_IP_BUFSIZE - 1);
   if (parse_server_arg(result)) {
      te_err("Bad parameter server argument \"%s\"\n", parameter_server_string);
      goto err;
   }
   result->c = connect_to_server(result->hostip, result->port);
   if (!result->c)
      goto err;
   result->no_encrypt = nap->no_encrypt;
	if (result->no_encrypt)
		printf("PS not using encryption...\n");
   return result;

err: free(result);
   return NULL;
}

redisReply *send_command(ParamServer *srv, const char *format, ...) {
    va_list ap;
    void *reply;
    va_start(ap, format);
    reply = redisvCommand(srv->c, srv->no_encrypt, format, ap);
    va_end(ap);
    return reply;
}

int send_parameters(ParamServer *srv, ParamSet *set)
{
    size_t i;
//    float *vel;
    int ret = 0;
    redisReply *reply;

    for (i = 0; i < set->count; i++) {
		 printf("Sending %s to redis\n", set->data[i].py_str);
       reply = send_command(srv, "SET %s %b", set->data[i].py_str, set->data[i].raw,
                            set->data[i].count * sizeof(float));
       if (reply->type == REDIS_REPLY_ERROR) {
          te_err("REDIS reported an ERROR: %s\n", reply->str);
          ret = -1;
       }
       // set initial velocity for momentum
       /*vel = malloc(set->data[i].count * sizeof(float));
         memset(vel, 0, set->data[i].count * sizeof(float));
         reply = redisCommand(srv->c, "SET v_%s %b", set->data[i].py_str,
                              vel,
                              set->data[i].count * sizeof(float));
         free(vel);
         if (reply->type == REDIS_REPLY_ERROR) {
            te_err("REDIS reported an ERROR: %s\n", reply->str);
            ret = -1;
         }

        freeReplyObject(reply);*/ 
       if (ret)
          break;
    }
    return ret;
}

int send_parameter_updates(ParamServer *srv, ParamSet *set, long long staleness, bool incr)
{
	size_t i;
	int ret = 0;
	redisReply *reply;

	for (i = 0; i < set->count; i++) {
		if (staleness > 0){
			reply = send_command(srv, "ADD %s %b %lld", set->data[i].py_str,
						            set->data[i].raw, set->data[i].count * sizeof(float), 
									   staleness);
		} else {
			reply = send_command(srv, "ADD %s %b", set->data[i].py_str,
					               set->data[i].raw, set->data[i].count * sizeof(float));
		}

       if (reply->type == REDIS_REPLY_ERROR) {
          te_err("REDIS reported an ERROR (on index %lu): %s\n", i, reply->str);
          ret = -1;
       }
       freeReplyObject(reply);

       if (ret)
          break;
    }
	
	if (incr){
		// set counter to calculate staleness
		reply = send_command(srv, "INCR counter");
		if (reply->type == REDIS_REPLY_ERROR) {
		   te_err("REDIS reported an ERROR: %s\n", reply->str);
		   ret = -1;
		}
		freeReplyObject(reply);
	}
   return ret;
}


int send_sparse_parameter_updates(ParamServer *srv, ParamSet *set, int **indices, float **values, int *counts, size_t num_params, bool incr)
{
	size_t i;
	int ret = 0;
	redisReply *reply;

	for (i = 0; i < num_params; i++) {
		if (counts[i] == 0 || !indices[i] || !values[i])
			continue;
		reply = redisCommand(srv->c, "SPADD %s %b %b", set->data[i].py_str,
  				               indices[i], counts[i] * sizeof(int),
									values[i], counts[i] * sizeof(float));

		if (reply->type == REDIS_REPLY_ERROR) {
			te_err("REDIS reported an ERROR (on index %lu): %s\n", i, reply->str);
			ret = -1;
		}

		freeReplyObject(reply);
		if (ret)
			break;
	}

	if (incr){
		// set counter to calculate staleness
		reply = redisCommand(srv->c, "INCR counter");
		if (reply->type == REDIS_REPLY_ERROR) {
		   te_err("REDIS reported an ERROR: %s\n", reply->str);
		   ret = -1;
		}
		freeReplyObject(reply);
	}
   return ret;
}



int recv_parameters(ParamServer *srv, ParamSet *set)
{
    size_t i;
    int ret = 0;
    redisReply *reply;
    for (i = 0; i < set->count; i++) {
       reply = send_command(srv,"GET %s", set->data[i].py_str);
       switch (reply->type) {
          case REDIS_REPLY_NIL:
             ret = -1;
             break;
          case REDIS_REPLY_ERROR:
             te_err("REDIS reported an ERROR: %s\n", reply->str);
             ret = -1;
             break;
          case REDIS_REPLY_STRING:
             if ((size_t)reply->len != set->data[i].count * sizeof(float)) {
               te_err("paramsz sanity check failed! got %d expected %lu\n",
                      reply->len, set->data[i].count * sizeof(float));
               ret = -1;
               break;
             }
             memcpy(set->data[i].raw, reply->str, reply->len);
             break;
          case REDIS_REPLY_STATUS:
	       	 te_err("REDIS reported an STATUS: %s\n", reply->str);
             ret = -1;
             break;
          default:
             te_err("REDIS unexpected reply type: %d\n", reply->type);
       }

       freeReplyObject(reply);
       if (ret)
          break;
    }
    return ret;
}


long long recv_iter(ParamServer *srv)
{
    redisReply *reply;
    char *endptr;
    long long res;
    reply = send_command(srv,"GET counter");
    switch (reply->type) {
          case REDIS_REPLY_NIL:
             res = 0;
             break;
          case REDIS_REPLY_ERROR:
             te_err("REDIS reported an ERROR: %s\n", reply->str);
             res = -1;
             break;
          case REDIS_REPLY_STRING:
             if (reply->str[0] == '\0')
                res = 0;
             else{
                res = strtoll(reply->str, &endptr, 10);
                if(*endptr != '\0') {
                    te_err("REDIS reply parsing incorrcet\n");
                    res = -1;
                }
             }
             break;
          case REDIS_REPLY_INTEGER:
             res = reply->integer;
             break;
          default:
             res = -1;
             te_err("REDIS unexpected reply type: %d\n", reply->type);
       }
    freeReplyObject(reply);
    return res;
}
