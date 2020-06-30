#include "native_client/src/trusted/service_runtime/klargest.h"

void sp_swap(sparse_grad *a, sparse_grad *b)
{
    sparse_grad temp = *a;
    *a = *b;
    *b = temp;
}
 
// Standard partition process of QuickSort().  It considers the last
// element as pivot and moves all smaller element to left of it and
// greater elements to right. This function is used by randomPartition()
int partition(sparse_grad *arr, int l, int r)
{
    sparse_grad x = arr[r];
	 int j, i = l;
    for (j = l; j <= r - 1; j++) {
        if (arr[j].abs_data >= x.abs_data) {
            sp_swap(&arr[i], &arr[j]);
            i++;
        }
    }
    sp_swap(&arr[i], &arr[r]);
    return i;
}
 
// Picks a random pivot element between l and r and partitions
// arr[l..r] arount the randomly picked element using partition()
int randomPartition(sparse_grad *arr, int l, int r)
{
    int n = r - l + 1;
    int pivot = rand() % n;
    sp_swap(&arr[l + pivot], &arr[r]);
    return partition(arr, l, r);
}

sparse_grad kthLargest(sparse_grad *arr, int l, int r, int k)
{
	int pos;
	sparse_grad rtn;
	rtn.data = -1;
	rtn.abs_data = -1;
	rtn.i = 0;
	rtn.j = 0;
    // If k is smaller than number of elements in array
    if (k > 0 && k <= r - l + 1) {
        // Partition the array around a random element and
        // get position of pivot element in sorted array
        pos = randomPartition(arr, l, r);
 
        // If position is same as k
        if (pos-l == k-1)
            return arr[pos];
        if (pos-l > k-1)  // If position is more, recur for left subarray
            return kthLargest(arr, l, pos-1, k);
 
        // Else recur for right subarray
        return kthLargest(arr, pos+1, r, k-pos+l-1);
    }
 
    // If k is more than number of elements in array
    return rtn;
}

