#ifndef _NACL_RYOAN_OPS_H
#define _NACL_RYOAN_OPS_H

enum  {
   RYOAN_OP_DECAY,
   RYOAN_OP_BUILD,
   RYOAN_OP_TEST,
   RYOAN_OP_TRAIN,
   RYOAN_MAX
};

struct arr_desc {
   uint32_t nd;
   uint64_t type_num;
   uint64_t itemsize;
   uint64_t flags;
   /* pointer */
   uint32_t strides;
   uint32_t strides_size;
   /* pointer */
   uint32_t dims;
   uint32_t dims_size;
   /* pointer */
   uint32_t data;
   uint32_t data_size;
};

int ryoan_trusted_op(int op, void *arg1, void *arg2, void *arg3, void *arg4);

#endif
