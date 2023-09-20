#include "buddy.h"
#include <stdio.h>

void bfree(void *ptr, buddy_allocator *alloc) {

    if (!ptr) return;

    buddy_block *bl = HEADER(ptr);
    printf("========\nTrying to free %ld (%d)\n", (void*)bl - alloc->memory_start, bl->order);
    buddy_block *buddy = get_buddy(alloc, bl);

    if (!buddy) {
        fprintf(stderr, "Failed to get buddy.\n");
        return;
    } else {
        printf("Buddy = %ld (%d)\n", (void*)buddy - alloc->memory_start, buddy->order);
    }

    if (buddy->order != bl->order || buddy->taken) {
        printf("Simply removing block, buddy is taken or different.\n");
        insert_block(alloc, bl);
    } else {
        // Iteratively merge as much as possible
        while (bl->order != alloc->max_order && !get_buddy(alloc, bl)->taken) {
            printf("Trying to merge %ld (%d)\n", (void*)bl - alloc->memory_start, bl->order);
            bl = merge(alloc, bl);
            if (!bl) { // Merge bl with buddy
                printf("Couldn't merge.\n");
                return;
            }
        }
        printf("Reached end of while loop in free.\n");
    }   

    for (int i = 0; i < BUDDY_NUM_LISTS; i++) {
        printf("\tList %d (order %d): %p\n", i, i + alloc->min_order, alloc->order_lists[i]);
    }    
}