#include <stdio.h>
#include <string.h>

#include "pipeline/json.h"
#include "pipeline/pipeline.h"

static inline void check_free(void *ptr) {
  if (ptr) free(ptr);
}

static char *alloc_string(const json_value *str) {
  char *ret = NULL;
  if (str->type != json_string) {
    goto done;
  }
  ret = calloc(sizeof(char), str->u.string.length + 1);
  memcpy(ret, str->u.string.ptr, str->u.string.length);
done:
  return ret;
}

void FittingSpec_free(FittingSpec *spec) {
  int i;
  if (spec->in_ids) {
    /*
    for (i = 0; i < spec->nin; ++i) {
        check_free(spec->in_names[i]);
    }
    */
    free(spec->in_ids);
  }
  if (spec->out_ids) {
    /*
    for (i = 0; i < spec->nout; ++i) {
        check_free(spec->out_names[i]);
    }
    */
    for (i = 0; i < spec->nout; ++i) {
      check_free(spec->out_ids[i]);
    }
    free(spec->out_ids);
  }
  if (spec->argv) {
    for (i = 0; i < spec->argc; ++i) {
      check_free(spec->argv[i]);
    }
    free(spec->argv);
  }
  free(spec);
}

/* parse the main JSON object look for required fields */
static int check_fittingspec_json(json_value *val, json_value **id,
                                  json_value **argv, json_value **in,
                                  json_value **out) {
  int i;
  *id = NULL;
  *argv = NULL;
  *in = NULL;
  *out = NULL;

  if (val->type != json_object) {
    fprintf(stderr, "JSON improperly formed\n");
    return -1;
  }

  for (i = 0; i < val->u.object.length; ++i) {
    if (strcmp(val->u.object.values[i].name, "in") == 0) {
      *in = val->u.object.values[i].value;
    }
    if (strcmp(val->u.object.values[i].name, "out") == 0) {
      *out = val->u.object.values[i].value;
    }
    if (strcmp(val->u.object.values[i].name, "id") == 0) {
      *id = val->u.object.values[i].value;
    }
    if (strcmp(val->u.object.values[i].name, "argv") == 0) {
      *argv = val->u.object.values[i].value;
    }
  }

  if (!*id || !*argv || !*in || !*out || (*id)->type != json_integer ||
      (*argv)->type != json_array || (*in)->type != json_array ||
      (*out)->type != json_array) {
    fprintf(stderr, "JSON improperly formed\n");
    return -1;
  }
  return 0;
}

static char **collect_array_of_strings(json_value *array, int extra_slots) {
  char **ret;
  int i;

  ret = calloc(sizeof(char *), array->u.array.length + extra_slots);
  for (i = 0; i < array->u.array.length; ++i) {
    if (!(ret[i] = alloc_string(array->u.array.values[i]))) {
      goto err;
    }
  }

  return ret;

err:
  for (--i; i >= 0; --i) {
    free(ret[i]);
  }
  free(ret);
  return NULL;
}

static int *collect_array_of_integers(json_value *array) {
  int *ret;
  int i;

  ret = calloc(sizeof(int), array->u.array.length);
  if (!ret) return NULL;
  for (i = 0; i < array->u.array.length; ++i) {
    ret[i] = array->u.array.values[i]->u.integer;
  }

  return ret;
}

FittingSpec *FittingSpec_parse(const char *str) {
  FittingSpec *ret = NULL;
  json_value *val, *id, *argv, *in_arr, *out_arr;

  val = json_parse(str, strlen(str));
  if (!val) {
    fprintf(stderr, "JSON parsing failed (%s)\n", str);
    goto done;
  }

  if (check_fittingspec_json(val, &id, &argv, &in_arr, &out_arr)) {
    fprintf(stderr, "JSON malformed\n");
    goto done;
  }

  ret = malloc(sizeof(FittingSpec));
  if (!ret) {
    goto done;
  }
  memset(ret, 0, sizeof(FittingSpec));

  ret->id = id->u.integer;

  if ((ret->nin = in_arr->u.array.length)) {
    if (!(ret->in_ids = collect_array_of_integers(in_arr))) {
      goto err;
    }
  }
  if ((ret->nout = out_arr->u.array.length)) {
    if (!(ret->out_ids = collect_array_of_strings(out_arr, 0))) {
      goto err;
    }
  }

  /*
   * Alloc w/ two extra slots, one for the arg and we need a null pointer
   * to terminate
   */
  ret->argc = argv->u.array.length;
  if (!(ret->argv = collect_array_of_strings(argv, 2))) {
    goto err;
  }

  goto done;

err:
  /* let free check and dealloc the extra buffers for us */
  FittingSpec_free(ret);
  ret = NULL;

done:
  json_value_free(val);
  return ret;
}
