/*
 * Seshcord - Slightly improved database access routines
 *
 * Copyright (C) 2026 Nathan Roberts
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libpq-fe.h>

/* The types of arguments supported */
enum db_param_types
{
    CHAR, INT, UINT, SHORT, USHORT, LONG, ULONG, LONGLONG, ULONGLONG,
    FLOAT, DOUBLE, LDOUBLE, STRING
};

/* Status/error codes */
enum db_prep_status
{
    DB_OK,  /* No errors */

    /* Errors on initialization */

    DB_BAD_CONN,            /* Passed a null or invalid connection */
    DB_MALLOC_ERROR,        /* Failed to malloc memory */
    DB_INVALID_PLACEHOLDER, /* Invalid format string */
    DB_CONN_ERROR           /* PostgreSQL returned error */
};

/*
 * Hold the information for executing a prepared statement.
 */
typedef struct db_prepared
{
    PGconn *conn;               /* The PostgreSQL connection */
    char *name;                 /* The name of the prepared statement */
    char *query;                /* The query, as sent to the server */
    int nparams;                /* The number of arguments in the query */
    enum db_param_types *types; /* The types of arguments in the query */
    int oparams;                /* The number of output arguments */
    enum db_param_types *otypes;/* The types of output arguments */
    enum db_prep_status status; /* The status; mainly used to return errors */
} db_prepared;

typedef struct db_result
{
    /* The query result */
    PGresult *res;
    /* The result's status */
    ExecStatusType status;
    /* The prepared query, if any */
    db_prepared *prep;
    /* The number of responses */
    int nrows;
    /* The number of output arguments */
    int oparams;                
    /* The types of output arguments */
    enum db_param_types *otypes;
    /* The next row to fetch */
    int row;
} db_result;

db_prepared *db_prep( PGconn *, char *, char * );
db_result *db_exec_prep( db_prepared *, ... );
db_result *db_exec( PGconn *, char *, ... );
db_result *db_exec_inline( PGconn *, char *, ... );
db_result *db_exec_prep_inline( db_prepared *, ... );
int db_fetch( db_result *, ... );
void db_free_result( db_result *);
void db_free_prep( db_prepared *);
