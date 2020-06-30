
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
//#include <execinfo.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"
#include "pipeline/json-builder.h"
#include "pipeline/json.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__); \
    fflush(stderr);


const char *const prog_name = "JSON_JOIN";
#define MAX_RESULTS 64

int json_strcmp(json_value *st1, json_value *st2)
{
    if (!st1) return 1;
    if (!st2) return -1;
    assert(st1->type == json_string && st2->type == json_string);
    return strcmp(st1->u.string.ptr, st2->u.string.ptr);
}

int hash_cmp(json_value *lhs, json_value *rhs)
{

    assert(lhs->type == json_object && rhs->type == json_object);
    return json_strcmp(json_object_find(lhs, "hash"),
            json_object_find(rhs, "hash"));
}


int check_hash(json_value **begin, json_value **end, json_value *val)
{
    while (begin < end--) {
        if (hash_cmp(*end, val) == 0) return end - begin;
    }
    return -1;
}

json_value *json_array_append_json_array(json_value *arr, json_value *other)
{
    int i, n;

    if (!other) return arr;
    assert(arr->type == json_array && other->type == json_array);

    n = other->u.array.length;
    other->u.array.length = 0;
    for (i = 0; i < n; i++) {
        if (!json_array_push(arr, other->u.array.values[i])) {
                return NULL;
        }
    }
    return arr;
}

json_value *do_obj_merge_arrays(json_value *a, json_value *b)
{
    int i;
    json_value *aval, *bval;
    char *bname;
    for (i = 0; i < b->u.object.length; i++) {
        bval = b->u.object.values[i].value;
        bname = b->u.object.values[i].name;
        if (bval->type == json_array) {
            if ((aval = json_object_find(a, bname)) &&
                    aval->type == json_array) {
                if (!json_array_append_json_array(aval, bval)) {
                    eprintf("array append failed\n");
                }
            }
        }
    }
    return a;
}

/* merge a bunch of objects */
json_value *merge(json_value *results[], int count)
{
    json_value *arr;
    int i;
    int n_unique = 0;

    if (!(arr = json_array_new(count))) {
        return NULL;
    }

    for (i = 0; i < count; ++i) {
        json_value **vals = arr->u.array.values;
        int idx = check_hash(vals, vals + n_unique, results[i]);
        if (idx >= 0) {
            if (!do_obj_merge_arrays(vals[idx], results[i])) {
                eprintf("Merge Failure!\n");
                goto err;
            }
        } else if (json_array_push(arr, results[i])) {
            n_unique++;
        } else {
            goto err;
        }
    }
    return arr;

 err:
    json_builder_free(arr);
    return NULL;
}

int run_pipeline()
{
    unsigned char *data;
    char *desc;
    work_ctx ctx = alloc_ctx();
    int64_t len, dlen;
    int i, count;
    json_value *cur;
    json_value *results[MAX_RESULTS];
    json_settings settings = {0};
    char error[128];
    int not_ready;

    settings.value_extra = json_builder_extra;
#ifdef __native_client__
    while (wait_for_chan(true, NULL, NULL, NULL, NULL, NULL) >= 0)
#else
    while(1)
#endif
    {
       count = 0;
       while ((not_ready = get_work_desc(ctx, &desc, &dlen, &data, &len)) >= 0) {
           if (not_ready) continue;
           if (!(cur = json_parse_ex(&settings, desc, len, error))) {
               fprintf(stderr, "JSON parsing failed!: %s\n%s\n", error, desc);
               fflush(stderr);
               return 1;
           }
           results[count] = cur;
           count ++;
           if (count > MAX_RESULTS) {
               eprintf("Get more results than we can hold....\n");
               return 1;
           }
           free(desc);
           free(data);
       }
       cur = merge(results, count);
       len = json_measure(cur);
       desc = malloc(len);
       json_serialize(desc, cur);
       json_builder_free(cur);
       // eprintf("Put desc: %s to next stage pipeline\n", desc);
       put_work_desc(ctx, desc, len, "", 1);
       free(desc);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    WorkSpec *spec;
    int ret;
    char* rest;

    spec = WorkSpec_parse(argv[argc - 1]);

    if (spec) {
        assert(spec->n > 0);
        if (setup_for_work(spec)) {
            fprintf(stderr, "setup failed");
            return 1;
        }
        ret = run_pipeline();
        finish_work(1);
    } else {
        fprintf(stderr, "No spec..\n");
        ret = 1;
    }

    return ret;
}
