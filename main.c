#include <stdio.h>

#include <stdlib.h>
#include <stdint.h>

// Total amount of memory supported is 2^(min_order + BUDDY_NUM_LISTS)
// If an allocator is initialised with min_order=5 (32 bytes min allocation block),
// then the total memory possible to be managed is 128 Gbytes.
#define BUDDY_NUM_LISTS 32 

typedef struct buddy_block {
    unsigned short int order;
    unsigned short int taken;

    struct buddy_block *next;
    struct buddy_block *prev;
} buddy_block;

typedef struct buddy_allocator {
    unsigned short int min_order;
    unsigned short int max_order;

    size_t size;
    void *memory_start;

    buddy_block *order_lists[BUDDY_NUM_LISTS];
} buddy_allocator;

#define DATA(blkptr) ((void*)(blkptr[1]))
#define HEADER(dataptr) ((buddy_block *)((void*)dataptr - sizeof(buddy_block)))

buddy_allocator alloc;

// Remove a block from whichever list it's in
// Before:  ... A <-> bl <-> B ...
// After:   ... A <--------> B ...
void *remove_block(buddy_allocator *alloc, buddy_block *bl) {
    buddy_block *left = bl->prev, *right = bl->next;

    if (left) left->next = right;
    if (right) right->prev = left;

    if (!left) alloc->order_lists[bl->order - alloc->min_order] = right;

    bl->taken = 1; // Blocks are removed from the list when they are taken

    return bl;
}

// Insert a block at the head of its correct list
// Before:  NULL <-> A ...
// After:   NULL <-> bl <-> A ...
 void insert_block(buddy_allocator *alloc, buddy_block *bl) {
    buddy_block *head = alloc->order_lists[bl->order - alloc->min_order];

    bl->prev = NULL;
    bl->next = head;
    bl->taken = 0; // Blocks in the list are free ones

    if (head) head->prev = bl;
    
    alloc->order_lists[bl->order - alloc->min_order] = bl;
}

int buddy_init(size_t size, size_t min_order, buddy_allocator *alloc) {
    if ((size & (size-1)) != 0) {
        printf("Must be a power of 2.\n");
        return -1;
    }

    buddy_block *root_block = aligned_alloc(size, size);

    if (!root_block) {
        printf("Couldn't allocate memory for allocator.\n");
        return -1;
    }

    for (int i = 0; i < BUDDY_NUM_LISTS; i++) {
        alloc->order_lists[i] = NULL;
    }

    alloc->min_order = min_order;
    alloc->max_order = __builtin_ctz(size);
    alloc->size = size;
    alloc->memory_start = root_block;

    root_block->order = alloc->max_order;
    insert_block(alloc, root_block);

    printf("Min order: %d\tMax order: %d\tSize: %ld\tMemory: %p\n",
        alloc->min_order, alloc->max_order, alloc->size, root_block);

    return 0;
}

buddy_block *get_buddy(buddy_allocator *alloc, buddy_block *bl) {
    buddy_block *buddy;
    size_t mask;

    if (bl->order == alloc->max_order) return NULL;

    mask = 1 << bl->order;
    buddy = (buddy_block *)((size_t)bl ^ mask);

    return buddy;
}

buddy_block *split(buddy_allocator *alloc, buddy_block *bl) {
    buddy_block *br;

    if (bl->order - 1 < alloc->min_order) {
        return NULL; // Splitting block too small
    }

    remove_block(alloc, bl);

    bl->order--;

    br = get_buddy(alloc, bl);
    br->order = bl->order;

    insert_block(alloc, bl);
    insert_block(alloc, br);

    return br;
}

buddy_block *merge(buddy_allocator *alloc, buddy_block *bl) {
    buddy_block *left_buddy;

    if (bl->order == alloc->max_order) return NULL;

    left_buddy = (buddy_block *)((size_t)bl & ~(1 << bl->order)); // Clear bit that separates the two buddies from each other

    remove_block(alloc, left_buddy);
    remove_block(alloc, get_buddy(alloc, left_buddy));

    left_buddy->order++;

    insert_block(alloc, left_buddy);

    return left_buddy;
}

void *balloc(size_t size) {
    //printf("balloc(%ld);\n", size);
    if (size > alloc.size) {
        return NULL;
        // TODO: Could allocate more memory for the buddy allocator here.
    }

    if (size <= 0) {
        return NULL;
    }

    size += sizeof(buddy_block);
    size = size == 1 ? 1 : 1 << (64 - __builtin_clz(size-1)); // Round up to next power of 2

    if (size < 1 << alloc.min_order) size = 1 << alloc.min_order;

    unsigned short order = __builtin_ctz(size); // Log2 of a power of 2 is the amount of trailing 0's

    printf("Effectively allocating %ld bytes (which is order %d)\n", size, order);

    buddy_block *ret_block;

    if (alloc.order_lists[order - alloc.min_order]) {
        ret_block = remove_block(&alloc, alloc.order_lists[order - alloc.min_order]);
        goto ret;
    } else {
        for (int n = order + 1; n <= alloc.max_order; n++) {
            ret_block = alloc.order_lists[n - alloc.min_order];

            //printf("\t -> Checking order %d (index %d) list at %p\n", n, n - alloc.min_order, ret_block);

            if (ret_block) {
                // Split block of order n down to order `order`.
                int splits = n - order;

                //printf("\tSplitting %d times.\n", splits);

                for (int s = 0; s < splits; s++) {
                    split(&alloc, ret_block);
                }
                
                goto ret;
            }
        }

        ret_block = NULL;
    }

ret:
    //printf(":) Allocated block starts at %p\n", ret_block);
    return &ret_block[1];
}

void bfree(void *ptr) {
    if (!ptr) return;

    buddy_block *bl = HEADER(ptr);
    buddy_block *buddy = get_buddy(&alloc, bl);

    if (buddy->taken) {
        remove_block(&alloc, bl);
    } else {
        // Iteratively merge as much as possible
        while (bl->order != alloc.max_order && !get_buddy(&alloc, bl)->taken)
            merge(&alloc, bl); // Merge bl with buddy
    }
}

int blocks_since_last_of_order(buddy_allocator *alloc, buddy_block *bl) {
    void *empty_start;

    if (!bl->prev) 
        empty_start = alloc->memory_start;
    else
        empty_start = ((void*)bl->prev) + (1 << bl->order);

    return ((void*)bl - empty_start) / (1 << bl->order);
}

int main() {
    buddy_init(32768, 5, &alloc);

    void *ptr1 = balloc(200);
    bfree(ptr1);

    free(alloc.memory_start);
}