#include "native_client/src/trusted/service_runtime/maxheap.h"

#define LCHILD(x) 2 * x + 1
#define RCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

/*
    Heapify function is used to make sure that the heap property is never violated
    In case of deletion of a node, or creating a min heap from an array, heap property
    may be violated. In such cases, heapify function can be called to make sure that
    heap property is never violated
*/
void heapify(maxHeap *hp, size_t i) {
    size_t largest = (LCHILD(i) < hp->size && hp->elem[LCHILD(i)].abs_data > hp->elem[i].abs_data) ? LCHILD(i) : i;

    if(RCHILD(i) < hp->size && hp->elem[RCHILD(i)].abs_data > hp->elem[largest].abs_data)
        largest = RCHILD(i);
 
    if(largest != i) {
        sp_swap(&(hp->elem[i]), &(hp->elem[largest])) ;
        heapify(hp, largest);
    }
}


void buildMaxHeap(maxHeap *hp, sparse_grad *arr, size_t size) {
	size_t i;
	hp->elem = arr;
	hp->size = size;

	// Making sure that heap property is also satisfied
	for(i = (hp->size - 1) / 2; i >= 1; i--)
		heapify(hp, i);

	heapify(hp, 0);
}


sparse_grad extractMax(maxHeap *hp)
{
	sparse_grad sp_grad;
	sp_grad.data = -1;
	sp_grad.abs_data = -1;
	sp_grad.i = 0;
	sp_grad.j = 0;

    if (hp->size == 0)
        return sp_grad;
 
    // Store the maximum vakue.
    sp_grad = hp->elem[0];
 
    // If there are more than 1 items, move the last item to root
    // and call heapify.
    if (hp->size > 1) {
        hp->elem[0] = hp->elem[hp->size-1];
        heapify(hp, 0);
    }

    hp->size--;
    return sp_grad;
}

