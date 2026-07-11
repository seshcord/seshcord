/*
 * Seshcord - Slightly improved database access routines
 *
 * Copyright (C) 2026 Nathan Roberts
 */

#include "dbexec.h"

/* The maximum length of a printed 64-bit number, plus nullterm */
#define MAXINTSTR 22

/*
 * Create a prepared SQL statement for use later.
 *
 * This is a frontend to PQprepare(), which takes the given name and
 * command, creates a prepared statement, and returns a structure which can
 * be used to easily execute the prepared statement with arguments later.
 * This structure is dynamically allocated, and must be freed with
 * db_free_prepped when no longer needed (including if an error occurrs.)
 *
 * This function accepts a command with printf-style placeholders, which is
 * converted into the appropriate format for PostgreSQL, and also makes note
 * of the types specified for later use. When the statement is executed,
 * only the structure and the arguments need to be provided, with the
 * arguments provided matching the placeholderes given in `cmd`,
 * printf-style.
 *
 * Supported placeholders:
 * %i, %d    int
 * %u        unsigned int
 * %li, %ld  long
 * %lu       unsigned long
 * %lli, %ld long long
 * %llu      unsigned long long
 * %f, %g    double
 * %Lf, %Lg  long double
 * %s        C-style string
 * %%        A literal %
 *
 * conn: The PostgreSQL connection
 * name: The name of the prepared statement to create
 * cmd: The command to prepare, with placeholders
 *
 * return: A new dynamically allocated db_prepared structure, or NULL if one
 * couldn't be allocated. If an error occurred, the `status` member will
 * contain the error.
 */

db_prepared *db_prep( PGconn *conn, char *name, char *cmd )
{
    int i;      /* Generic loop index */
    char *c;    /* Generic string loop pointer */
    int count;  /* Generic counter */
    char *query; /* Translated query */

    db_prepared *prep = malloc( sizeof( db_prepared ));
    if( prep == NULL ) return NULL;
    prep->conn = NULL;
    prep->name = NULL;
    prep->query = NULL;
    prep->nparams = 0;
    prep->types = NULL;
    prep->status = DB_OK;

    if( conn == NULL )
    {
        prep->status = DB_BAD_CONN;
        return prep;
    }
    prep->conn = conn;

    prep->name = malloc( sizeof( char ) * (strlen( name ) + 1) );
    if( prep->name == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        return prep;
    }
    strcpy( prep->name, name );

    /* See how many placeholders we have */
    count = 0;
    for( c = cmd; *c; c++ )
    {
        if( *c == '%' )
        {
            if( c[1] == '%' )
            {
                /* Ignore literal %'s (and bypass them) */
                c++;
            }
            else if( c[1] )
            {
                /* Also ignore %'s at the end of a string */
                count++;
            }
        }
    }

    prep->types = malloc( sizeof( prep->types ) * count );
    if( prep->types == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        return prep;
    }

    prep->nparams = count;

    /*
     * Calculate the approximate length of the translated query.
     *
     * This is approximately the length of the provided query: Each "%x"
     * translates to a "$n", unless there are more than 9 placeholders, in
     * which case a "%x" might translate into an "$nn". So we'll allocate the
     * length of the original, plus the number of placeholders. This should
     * give enough space for up to 99 arguments, plus some extra.
     */
    i = sizeof( char ) * (strlen( cmd ) + count + 1);
    /* printf( "%i\n", i ); */
    query = malloc( i );
    if( query == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        return prep;
    }
    prep->query = query;

    char *q = query; /* Pointer into query during population */
    i = 0; /* Placeholder index/count */
    for( c = cmd; *c; c++ )
    {
        if( *c == '%' )
        {
            c++;
            switch( *c )
            {
                case 'i':
                case 'd':
                    prep->types[i++] = INT;
                    break;
                case 'u':
                    prep->types[i++] = UINT;
                    break;
                case 'f':
                case 'g':
                    prep->types[i++] = DOUBLE;
                    break;
                case 's':
                    prep->types[i++] = STRING;
                    break;
                case 'l':
                    c++;
                    switch( *c )
                    {
                        case 'i':
                        case 'd':
                            prep->types[i++] = LONG;
                            break;
                        case 'u':
                            prep->types[i++] = ULONG;
                            break;
                        case 'l':
                            c++;
                            switch( *c )
                            {
                                case 'i':
                                case 'd':
                                    prep->types[i++] = LONGLONG;
                                    break;
                                case 'u':
                                    prep->types[i++] = ULONGLONG;
                                    break;
                                default:
                                    prep->status = DB_INVALID_PLACEHOLDER;
                                    free( query );
                                    return prep;
                            }
                            break;
                        default:
                            prep->status = DB_INVALID_PLACEHOLDER;
                            free( query );
                            return prep;
                    }
                    break;
                case 'L':
                    c++;
                    switch( *c )
                    {
                        case 'f':
                        case 'g':
                            prep->types[i++] = LDOUBLE;
                            break;
                        default:
                            prep->status = DB_INVALID_PLACEHOLDER;
                            free( query );
                            return prep;
                    }
                    break;
                case '%':
                    /* We'll handle this below */
                    break;
                default:
                    prep->status = DB_INVALID_PLACEHOLDER;
                    free( query );
                    return prep;
            }
            if( *c == '%' )
            {
                *q++ = '%';
            }
            else
            {
                q += sprintf( q, "$%i", i );
            }
        }
        else
        {
            *q++ = *c;
        }
    }
    *q = 0;

    PGresult *res = PQprepare( conn, name, query, prep->nparams, NULL );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK ) prep->status = DB_CONN_ERROR;
    PQclear( res );
    return prep;
}

/*
 * Helper macro for db_exec. Converts the current argument (args[i] from the
 * caller), of the given type `f`, using the printf formatting code `f`,
 * store it in the target string (`c` from the caller), and break out of the
 * enclosing switch()
 */
#define procarg( f, t ) args[i] = c; \
    c += sprintf( c, f, va_arg( vargs, t )); \
    break

/*
 * Call a prepared statement returned by db_prep
 *
 * This function accepts the arguments described by the `cmd` argument to
 * db_prep(), and executes the command.
 *
 * Note that this returns results in text format.
 */
PGresult *db_exec( db_prepared *prep, ... )
{
    char **args; /* Array of arguments */
    args = malloc( sizeof( char * ) * prep->nparams );
    if( args == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        return NULL;
    }

    char *argstr; /* Storage for actual argument strings */
    /* Allocate one chunk of memory for all the numeric strings we might
     * possibly need. */
    argstr = malloc( sizeof( char ) * MAXINTSTR * prep->nparams );
    if( argstr == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        free( args );
        return NULL;
    }

    /* Convert all the given args into strings. */

    char *c = argstr; /* Pointer into `argstr` as we use it */
    int i; /* Loop index */
    va_list vargs;
    va_start( vargs, prep );
    for( int i = 0; i < prep->nparams; i++ )
    {
        switch( prep->types[i] )
        {
            case CHAR:
                /* We're not actually using this as it turns out */
                break;
            case SHORT:
            case USHORT:
                /* We're also not using this because shorts automatically
                 * get promoted to ints */
                break;
            case FLOAT:
                /* Similarly, we're not using this because floats
                 * automatically get promoted to doubles */
                break;

            /* Now for the types we're *actually* using. */

            case STRING:
                /* If it's a string, just shovel in what we were given */
                args[i] = va_arg( vargs, char * );
                break;

            /* For integer arguments, convert them to strings and add the
             * result (using the procargs macro) */
            case INT: procarg( "%i", int );
            case UINT: procarg( "%u", unsigned int );
            case LONG: procarg( "%li", long );
            case ULONG: procarg( "%lu", unsigned long );
            case LONGLONG: procarg( "%lli", long long );
            case ULONGLONG: procarg( "%llu", unsigned long long );
            case DOUBLE: procarg( "%f", double );
            case LDOUBLE: procarg( "%Lf", long double );
        }
    }
    va_end( vargs );

    PGresult *res = NULL;

    res = PQexecPrepared( prep->conn, prep->name, prep->nparams,
            (const char * const *) /* C sucks */ args,
            NULL, /* Parameter lengths not needed for string args */
            NULL, /* ...Likewise for formats */
            0 /* Results in text format. */ );
    free( args );
    free( argstr );
    return res;
}

/* 
 * Free a prepared statement created by db_prep().
 *
 * This frees all mallocated memory, and is safe to use even if the structure
 * was only partially initialized due to an error. Note that it does *not*
 * remove the prepared statement on the server.
 */

void db_free_prepped( db_prepared *prep )
{
    if( prep == NULL ) return;

    if( prep->name != NULL ) free( prep->name );
    if( prep->query != NULL ) free( prep->query );
    if( prep->types != NULL ) free( prep->types );
    
    free( prep );
}
