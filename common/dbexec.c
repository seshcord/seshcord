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
/* static */ int decode_args( char *cmd,
        enum db_param_types **types_out,
        enum db_param_types **otypes_out,
        int *oargs_out,
        char **query_out )
{
    /* Number of arguments found */
    int nargs = 0;
    int oargs = 0;
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
            else if( c[1] == '-' )
            {
                /* An output argument */
                oargs++;
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
    enum db_param_types *otypes = malloc( sizeof( types ) * oargs );
    if( types == NULL || otypes == NULL )
    {
        free( types );
        free( otypes );
        return DECODE_ERR_MALLOC;
    }

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
        free( otypes );
        return DECODE_ERR_MALLOC;
    }

    /* Pointer into query during population */
    char *q = query; 
    /* Placeholder index/count */
    i = 0; 
    int o = 0;

    /* Is this an output argument? */
    int oarg = 0; 
    /* The type we just decoded */
    enum db_param_types thistype;

    for( c = cmd; *c; c++ )
    {
        oarg = 0;
        if( *c == '%' )
        {
            c++;
            if( *c == '-' )
            {
                oarg = 1;
                c++;
            }
            switch( *c )
            {
                case 'i':
                case 'd':
                    thistype = INT;
                    break;
                case 'u':
                    thistype = UINT;
                    break;
                case 'f':
                case 'g':
                    thistype = DOUBLE;
                    break;
                case 's':
                    thistype = STRING;
                    break;
                case 'l':
                    c++;
                    switch( *c )
                    {
                        case 'i':
                        case 'd':
                            thistype = LONG;
                            break;
                        case 'u':
                            thistype = ULONG;
                            break;
                        case 'l':
                            c++;
                            switch( *c )
                            {
                                case 'i':
                                case 'd':
                                    thistype = LONGLONG;
                                    break;
                                case 'u':
                                    thistype = ULONGLONG;
                                    break;
                                default:
                                    free( types );
                                    free( otypes );
                                    free( query );
                                    return DECODE_ERR_PLACEHOLDER;
                            }
                            break;
                        default:
                            free( otypes );
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
                            thistype = LDOUBLE;
                            break;
                        default:
                            free( types );
                            free( otypes );
                            free( query );
                            return DECODE_ERR_PLACEHOLDER;
                    }
                    break;
                case '%':
                    /* We'll handle this below */
                    break;
                default:
                    free( types );
                    free( otypes );
                    free( query );
                    return DECODE_ERR_PLACEHOLDER;
            }
            if( *c == '%' )
            {
                *q++ = '%';
            }
            else if( oarg )
            {
                otypes[o++] = thistype;
            }
            else
            {
                types[i++] = thistype;
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
    *otypes_out = otypes;
    *oargs_out = oargs;
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
    prep->oparams = 0;
    prep->types = NULL;
    prep->otypes = NULL;
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

    prep->nparams = decode_args( cmd,
            &prep->types, &prep->otypes, &prep->oparams, &prep->query );

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

int vconvert_results( PGresult *result, int nparams,
        enum db_param_types *types, int row, va_list vargs )
{
    for( int i = 0; i < nparams; i++ )
    {
        char *value = PQgetvalue( result, row, i );
        switch( types[i] )
        {
#define DECODE_ARG( e, t, f ) \
            case e : *(va_arg( vargs, t )) = f( value, NULL, 10 ); \
                     break
            DECODE_ARG( CHAR, char *, strtol );
            DECODE_ARG( SHORT, short *, strtol );
            DECODE_ARG( USHORT, unsigned short *, strtoul );
            DECODE_ARG( INT, int *, strtol );
            DECODE_ARG( UINT, unsigned int *, strtoul );
            DECODE_ARG( LONG, long *, strtol );
            DECODE_ARG( ULONG, unsigned long *, strtoul );
            DECODE_ARG( LONGLONG, long long *, strtoll );
            DECODE_ARG( ULONGLONG, unsigned long long *, strtoull );

#define DECODE_ARG2( e, t, f ) \
            case e : *(va_arg( vargs, t )) = f( value, NULL ); \
                     break
            DECODE_ARG2( FLOAT, float *, strtof );
            DECODE_ARG2( DOUBLE, double *, strtod );
            DECODE_ARG2( LDOUBLE, long double *, strtold );

            case STRING:
            *(va_arg( vargs, char ** )) = value;
            break;
        }
    }

    return 0;
}

int convert_results( PGresult *result, int nparams,
        enum db_param_types *types, int row, ...)
{
    va_list vargs ;
    va_start( vargs, row );
    int res = vconvert_results( result, nparams, types, row, vargs );
    va_end( vargs );
    return res;
}


db_result *init_result( PGresult *res, db_prepared *prep,
        int oparams, enum db_param_types *otypes )
{
    db_result *r = malloc( sizeof( db_result ));
    if( r == NULL ) return NULL;

    r->res = res;
    r->nrows = PQntuples( res );
    r->status = PQresultStatus( res );
    r->prep = prep;
    if( prep != NULL )
    {
        r->oparams = prep->oparams;
        r->otypes = prep->otypes;
    }
    else
    {
        r->oparams = oparams;
        r->otypes = otypes;
    }
    r->row = 0;

    return r;
}

void db_free_result( db_result *r )
{
    if( r == NULL ) return;
    PQclear( r->res );
    if( r->prep == NULL )
    {
        /* Only free this if it doesn't belong to the prepared statement */
        free( r->otypes );
    }

    free( r );
}

/*
 * Call a prepared statement returned by db_prep
 *
 * This function accepts the arguments described by the `cmd` argument to
 * db_prep(), and executes the command.
 *
 * Note that this returns results in text format.
 */
PGresult *vdb_exec( db_prepared *prep, va_list vargs )
{
    /* Array of arguments */
    char **args = convert_args( prep->nparams, prep->types, vargs );
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

PGresult *db_exec( db_prepared *prep, ... )
{
    va_list vargs;
    va_start( vargs, prep );
    PGresult *res = vdb_exec( prep, vargs );
    va_end( vargs );
}

db_result *db_exec_wrap( db_prepared *prep, ... )
{
    va_list vargs;
    va_start( vargs, prep );
    PGresult *res = vdb_exec( prep, vargs );
    va_end( vargs );
    return init_result( res, prep, 0, NULL );
}

int db_fetch( db_result *res, ... )
{
    if( res->row >= res->nrows ) return 0;

    va_list vargs;
    va_start( vargs, res );
    vconvert_results( res->res, res->oparams, res->otypes, res->row++, vargs );
    va_end( vargs );

    return 1;
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

    free( prep->name );
    free( prep->query );
    free( prep->types );
    free( prep->otypes );
    free( prep );
}

PGresult *db_exec_direct( PGconn *conn, char *cmd, ... )
{
    if( conn == NULL ) return NULL;
    enum db_param_types *types;
    enum db_param_types *otypes;
    int oargs;
    char *query;
    int nparams = decode_args( cmd, &types, &otypes, &oargs, &query );

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
