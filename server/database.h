#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "types.h"

#ifndef DATABASE_H
#define DATABASE_H

void add_user(PGconn *conn, user_t user);
void delete_user(PGconn *conn, user_t user);
void update_user(PGconn *conn, user_t user);
sc_id_t get_user_id_from_username(PGconn *conn, const char *username);
void get_user_by_id(PGconn *conn, user_t *user, sc_id_t id);
void free_user(user_t *user);
void exit_nicely(PGconn *conn);

#endif /* DATABASE_H */
