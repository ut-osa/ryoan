#ifndef _TRUSTED_SERVICE_RUNTIME_KLARGEST_H_
#define _TRUSTED_SERVICE_RUNTIME_KLARGEST_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct sparse_grad {
	size_t i;
	size_t j;
	float abs_data;
	float data;
} sparse_grad;


sparse_grad kthLargest(sparse_grad *arr, int l, int r, int k);
void sp_swap(sparse_grad *a, sparse_grad *b);

#endif
