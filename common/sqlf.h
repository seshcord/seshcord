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
enum sqlf_param_types
{
    CHAR, INT, UINT, SHORT, USHORT, LONG, ULONG, LONGLONG, ULONGLONG,
    FLOAT, DOUBLE, LDOUBLE, STRING
};

/* Status/error codes */
enum sqlf_prep_status
{
    SQLF_OK,  /* No errors */

    /* Errors on initialization */

    SQLF_BAD_CONN,            /* Passed a null or invalid connection */
    SQLF_MALLOC_ERROR,        /* Failed to malloc memory */
    SQLF_INVALID_PLACEHOLDER, /* Invalid format string */
    SQLF_CONN_ERROR           /* PostgreSQL returned error */
};

/*
 * Hold the information for executing a prepared statement.
 */
typedef struct sqlf_prepared
{
    PGconn *conn;               /* The PostgreSQL connection */
    char *name;                 /* The name of the prepared statement */
    char *query;                /* The query, as sent to the server */
    int nparams;                /* The number of arguments in the query */
    enum sqlf_param_types *types; /* The types of arguments in the query */
    int oparams;                /* The number of output arguments */
    enum sqlf_param_types *otypes;/* The types of output arguments */
    enum sqlf_prep_status status; /* The status; mainly used to return errors */
} sqlf_prepared;

typedef struct sqlf_result
{
    /* The query result */
    PGresult *res;
    /* The result's status */
    ExecStatusType status;
    /* The prepared query, if any */
    sqlf_prepared *prep;
    /* The number of responses */
    int nrows;
    /* The number of output arguments */
    int oparams;                
    /* The types of output arguments */
    enum sqlf_param_types *otypes;
    /* The next row to fetch */
    int row;
} sqlf_result;

sqlf_prepared *sqlf_prep( PGconn *, char *, char * );
sqlf_result *sqlf_exec_prep( sqlf_prepared *, ... );
sqlf_result *sqlf_exec( PGconn *, char *, ... );
sqlf_result *sqlf_exec_inline( PGconn *, char *, ... );
sqlf_result *sqlf_exec_prep_inline( sqlf_prepared *, ... );
int sqlf_fetch( sqlf_result *, ... );
void sqlf_free_result( sqlf_result *);
void sqlf_free_prep( sqlf_prepared *);
