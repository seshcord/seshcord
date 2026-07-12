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
    enum db_prep_status status; /* The status; mainly used to return errors */
} db_prepared;

db_prepared *db_prep( PGconn *, char *, char * );
PGresult *db_exec( db_prepared *, ... );
PGresult *db_exec_direct( PGconn *, char *, ... );
void db_free_prepped( db_prepared *);
