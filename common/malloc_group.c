#include <stdio.h>

#include "malloc_group.h"

/*
 * Create a new malloc group.
 *
 * max: The number of entries the group can hold. The `mallocs` array is
 * dynamically allocated based on this argument.
 *
 * return: A pointer to the new group, or NULL if an error occurrs.
 */
malloc_group *new_malloc_group( int max )
{
    malloc_group *group = malloc( sizeof( malloc_group ));
    if( group == NULL ) return NULL;
    /* fprintf( stderr, "Creating a new malloc group at %p\n", group ); */

    group->mallocs = malloc( sizeof( void * ) * max );
    if( group->mallocs == NULL ) return NULL;
    /* fprintf( stderr, "Creating a malloc list at %p\n", group->mallocs ); */

    group->max = max;
    group->count = 0;

    return group;
}

/*
 * Malloc a memory chunk and track it in the group
 *
 * group: The group tracking the malloc
 * size: The size of memory to allocate
 *
 * return: The malloced memory, or NULL if an error occurrs. This may happen
 * if either the malloc itself fails, or if the group is full.
 */
void *new_malloc_entry( malloc_group *group, size_t size )
{
    if( group->count >= group->max ) return NULL;
    void *item = malloc( size );
    if( item == NULL ) return NULL;
    /* fprintf( stderr, "Creating a new malloc entry at %p\n", item ); */
    group->mallocs[group->count++] = item;
    return item;
}

/*
 * Free a malloc group and everything it's tracking.
 *
 * This will free all mallocs tracked by the group, the array that was
 * created to hold the pointers, and the group itself.
 *
 * group: The group to free
 */
void free_malloc_group( malloc_group *group )
{
    int i;
    for( i = 0; i < group->count; i++ )
    {
        /* fprintf( stderr, "Freeing a malloc at %p\n", group->mallocs[i] ); */
        free( group->mallocs[i] );
    }
    /* fprintf( stderr, "Freeing a malloc list at %p\n", group->mallocs ); */
    free( group->mallocs );
    /* fprintf( stderr, "Freeing a malloc group at %p\n", group ); */
    free( group );

    return;
}




