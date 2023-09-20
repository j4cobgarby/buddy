#include "buddy.h"
#include <stdio.h>
#include <string.h>

static buddy_allocator alloc;

#define alloc(size) balloc(size, &alloc)
#define release(ptr) bfree(ptr, &alloc)

int main() {
    buddy_init(16384, 5, &alloc);

    char *s1 = alloc(500);
    char *s2 = alloc(1);
    char *s3 = alloc(4000);
    char *s4 = alloc(2310);

    printf("Allocated %p, %p\n", s1, s2);

    release(s2);
    release(s1);
    release(s4);
    release(s3);
}