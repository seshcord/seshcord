/*
 * Seshcord - Slightly improved database access routines
 *
 * Copyright (C) 2026 Nathan Roberts
 */

#include "dbexec.h"

/* The maximum length of a printed 64-bit number, plus nullterm */
#define MAXINTSTR 22

#define DECODE_ERR_MALLOC -1
#define DECODE_ERR_PLACEHOLDER -2
/*
 * Decode a format string for db_prep
 *
 * cmd: The query to decode, with printf-style placeholders
 * types_out: An array of the types that were decoded.
 * query_out: The translated query
 *
 * Return: The number of arguments found, or:
 *      DECODE_ERR_MALLOC: malloc() failure
 *      DECODE_ERR_PLACEHOLDER Invalid format string
 */
static int decode_args( char *cmd,
        enum db_param_types **types_out,
        char **query_out )
{
    /* Number of arguments found */
    int nargs = 0;
    /* Pointer into cmd */
    char *c;

    /* See how many placeholders we have */
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
                nargs++;
            }
        }
    }

    /* The types we found */
    enum db_param_types *types = malloc( sizeof( types ) * nargs );
    if( types == NULL ) return DECODE_ERR_MALLOC;

    /*
     * Calculate the approximate length of the translated query.
     *
     * This is approximately the length of the provided query: Each "%x"
     * translates to a "$n", unless there are more than 9 placeholders, in
     * which case a "%x" might translate into an "$nn". So we'll allocate the
     * length of the original, plus the number of placeholders. This should
     * give enough space for up to 99 arguments, plus some extra.
     */
    int i = sizeof( char ) * (strlen( cmd ) + nargs + 1);
    /* printf( "%i\n", i ); */

    /* The translated query */
    char *query = malloc( i );
    if( query == NULL )
    {
        free( types );
        return DECODE_ERR_MALLOC;
    }

    /* Pointer into query during population */
    char *q = query; 
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
                    types[i++] = INT;
                    break;
                case 'u':
                    types[i++] = UINT;
                    break;
                case 'f':
                case 'g':
                    types[i++] = DOUBLE;
                    break;
                case 's':
                    types[i++] = STRING;
                    break;
                case 'l':
                    c++;
                    switch( *c )
                    {
                        case 'i':
                        case 'd':
                            types[i++] = LONG;
                            break;
                        case 'u':
                            types[i++] = ULONG;
                            break;
                        case 'l':
                            c++;
                            switch( *c )
                            {
                                case 'i':
                                case 'd':
                                    types[i++] = LONGLONG;
                                    break;
                                case 'u':
                                    types[i++] = ULONGLONG;
                                    break;
                                default:
                                    free( types );
                                    free( query );
                                    return DECODE_ERR_PLACEHOLDER;
                            }
                            break;
                        default:
                            free( types );
                            free( query );
                            return DECODE_ERR_PLACEHOLDER;
                    }
                    break;
                case 'L':
                    c++;
                    switch( *c )
                    {
                        case 'f':
                        case 'g':
                            types[i++] = LDOUBLE;
                            break;
                        default:
                            free( types );
                            free( query );
                            return DECODE_ERR_PLACEHOLDER;
                    }
                    break;
                case '%':
                    /* We'll handle this below */
                    break;
                default:
                    free( types );
                    free( query );
                    return DECODE_ERR_PLACEHOLDER;
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
    *query_out = query;
    *types_out = types;
    return nargs;
}

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
 * contain the error:
 *  DB_BAD_CONN: Passed a null or invalid connection.
 *  DB_MALLOC_ERROR: Failed to malloc memory.
 *  DB_INVALID_PLACEHOLDER: The query had an invalid placeholder.
 *  DB_CONN_ERROR: PostgreSQL returned an error.
 *
 */
db_prepared *db_prep( PGconn *conn, char *name, char *cmd )
{
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

    prep->name = strdup( name );
    if( prep->name == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        return prep;
    }

    prep->nparams = decode_args( cmd, &prep->types, &prep->query );

    if( prep->nparams == DECODE_ERR_MALLOC )
    {
        prep->status = DB_MALLOC_ERROR;
        return prep;
    }
    if( prep->nparams == DECODE_ERR_PLACEHOLDER )
    {
        prep->status = DB_INVALID_PLACEHOLDER;
        return prep;
    }

    PGresult *res = PQprepare( conn, name,
            prep->query, prep->nparams, NULL );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK )
    {
        prep->status = DB_CONN_ERROR;
    }
    PQclear( res );
    return prep;
}

/*
 * Helper macro for convert_args. Converts the current argument (args[i] from
 * the caller), of the given type `f`, using the printf formatting code `f`,
 * store it in the target string (`c` from the caller), and break out of the
 * enclosing switch()
 */
#define procarg( f, t ) \
    args[i] = c; \
    c += sprintf( c, f, va_arg( vargs, t )); \
    break

/*
 * Convert a list of arguments to an array of strings.
 *
 * nparams: The number of arguments supplied
 * types: The types of the arguments, as returned by decode_args()
 * vargs: The arguments
 *
 * Return: An array of strings, or NULL if memory couldn't be allocated. This
 * pointer should be free()d when no longer needed (which also frees the string
 * data, as it is all allocated in a single chunk)
 */
static char **convert_args(
        int nparams, enum db_param_types *types, va_list vargs )
{
    /* Array of arguments */
    char **args;
    /* The size of that array */
    int asize = sizeof( char * ) * nparams;      
    /* Allocate one chunk for everything we need. Mainly because this will be
     * the return value, and can be freed by the caller. */
    void *chunk = malloc(
            /* The argument array */
            asize
            /* All the string storage we might need */
            + sizeof( char ) * MAXINTSTR * nparams
            );
    if( chunk == NULL ) return NULL;
    args = chunk;

    /* Convert all the given args into strings. */

    /* Pointer into string space as we use it */
    char *c = chunk + asize;
    int i; /* Loop index */
    for( int i = 0; i < nparams; i++ )
    {
        switch( types[i] )
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

    return args;
}

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
    va_list vargs;
    va_start( vargs, prep );

    /* Array of arguments */
    char **args = convert_args( prep->nparams, prep->types, vargs );
    va_end( vargs );
    if( args == NULL )
    {
        prep->status = DB_MALLOC_ERROR;
        return NULL;
    }

    PGresult *res = NULL;

    res = PQexecPrepared( prep->conn, prep->name, prep->nparams,
            (const char * const *) /* C sucks */ args,
            NULL, /* Parameter lengths not needed for string args */
            NULL, /* ...Likewise for formats */
            0 /* Results in text format. */ );
    free( args );
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

PGresult *db_exec_direct( PGconn *conn, char *cmd, ... )
{
    if( conn == NULL ) return NULL;
    enum db_param_types *types;
    char *query;
    int nparams = decode_args( cmd, &types, &query );

    if( nparams == DECODE_ERR_MALLOC ||
            nparams == DECODE_ERR_PLACEHOLDER )
    {
        return NULL;
    }

    va_list vargs;
    va_start( vargs, cmd );
    char **args = convert_args( nparams, types, vargs );
    va_end( vargs );
    if( args == NULL ) return NULL;

    PGresult *res = NULL;
    res = PQexecParams( conn, query, nparams,
            NULL, /* Parameter types: not needed for string args */
            (const char * const *) /* C sucks */ args,
            NULL, /* Parameter lengths: not needed for string args */
            NULL, /* ...Likewise for formats */
            0 /* Results in text format. */ );
    free( args );
    return res;
}
