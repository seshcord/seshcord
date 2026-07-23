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

static sqlf_result *sqlf_vexec( PGconn *, char *, va_list );
static sqlf_result *sqlf_vexec_prep( sqlf_prepared *prep, va_list vargs );
static int sqlf_vfetch( sqlf_result *, va_list );
static int decode_args( char *, enum sqlf_param_types **,
        enum sqlf_param_types **, int *, char ** );
static char **convert_args( int nparams, enum sqlf_param_types *,
        va_list );
static int convert_results( PGresult *, int,
        enum sqlf_param_types *, int, ... );
static int vconvert_results( PGresult *, int,
        enum sqlf_param_types *, int, va_list vargs );
static sqlf_result *init_result( PGresult *, sqlf_prepared *,
        int, enum sqlf_param_types * );

/* ------------------------------------------------------------ */
/* External interface */
/* ------------------------------------------------------------ */

/*
 * Create a prepared SQL statement for use later.
 *
 * This is a frontend to PQprepare(), which takes the given name and
 * command, creates a prepared statement, and returns a structure which can
 * be used to easily execute the prepared statement with arguments later.
 * This structure is dynamically allocated, and must be freed with
 * sqlf_free_prepped when no longer needed (including if an error occurrs.)
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
 * Prefixing a parameter with a - (Ex. "%-i") means that the parameter is an
 * *output* parameter; it will be removed from the query, and will be used
 * later to decode results into target variables.
 *
 * conn: The PostgreSQL connection
 * name: The name of the prepared statement to create
 * cmd: The command to prepare, with placeholders
 *
 * return: A new dynamically allocated sqlf_prepared structure, or NULL if one
 * couldn't be allocated. If an error occurred, the `status` member will
 * contain the error:
 *  SQLF_BAD_CONN: Passed a null or invalid connection.
 *  SQLF_MALLOC_ERROR: Failed to malloc memory.
 *  SQLF_INVALID_PLACEHOLDER: The query had an invalid placeholder.
 *  SQLF_CONN_ERROR: PostgreSQL returned an error.
 *
 */
sqlf_prepared *sqlf_prep( PGconn *conn, char *name, char *cmd )
{
    sqlf_prepared *prep = malloc( sizeof( sqlf_prepared ));
    if( prep == NULL ) return NULL;

    prep->conn = NULL;
    prep->name = NULL;
    prep->query = NULL;
    prep->nparams = 0;
    prep->oparams = 0;
    prep->types = NULL;
    prep->otypes = NULL;
    prep->status = SQLF_OK;

    if( conn == NULL )
    {
        prep->status = SQLF_BAD_CONN;
        return prep;
    }
    prep->conn = conn;

    prep->name = strdup( name );
    if( prep->name == NULL )
    {
        prep->status = SQLF_MALLOC_ERROR;
        return prep;
    }

    prep->nparams = decode_args( cmd,
            &prep->types, &prep->otypes, &prep->oparams, &prep->query );

    if( prep->nparams == DECODE_ERR_MALLOC )
    {
        prep->status = SQLF_MALLOC_ERROR;
        return prep;
    }
    if( prep->nparams == DECODE_ERR_PLACEHOLDER )
    {
        prep->status = SQLF_INVALID_PLACEHOLDER;
        return prep;
    }

    PGresult *res = PQprepare( conn, name,
            prep->query, prep->nparams, NULL );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK )
    {
        prep->status = SQLF_CONN_ERROR;
    }
    PQclear( res );
    return prep;
}

/*
 * Call a prepared statement returned by sqlf_prep
 *
 * This function accepts the arguments described by the `cmd` argument to
 * sqlf_prep(), and executes the command.
 *
 * prep: The prepared statement object
 * ...: The input parameters, matching the format specified in the
 *      prepared statement.
 *
 * Return: The results, as a malloced sqlf_result object
 */
sqlf_result *sqlf_exec_prep( sqlf_prepared *prep, ... )
{
    va_list vargs;
    va_start( vargs, prep );
    sqlf_result *out = sqlf_vexec_prep( prep, vargs );
    va_end( vargs );
    return out;
}

/*
 * Execute a SQL statement directly, without a prepared statement
 *
 * conn: The SQL connection
 * cmd: The query to execute, possibly with placeholders (see sqlf_prep)
 * ...: The variables, if appropriate
 *
 * Return: The result, as a sqlf_result, or NULL on error.
 */

sqlf_result *sqlf_exec( PGconn *conn, char *cmd, ... )
{
    va_list vargs;
    va_start( vargs, cmd );
    sqlf_result *res = sqlf_vexec( conn, cmd, vargs );
    va_end( vargs );
    return res;
}

/*
 * Execute a query directly, and save the first row of results.
 *
 * This is effectively a sqlf_exec() and sqlf_fetch() rolled into one.
 *
 * This will execute a query, possibly with placeholders, pass the given
 * inputs into the query, and save any outputs to the supplied output
 * variables. Functionally this is more or less equialent to a sqlf_prep
 * followed by a sqlf_exec_prep.
 *
 * Note that all input parameters must be specified first (in order),
 * followed by all output parameters (in order), regardless of whether
 * output placeholders appear before input parameters in the query.
 *
 * If no results are returned, the supplied variables are untouched. Always
 * check the result object to see if any rows were returned.
 * 
 * conn: The connection
 * cmd: The query to execute, with placeholders.
 * ...: The parameters for the query.
 *
 * Return: A result object. This must be freed, but not until any data
 * returned (particularly string data) is no longer needed, or has been
 * copied.
 *
 */

sqlf_result *sqlf_exec_inline( PGconn *conn, char *cmd, ... )
{
    va_list vargs;
    va_start( vargs, cmd );
    sqlf_result *res = sqlf_vexec( conn, cmd, vargs );
    if( res != NULL ) sqlf_vfetch( res, vargs );
    va_end( vargs );
    return res;
}

sqlf_result *sqlf_exec_prep_inline( sqlf_prepared *prep, ... )
{
    va_list vargs;
    va_start( vargs, prep );
    sqlf_result *res = sqlf_vexec_prep( prep, vargs );
    if( res != NULL ) sqlf_vfetch( res, vargs );
    va_end( vargs );
    return res;
}

/*
 * Fetch the next row of results from a result object.
 *
 * res: The result object
 * ...: Pointers to the variables to store results in, as specified by the
 *      output variables in the prepared statement.
 *
 * Return: True if there were results to fetch, else false.
 */

int sqlf_fetch( sqlf_result *res, ... )
{
    va_list vargs;
    va_start( vargs, res );
    int out = sqlf_vfetch( res, vargs );
    va_end( vargs );
    return out;
}

/*
 * Free a returned result object and associated objects.
 *
 * This also clears the underlying SQL result object.
 */

void sqlf_free_result( sqlf_result *r )
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
 * Free a prepared statement created by sqlf_prep().
 *
 * This frees all mallocated memory, and is safe to use even if the structure
 * was only partially initialized due to an error. Note that it does *not*
 * remove the prepared statement on the server.
 */

void sqlf_free_prep( sqlf_prepared *prep )
{
    if( prep == NULL ) return;

    free( prep->name );
    free( prep->query );
    free( prep->types );
    free( prep->otypes );
    free( prep );
}

/* ------------------------------------------------------------ */
/* Internal helper functions */
/* ------------------------------------------------------------ */

/*
 * sqlf_exec(), but takes a va_list (The actual meat of sqlf_exec)
 */
static sqlf_result *sqlf_vexec( PGconn *conn, char *cmd, va_list vargs )
{
    if( conn == NULL ) return NULL;
    enum sqlf_param_types *types;
    enum sqlf_param_types *otypes;
    int oargs;
    char *query;
    int nparams = decode_args( cmd, &types, &otypes, &oargs, &query );

    if( nparams == DECODE_ERR_MALLOC ||
            nparams == DECODE_ERR_PLACEHOLDER )
    {
        return NULL;
    }

    char **args = convert_args( nparams, types, vargs );
    if( args == NULL ) return NULL;

    PGresult *res = NULL;
    res = PQexecParams( conn, query, nparams,
            NULL, /* Parameter types: not needed for string args */
            (const char * const *) /* C sucks */ args,
            NULL, /* Parameter lengths: not needed for string args */
            NULL, /* ...Likewise for formats */
            0 /* Results in text format. */ );
    free( args );
    if( res == NULL ) return NULL;
    sqlf_result *out = init_result( res, NULL, oargs, otypes );
    if( out != NULL ) return out;
    PQclear( res );
    return NULL;
}

/*
 * sqlf_exec_prep() but takes a va_list
 */
static sqlf_result *sqlf_vexec_prep( sqlf_prepared *prep, va_list vargs )
{
    /* Array of arguments */
    char **args = convert_args( prep->nparams, prep->types, vargs );
    if( args == NULL )
    {
        prep->status = SQLF_MALLOC_ERROR;
        return NULL;
    }

    PGresult *res = PQexecPrepared( prep->conn, prep->name, prep->nparams,
            (const char * const *) /* C sucks */ args,
            NULL, /* Parameter lengths not needed for string args */
            NULL, /* ...Likewise for formats */
            0 /* Results in text format. */ );
    free( args );

    sqlf_result *out = init_result( res, prep, 0, NULL );
    if( out == NULL ) PQclear( res );
    return out;
}

/*
 * sqlf_fetch(), but takes a va_list
 */
static int sqlf_vfetch( sqlf_result *res, va_list vargs )
{
    if( res->row >= res->nrows ) return 0;

    vconvert_results( res->res, res->oparams, res->otypes, res->row++, vargs );

    return 1;
}

/*
 * Decode a format string for sqlf_prep and friends
 *
 * cmd: The query to decode, with printf-style placeholders
 * types_out: (A pointer to) An array of the types that were decoded. (NULL if
 *      there were no input placeholders)
 * otypes_out: (A pointer to) An array of the output types that were decoded.
 *      (NULL if there were none)
 * oargs_out: (A pointer to) The number of output arguments
 * query_out: The translated query
 *
 * Return: The number of arguments found, or:
 *      DECODE_ERR_MALLOC: malloc() failure
 *      DECODE_ERR_PLACEHOLDER Invalid format string
 */

static int decode_args( char *cmd,
        enum sqlf_param_types **types_out,
        enum sqlf_param_types **otypes_out,
        int *oargs_out,
        char **query_out )
{
    /* Number of arguments found */
    int nargs = 0;
    int oargs = 0;
    /* Pointer into cmd */
    char *c;
    /* The types we found */
    enum sqlf_param_types *types = NULL;
    enum sqlf_param_types *otypes = NULL;
    /* The translated query */
    char *query = NULL;
    /* The error we return with */
    int err = 0;

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

    /* This is the error we might encounter this stage */
    err = DECODE_ERR_MALLOC;

    if( nargs > 0 )
    {
        types = malloc( sizeof( types ) * nargs );
        if( types == NULL ) goto fail;
    }
    if( oargs > 0 )
    {
        otypes = malloc( sizeof( types ) * oargs );
        if( otypes == NULL ) goto fail;
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

    query = malloc( i );
    if( query == NULL ) goto fail;

    /* Pointer into query during population */
    char *q = query; 
    /* Placeholder index/count */
    i = 0; 
    int o = 0;

    /* Is this an output argument? */
    int oarg = 0; 
    /* The type we just decoded */
    enum sqlf_param_types thistype;

    /* This is the error we might encounter this stage */
    err = DECODE_ERR_PLACEHOLDER;

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
            /* FIXME: Handle floats, chars and shorts */
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
                                    goto fail;
                            }
                            break;
                        default:
                            goto fail;
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
                            goto fail;
                    }
                    break;
                case '%':
                    /* We'll handle this below */
                    break;
                default:
                    goto fail;
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

fail:
    free( types );
    free( otypes );
    free( query );
    return err;
    
}

/*
 * Convert a list of arguments to an array of strings.
 *
 * nparams: The number of arguments supplied
 * types: The types of the arguments, as returned by decode_args()
 * vargs: The arguments
 *
 * Return: An array of strings, or NULL if memory couldn't be allocated.
 *      This pointer should be free()d when no longer needed (which also
 *      frees the string data, as it is all allocated in a single chunk)
 */
static char **convert_args(
        int nparams, enum sqlf_param_types *types, va_list vargs )
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
        /* FIXME: Handle chars, floats and shorts */
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

#define procarg( f, t ) \
    args[i] = c; \
    c += sprintf( c, f, va_arg( vargs, t )); \
    break

            /* For integer arguments, convert them to strings and add the
             * result */
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
 * Convert results from a query and copy them to variables.
 *
 * This takes a query result, refers to the output parameters specified in
 * the query (see sqlf_prep), converts then, and saves them into the variables
 * specified.
 *
 * result: The SQL result object
 * nparams: The number of output parameters
 * types: The output parameter types
 * row: The row to save
 * ...: (Pointers to) the variables to save into
 *
 * Return: Currently unused.
 */
static int convert_results( PGresult *result, int nparams,
        enum sqlf_param_types *types, int row, ...)
{
    va_list vargs ;
    va_start( vargs, row );
    int res = vconvert_results( result, nparams, types, row, vargs );
    va_end( vargs );
    return res;
}
/*
 * convert_results(), but takes a va_list
 */

static int vconvert_results( PGresult *result, int nparams,
        enum sqlf_param_types *types, int row, va_list vargs )
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

/*
 * Initialize a sqlf_result object
 *
 * res: The underlying SQL result object
 * prep: The prepared statement object, if applicable
 * oparams: The number of output parameters (taken from `prep` if supplied)
 * otypes: The type of output parameters (taken from `prep if supplied)
 *
 * Return: A malloced sqlf_result
 */

static sqlf_result *init_result( PGresult *res, sqlf_prepared *prep,
        int oparams, enum sqlf_param_types *otypes )
{
    sqlf_result *r = malloc( sizeof( sqlf_result ));
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

