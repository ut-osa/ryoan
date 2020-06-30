#include "native_client/src/trusted/service_runtime/trusted_exec.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/parameter_server.h"
#include "native_client/src/trusted/service_runtime/sgx_perf.h"
#include "native_client/src/trusted/service_runtime/minheap.h"

#include <python3.5/Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"

typedef struct {
   union {
      int32_t(*one)(struct NaClAppThread *, uint32_t);
      int32_t(*two)(struct NaClAppThread *, uint32_t, uint32_t);
      int32_t(*three)(struct NaClAppThread *, uint32_t, uint32_t, uint32_t);
      int32_t(*four)(struct NaClAppThread *, uint32_t, uint32_t, uint32_t,
                     uint32_t);
   } op;
   unsigned nargs;
} ryoan_op_t;

static PyObject *theano_function;
static PyObject *sickle_loads;

struct network {
   PyObject *training_func;
   PyArrayObject **pyArray_refs;
   size_t nparams;
   struct param_set *raw_params;
   struct param_set *updates;
   struct param_set *last;
};

static struct network net;
static struct parameter_server *pserver;
static PyObject **shared;
static size_t num_params;

static PyObject *import_theano_function(void)
{
   PyObject *theano_module, *res;

   theano_module = PyImport_ImportModule("theano");
   if (!theano_module) {
      pyerror("THEANO load NOT successful: ");
      return NULL;
   }

   res = PyObject_GetAttrString(theano_module, "function");
   if (!res)
      pyerror("THEANO.function load NOT successful: ");

   Py_DECREF(theano_module);
   return res;
}

static void *init_numpy(void)
{
   import_array();
   return NULL;
}

static int init_python(void)
{
   char *old_pythonpath, *new_pythonpath;
   char buffer[1024];

   old_pythonpath = getenv("PYTHONPATH");

   if (old_pythonpath) {
      snprintf(buffer, 1024, THEANO_ROOT ":%s", old_pythonpath);
      new_pythonpath = buffer;
   } else
      new_pythonpath = THEANO_ROOT;

   if (setenv("PYTHONPATH", new_pythonpath, 1)) {
      te_err("setenv Failed!");
      return -1;
   }

	Py_Initialize();

   if (old_pythonpath) {
      if (setenv("PYTHONPATH", old_pythonpath, 1)) {
         te_err("setenv Failed!");
         return -1;
      }
   } else {
      if (unsetenv("PYTHONPATH")) {
         te_err("unsetenv Failed!");
         return -1;
      }
   }
   return 0;
}

static PyObject *import_sickle_loads(void)
{
   PyObject *sickle, *res;

   sickle = PyImport_ImportModule("sickle");
   if (!sickle) {
      pyerror("SICKLE load NOT successful: ");
      return NULL;
   }

   res = PyObject_GetAttrString(sickle, "loads");
   if (!res)
      pyerror("SICKLE.loads load NOT successful: ");

   Py_DECREF(sickle);
   return res;
}

static int fixup_multiarray(void)
{
   PyObject *numpy = NULL,
            *core = NULL,
            *multiarray = NULL,
            *sys = NULL,
            *modules_dict = NULL;
   int ret = -1;

   numpy = PyImport_ImportModule("numpy");
   if (!numpy) {
      pyerror("numpy load NOT successful: ");
      goto done;
   }

   core = PyObject_GetAttrString(numpy, "core");
   if (!core) {
      pyerror("core load NOT successful: ");
      goto done;
   }

   multiarray = PyObject_GetAttrString(core, "multiarray");
   if (!multiarray) {
      pyerror("multiarray load NOT successful: ");
      goto done;
   }

   sys = PyImport_ImportModule("sys");
   if (!sys) {
      pyerror("coudn't load sys: ");
      goto done;
   }

   modules_dict = PyObject_GetAttrString(sys, "modules");
   if (!modules_dict) {
      pyerror("couldn't get modules dict: ");
      goto done;
   }

   if (PyDict_SetItemString(modules_dict, "multiarray", multiarray)) {
      pyerror("failed to add multiarray alias ");
      goto done;
   }
   if (init_numpy()) {
      pyerror("failed to init numpy ");
      goto done;
   }

   ret = 0;

done:
   Py_XDECREF(multiarray);
   Py_XDECREF(sys);
   Py_XDECREF(modules_dict);

   return ret;
}

void NaClInitPython(char *parameter_server_string)
{
   if (load_compiler_module())
      te_err_fatal("loading ryoan python module failed\n");

   if (init_python())
      te_err_fatal("python init failed\n");

   theano_function = import_theano_function();
   if (!theano_function)
      te_err_fatal("Couldn't load theano_function\n");

   sickle_loads = import_sickle_loads();
   if (!sickle_loads)
      te_err_fatal("sickle_loads load NOT successful\n");

   if (fixup_multiarray())
      te_err_fatal("failed to fixup multiarray\n");

   if (parameter_server_string) {
      pserver = init_parameter_server(parameter_server_string);
      if (!pserver)
         te_err_fatal("failed to connect to parameter server\n");
   }
}

static PyObject *unsickle(char *buf, uint32_t size)
{
   PyObject *obj_sickle, *args, *obj;

   obj_sickle = PyUnicode_FromStringAndSize(buf, size);
   if (!obj_sickle) {
      pyerror("failed to load sickle in python runtime\n");
      return NULL;
   }

   args = PyTuple_Pack(1, obj_sickle);
   if (!args) {
      pyerror("failed to pack tuple\n");
      obj = NULL;
   } else {
      obj = PyObject_Call(sickle_loads, args, NULL);
      if (!obj)
         pyerror("failed to unsickle spec\n");
      Py_DECREF(args);
   }
   Py_DECREF(obj_sickle);

   return obj;
}

static PyArrayObject *param2array(PyObject *param)
{
   static PyObject *get_value_str;
   PyObject *tmp;
   PyArrayObject *value;

   if (!get_value_str)
      get_value_str = PyUnicode_FromString("get_value");

   tmp = PyObject_CallMethodObjArgs(param, get_value_str, Py_True, Py_True, NULL);
   if (!tmp) {
      pyerror("extracting value from param:");
      return NULL;
   }

   value = (PyArrayObject *)PyArray_EnsureArray(tmp);
   if (!value) {
      Py_DECREF(tmp);
      pyerror("Ensure ArrayObj: ");
   }

   return value;
}


int comp_params(const void *lhs, const void *rhs)
{
   char *left = ((struct param *)lhs)->py_str;
   char *right = ((struct param *)rhs)->py_str;

   return strcmp(left, right);
}


static int init_parameters(PyObject *shared_params)
{
   PyObject *set_value;
   PyArrayObject *value;
   PyObject *tmp;
   size_t len, i;
   int res = 0;

   set_value = PyUnicode_FromString("set_value");

   for (i = 0; i < net.nparams; i++) {
      PyObject *cursor = PySequence_ITEM(shared_params, i);
      if (!cursor) {
         pyerror("Problem getting element %ld from keys list\n", i);
         res = -1;
         break;
      }

      value = param2array(cursor);
      if (!value) {
         res = -1;
         break;
      }
      len = PyArray_NBYTES(value);

      net.pyArray_refs[i] = value;

      net.raw_params->data[i].count = len / sizeof(float);
      net.raw_params->data[i].raw = (float *)PyArray_DATA(value);
      net.raw_params->data[i].py_str = PyUnicode_AsUTF8AndSize(PyObject_Str(cursor), NULL);

      printf("Beginning, %ld, %p\n", i, (float *)PyArray_DATA(value));
      /* we want 'shared_params' to point to memory aliased by the
       * parameter objects Theano is using
       */
      tmp = PyObject_CallMethodObjArgs(cursor, set_value, value, Py_True, NULL);
      if (!tmp) {
         pyerror("Problem set value %ld from params\n", i);
         res = -1;
         break;
      }
   }

   //qsort(net.raw_params->data, net.nparams, sizeof(net.raw_params->data[0]), comp_params);

   for (i = 0; i < net.nparams; i++) {
      len = net.raw_params->data[i].count * sizeof(float);
      net.updates->data[i].py_str = net.raw_params->data[i].py_str;
      net.last->data[i].py_str = net.raw_params->data[i].py_str;
      net.updates->data[i].count = net.raw_params->data[i].count;
      net.updates->data[i].raw = malloc(len);
      net.last->data[i].count = net.raw_params->data[i].count;
      net.last->data[i].raw = malloc(len);
      if (!net.updates->data[i].raw) {
         Py_DECREF(value);
         te_err("No memory for raw_params\n");
         res = -1;
         break;
      }
   }

   Py_DECREF(set_value);
   return res;
}

static void free_network_fields(void)
{
   if (net.raw_params)
      free_param_set(net.raw_params);
   if (net.updates)
      free_param_set(net.updates);
   if (net.last)
      free_param_set(net.last);
   if (net.pyArray_refs)
      free(net.pyArray_refs);
}

static void print_shared_addr(void)
{
   size_t i;
   PyArrayObject *value;
   float *arr;
   for (i = 0; i < num_params; i++) {
      value = param2array(shared[i]);
      arr =  (float *)PyArray_DATA(value);
      printf("Shared param %lu in addr: %p\n", i, arr);
   }
}

static int alloc_network_fields(PyObject *shared_params)
{
   net.nparams = PySequence_Length(shared_params);
   num_params = net.nparams;
   net.raw_params = malloc_param_set(net.nparams);
   net.updates = malloc_param_set(net.nparams);
   net.last = malloc_param_set(net.nparams);
   net.pyArray_refs = malloc(sizeof(PyObject *) * net.nparams);

   if (net.nparams &&
         net.raw_params &&
         net.updates &&
         net.last &&
         net.pyArray_refs)
      return 0;
   free_network_fields();
   return-1;
}

static int initialize_global_params(PyObject *updates)
{
   PyObject *keys;
   
   keys = PyMapping_Keys(updates);
   if (!keys) {
      pyerror("Failed to get keys from updates\n");
      return -1;
   }

   if (alloc_network_fields(keys)) {
      te_err("Out of memory!\n");
      return -1;
   }

   if (init_parameters(keys))
      goto err;

   if (recv_parameters(pserver, net.raw_params)){
	  printf("Receive failed, send params...\n");
      if (send_parameters(pserver, net.raw_params))
         goto err;
   }

   return 0;

err:
   free_network_fields();
   return -1;
}

static int share_global_params(void)
{
   if (do_param_diff(net.updates, net.raw_params, net.last)) {
      te_err("Failed to calculate parameter updates\n");
      return -1;
   }
   return send_parameter_updates(pserver, net.updates);
}

static
int32_t ryoan_op_build(struct NaClAppThread *natp, uint32_t buf, uint32_t size)
{
   struct NaClApp *nap = natp->nap;
   uintptr_t sysaddr;
   PyObject *kwargs, *pos_arg;
   int32_t ret = 0;
   PyObject *updates = NULL;
   PyObject *keys;
   size_t i;

   sysaddr = NaClUserToSysAddrRange(nap, buf, size);

   if (!theano_function) {
      te_err("tried to use Theano without loading\n");
      return -1;
   }

   if (size > INT32_MAX)
      size = INT32_MAX;

   kwargs = unsickle((char *)sysaddr, size);
   if (!kwargs)
      return -EINVAL;

   pos_arg = PyTuple_Pack(0);
   if (!pos_arg) {
      ret = -ENOMEM;
      goto done;
   }

   net.training_func = PyObject_Call(theano_function, pos_arg, kwargs);
   if (!net.training_func)
      pyerror_fatal("failed to build network training function\n");

   if (nap->sgx_perf)
      sgx_perf_enable();

   Py_DECREF(pos_arg);
   
      /* Tell the  parameter server about our initial parameters */
   updates = PyMapping_GetItemString(kwargs, "updates");
   if (!updates) {
      pyerror("Failed to get \"updates\" kwarg\n");
      goto done;
   }
   if (pserver && initialize_global_params(updates))
      goto done;

   if (!shared){
        printf("Initialize shared params.\n");
        keys = PyMapping_Keys(updates);
        num_params = PySequence_Length(keys);
        shared = malloc(sizeof(PyObject *) * num_params);
        for(i = 0; i < num_params; i++)
            shared[i] = PySequence_ITEM(keys, i);
   }
   print_shared_addr();

done:
   Py_XDECREF(kwargs);
   Py_XDECREF(updates);
   Py_XDECREF(keys);
   return ret;
}

static PyObject *desc2PyArray(struct NaClApp *nap, uint32_t _desc)
{
   struct arr_desc *desc;
   void *data;
   uint32_t *_dims;
   unsigned i;
   int type;
   npy_intp *dims, *strides;
   PyObject* ret = NULL;

   desc = (struct arr_desc *)NaClUserToSysAddrRange(nap, _desc, sizeof(*desc));
   if (kNaClBadAddress == (uintptr_t)desc) {
      te_err("arr_desc object bad address");
      return NULL;
   }

   _dims = (uint32_t *)NaClUserToSysAddrRange(nap, desc->dims, desc->dims_size);
   if (kNaClBadAddress == (uintptr_t)_dims) {
      te_err("dims object bad address\n");
      return NULL;
   }

   dims = malloc(sizeof(npy_intp) * desc->nd);
   if (!dims) {
      te_err("Malloc Failed! (%s:%d)\n", __FILE__, __LINE__);
      return NULL;
   }

   for (i = 0; i < desc->nd; ++i)
      dims[i] = _dims[i];

   strides = (void *)NaClUserToSysAddrRange(nap, desc->strides,
                                            desc->strides_size);
   if (kNaClBadAddress == (uintptr_t)strides) {
      te_err("strides object bad address\n");
      goto done;
   }


   data = (void *)NaClUserToSysAddrRange(nap, desc->data, desc->data_size);
   if (kNaClBadAddress == (uintptr_t)data) {
      te_err("data object bad address\n");
      goto done;
   }

   type = desc->type_num;
   if (desc->itemsize == 4 && desc->type_num == NPY_INT64)
      type = NPY_INT32;
   ret = PyArray_New(&PyArray_Type, desc->nd, dims, type, NULL, data,
                     desc->itemsize, desc->flags, NULL);
done:
   free(dims);
   return ret;
}

static int save_trained_params(void) {
   size_t i, j, len;
   FILE * fout;
   PyArrayObject *value;
   float *arr;

   printf("Open file...\n");
   fout = fopen("/home/song/ryoan/naclports/examples/params.txt", "w+");
   printf("Print %lu params to files...\n", num_params);

   for (i = 0; i < num_params; i++) {
      value = param2array(shared[i]);
      len = PyArray_NBYTES(value);
      arr =  (float *)PyArray_DATA(value);
      printf("%lu, %p\n", i, arr);
   	  fprintf(fout, "%s:\t", PyUnicode_AsUTF8AndSize(PyObject_Str(shared[i]), NULL));
      for (j = 0; j < len / sizeof(float); j++){
		 fprintf(fout, "%.9g\t", arr[j]);
	  }
	  fprintf(fout, "\n");
   }
   fclose(fout);

   if (pserver){
       printf("Open file...\n");
       fout = fopen("/home/song/ryoan/naclports/examples/raw_params.txt", "w+");
       printf("Print %lu params to files...\n", num_params);
       for (i = 0; i < num_params; i++) {
          arr =  net.raw_params->data[i].raw;
          printf("%lu, %p\n", i, arr);
          fprintf(fout, "%s:\t", net.raw_params->data[i].py_str);
          for (j = 0; j < net.raw_params->data[i].count; j++){
             fprintf(fout, "%.9g\t", arr[j]);
          }
          fprintf(fout, "\n");
       }
   }
   fclose(fout);

   return 0;
}


static
int32_t ryoan_op_train(struct NaClAppThread *natp, uint32_t data_desc,
                       uint32_t target_desc, uint32_t epochs)
{
   struct NaClApp *nap = natp->nap;
   PyObject *data = NULL, *target = NULL;
   PyObject *result = NULL;
   double res;
   size_t i;
   int ret;

   if (!net.training_func) {
      te_err("Network not defined yet!\n");
      return -EINVAL;
   }

   data = desc2PyArray(nap, data_desc);
   if (!data) {
      pyerror("Failed getting data");
      return -EINVAL;
   }

   target = desc2PyArray(nap, target_desc);
   if (!target) {
      pyerror("Failed getting target");
      Py_DECREF(data);
      return -EINVAL;
   }
   
   if (pserver) {
	 if (recv_parameters(pserver, net.raw_params))
		te_err("Failed to load_global_params\n");
	 for (i = 0; i < net.nparams; i++)
	   memcpy(net.last->data[i].raw, net.raw_params->data[i].raw,
			  net.last->data[i].count * sizeof(float));
   }
   
    if (epochs == 0){
        save_trained_params();
    } else {
        result = PyObject_CallFunctionObjArgs(net.training_func, data, target, NULL);

        if (!result)
            pyerror_fatal("At epoch %u no result: ", epochs);
        else {
            res = PyFloat_AsDouble(result);
            Py_DECREF(result);
        }
        if (pserver) {
            if (share_global_params())
                te_err("Failed to update_global_params\n");
        }
        printf("At epoch %u, loss %f\n", epochs, res);
        print_shared_addr();
        sgx_perf_print();
    }

   ret = 0;
   Py_DECREF(target);
   Py_DECREF(data);
   return ret;
}


static
const ryoan_op_t ops[RYOAN_MAX] = {
   {.op.two = ryoan_op_build,
    .nargs = 2,
   },
   {.op.three = ryoan_op_train,
    .nargs = 3,
   },
};

int32_t NaClSysTrustedExec(struct NaClAppThread *natp, uint32_t op,
                           uint32_t arg1, uint32_t arg2, uint32_t arg3,
                           uint32_t arg4)
{
   int32_t ret = -EINVAL;
   const ryoan_op_t *op_struct;

   assert(op < RYOAN_MAX);
   op_struct = &ops[op];
   switch (op_struct->nargs) {
   case 1:
      ret = op_struct->op.one(natp, arg1);
      break;
   case 2:
      ret = op_struct->op.two(natp, arg1, arg2);
      break;
   case 3:
      ret = op_struct->op.three(natp, arg1, arg2, arg3);
      break;
   case 4:
      ret = op_struct->op.four(natp, arg1, arg2, arg3, arg4);
      break;
   default:
      assert(false && "Too many arguments");
   }

   return ret;
}
