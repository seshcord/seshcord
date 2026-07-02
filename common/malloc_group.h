#include <stdlib.h>

/* Malloc memory and keep track of it for easy freeing */

/* A group of memory chunks that have been malloced */
typedef struct malloc_group
{
    void **mallocs; /* The chunks malloced in this group */
    int max; /* The maximum numver of entries in this group */
    int count; /* The current number of entries in this group */
} malloc_group;

malloc_group *new_malloc_group( int );
void *new_malloc_entry( malloc_group *, size_t );
void free_malloc_group( malloc_group * );



