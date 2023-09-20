#include "buddy.h"
#include <stdio.h>

void *balloc(size_t size, buddy_allocator *alloc) {
    //printf("balloc(%ld);\n", size);
    if (size > alloc->size) {
        return NULL;
        // TODO: Could allocate more memory for the buddy allocator here.
    }

    if (size <= 0) {
        return NULL;
    }

    size += sizeof(buddy_block);
    size = size == 1 ? 1 : 1 << (64 - __builtin_clz(size-1)); // Round up to next power of 2

    if (size < 1 << alloc->min_order) size = 1 << alloc->min_order;

    unsigned short order = __builtin_ctz(size); // Log2 of a power of 2 is the amount of trailing 0's

    printf("Effectively allocating %ld bytes (which is order %d)\n", size, order);

    buddy_block *ret_block;

    if (alloc->order_lists[order - alloc->min_order]) {
        ret_block = remove_block(alloc, alloc->order_lists[order - alloc->min_order]);
        goto ret;
    } else {
        for (int n = order + 1; n <= alloc->max_order; n++) {
            ret_block = alloc->order_lists[n - alloc->min_order];

            //printf("\t -> Checking order %d (index %d) list at %p\n", n, n - alloc.min_order, ret_block);

            if (ret_block) {
                // Split block of order n down to order `order`.
                int splits = n - order;

                //printf("\tSplitting %d times.\n", splits);

                for (int s = 0; s < splits; s++) {
                    split(alloc, ret_block);
                }
                
                goto ret;
            }
        }

        ret_block = NULL;
    }

ret:
    //printf(":) Allocated block starts at %p\n", ret_block);
    remove_block(alloc, ret_block);
    for (int i = 0; i < BUDDY_NUM_LISTS; i++) {
        printf("\tList %d (order %d): ", i, i + alloc->min_order);
        for (buddy_block *b = alloc->order_lists[i]; b; b = b->next) {
            printf("%ld, ", (void*)b - alloc->memory_start);
        }
        printf("\n");
    }


    return &ret_block[1];
}