#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pipeline/json-builder.h"
#include "pipeline/pipeline.h"
static inline void check_free(void *ptr) {
  if (ptr) free(ptr);
}

static void free_channel_desc_members(ChannelDesc *set) {
  if (!set) return;
  check_free(set->other_path);
  if (set->size_func) check_free(set->size_func);
}

void WorkSpec_free(WorkSpec *spec) {
  int i;
  if (!spec) return;
  for (i = 0; i < spec->n; ++i) {
    free_channel_desc_members(spec->channels[i]);
  }
  free(spec);
}

/* parse the main JSON object look for required fields */
static int check_workspec_json(json_value *val, json_value **channels) {
  int i;
  if (val->type != json_object) {
    fprintf(stderr, "JSON improperly formed\n");
    return -1;
  }

  for (i = 0; i < val->u.object.length; ++i) {
    if (strcmp(val->u.object.values[i].name, "channels") == 0) {
      *channels = val->u.object.values[i].value;
    }
  }

  if (!*channels || (*channels)->type != json_array) {
    fprintf(stderr, "JSON improperly formed\n");
    return -1;
  }
  return 0;
}

static int extract_channel_desc(json_value *val, ChannelDesc *desc) {
  int i;
  int in = -1, out = -1;
  char *name = NULL;
  char *size_func = NULL;
  Flow_Dir dir = UNDEF;
  Channel_Type type = UNDEF_CHANNEL;

  if (val->type != json_object) goto err;
  for (i = 0; i < val->u.object.length; ++i) {
    char *cur_name = val->u.object.values[i].name;
    json_value *cur_val = val->u.object.values[i].value;

    if (!name && strcmp(cur_name, "name") == 0) {
      if (cur_val->type != json_string) {
        goto err;
      }
      name = malloc(cur_val->u.string.length + 1);
      if (!name) {
        goto err;
      }
      strcpy(name, cur_val->u.string.ptr);
    }
    if (!size_func && strcmp(cur_name, "size_func") == 0) {
      if (cur_val->type != json_string) {
        goto err;
      }
      size_func = malloc(cur_val->u.string.length + 1);
      if (!size_func) {
        goto err;
      }
      strcpy(size_func, cur_val->u.string.ptr);
    }
    if (in < 0 && strcmp(cur_name, "from") == 0) {
      if (cur_val->type != json_integer) {
        goto err;
      }
      in = cur_val->u.integer;
    }
    if (out < 0 && strcmp(cur_name, "to") == 0) {
      if (cur_val->type != json_integer) {
        goto err;
      }
      out = cur_val->u.integer;
    }
    if (dir < 0 && strcmp(cur_name, "dir") == 0) {
      if (cur_val->type != json_string) {
        goto err;
      }
      dir = str_flow_dir(cur_val->u.string.ptr);
    }
    if (type < 0 && strcmp(cur_name, "type") == 0) {
      if (cur_val->type != json_string) {
        goto err;
      }
      type = str_chan_type(cur_val->u.string.ptr);
    }
  }

  desc->other_path = name;
  desc->in_fd = in;
  desc->out_fd = out;
  desc->dir = dir;
  desc->type = type;
  desc->size_func = size_func;
  return 0;

err:
  check_free(name);
  return -1;
}

static int parse_channel_descs(json_value *val, WorkSpec *spec) {
  int i, count = 0;

  assert(val->type == json_array);
  count = val->u.array.length;

  for (i = 0; i < count; ++i) {
    if (extract_channel_desc(val->u.array.values[i], spec->channels[i])) {
      return -1;
    }
  }
  spec->n = count;
  return 0;
}

WorkSpec *WorkSpec_parse(const char *str) {
  int i;
  WorkSpec *ret = NULL;
  json_value *val, *channels_arr = NULL;

  val = json_parse(str, strlen(str));
  if (!val) {
    fprintf(stderr, "JSON parsing failed (%s)\n", str);
    goto done;
  }

  if (check_workspec_json(val, &channels_arr)) {
    fprintf(stderr, "JSON malformed\n");
    goto done;
  }

  ret = malloc(sizeof(WorkSpec));
  if (!ret) {
    goto done;
  }
  ret->channels = calloc(sizeof(ChannelDesc *), channels_arr->u.array.length);
  for (i = 0; i < channels_arr->u.array.length; ++i)
    ret->channels[i] = malloc(sizeof(ChannelDesc));

  if (parse_channel_descs(channels_arr, ret)) {
    fprintf(stderr, "parsing failed\n");
    WorkSpec_free(ret);
    ret = NULL;
  }

done:
  json_value_free(val);
  return ret;
}

/* Callback for object creation */
static json_value *json_object_create_ChannelDesc(void *ptr) {
  ChannelDesc *desc;
  json_value *obj;

  if (!(desc = ptr)) return NULL;
  if (!(obj = json_object_new(5))) return NULL;

  if (!json_object_push_string(obj, "name", desc->other_path) ||
      !json_object_push_int(obj, "from", desc->in_fd) ||
      !json_object_push_int(obj, "to", desc->out_fd) ||
      !json_object_push_string(obj, "dir", flow_dir_str(desc->dir)) ||
      !json_object_push_string(obj, "type", chan_type_str(desc->type))) {
    json_builder_free(obj);
    return NULL;
  }
  if (desc->dir == OUT &&
      !json_object_push_string(obj, "size_func", desc->size_func)) {
    json_builder_free(obj);
    return NULL;
  }
  return obj;
}

static json_value *jsonify_WorkSpec(WorkSpec *work) {
  json_value *val;
  json_create_func f = json_object_create_ChannelDesc;
  if (!(val = json_object_new(1))) return NULL;
  if (!json_object_push_array(val, "channels", work->channels, work->n, f)) {
    json_builder_free(val);
    return NULL;
  }
  return val;
}

char *WorkSpec_serialize(WorkSpec *work) {
  json_value *val;
  char *ret;
  if (!(val = jsonify_WorkSpec(work))) return NULL;
  if ((ret = malloc(json_measure(val)))) json_serialize(ret, val);
  json_builder_free(val);
  return ret;
}

static char *alloc_string(const char *source) {
  size_t len;
  char *ret;
  if (!source) return NULL;
  len = strlen(source);
  if (!len++) return NULL;
  ret = malloc(len);
  if (ret) {
    memcpy(ret, source, len);
  }
  return ret;
}

WorkSpec *fitting_to_workspec(FittingSpec *fit, FittingSpec *id_map[],
                              int from_pred, int to_pred, int from_succ,
                              int to_succ) {
  WorkSpec *work;
  int i = 0;
  work = malloc(sizeof(WorkSpec));
  if (!work) return NULL;
  work->n = 0;
  work->channels = calloc(sizeof(ChannelDesc *), 2);
  if (!work->channels) {
    free(work);
    return NULL;
  }

  /* TODO I'll generalize this later */
  if (from_pred > 0) {
    work->channels[i] = malloc(sizeof(ChannelDesc));
    work->channels[i]->other_path =
        alloc_string(id_map[fit->in_ids[0]]->argv[0]);
    work->channels[i]->in_fd = from_pred, work->channels[i]->out_fd = to_pred;
    work->channels[i]->dir = IN;
    work->channels[i]->type = PLAIN_CHANNEL;
    i++;
  }
  if (to_succ > 0) {
    char *tmp;
    int outid0 = strtol(fit->out_ids[0], &tmp, 0);
    tmp++;
    work->channels[i]->size_func = malloc(strlen(tmp) + 1);
    if (work->channels[i]->size_func == NULL) {
      free(work);
      return NULL;
    }
    strcpy(work->channels[i]->size_func, tmp);
    work->channels[i] = malloc(sizeof(ChannelDesc));
    work->channels[i]->other_path = alloc_string(id_map[outid0]->argv[0]);
    work->channels[i]->in_fd = from_succ, work->channels[i]->out_fd = to_succ;
    work->channels[i]->dir = OUT;
    work->channels[i]->type = PLAIN_CHANNEL;
    i++;
  }
  work->n = i;
  return work;
}
