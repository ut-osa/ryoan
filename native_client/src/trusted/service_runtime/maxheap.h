#ifndef _TRUSTED_SERVICE_RUNTIME_MINHEAP_H_
#define _TRUSTED_SERVICE_RUNTIME_MINHEAP_H_

#include <stdio.h>
#include <stdlib.h>
#include "native_client/src/trusted/service_runtime/klargest.h"

typedef struct maxHeap {
	size_t size;
	sparse_grad *elem;
} maxHeap;


void buildMaxHeap(maxHeap *hp, sparse_grad *arr, size_t size);
sparse_grad extractMax(maxHeap *hp);

#endif
