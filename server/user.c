/*
 * Seshcord - Server - Main initialization
 *
 * Copyright (C) 2025 Mineman
 * Copyright (C) 2025-2026 Techflash
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "./types.h"

void exit_nicely(PGconn *conn) {
	PQfinish(conn);
	exit(1);
}

void add_user(PGconn *conn, user_t user) {
	const char *args[6];
	PGresult *res;

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	args[0] = user.username;
	args[1] = user.display_name;
	args[2] = user.email;
	args[3] = user.password_hash;
	args[4] = user.avatar_url;
	args[5] = user.bio;

	res = PQexecParams(
		conn,
		"INSERT INTO users (username, display_name, email, password_hash, avatar_url, bio) "
		"VALUES ($1, $2, $3, $4, $5, $6) RETURNING id;",
		6,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Failed to insert user: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}

void delete_user(PGconn *conn, user_t user) {
	PGresult *res;
	char user_id_str[64];
	const char *args[1];
	sprintf(user_id_str, "%llu", user.id);

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	args[0] = user_id_str;

	res = PQexecParams(
		conn,
		"DELETE FROM users WHERE id = $1",
		1,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "Failed to delete user: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}

void update_user(PGconn *conn, user_t user) {
	PGresult *res;
	char user_id_str[64];
	const char *args[7];
	sprintf(user_id_str, "%llu", user.id);

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	args[0] = user.username;
	args[1] = user.display_name;
	args[2] = user.email;
	args[3] = user.password_hash;
	args[4] = user.avatar_url;
	args[5] = user.bio;
	args[6] = user_id_str;

	res = PQexecParams(
		conn,
		"UPDATE users SET username = $1, display_name = $2, email = $3, password_hash = $4, avatar_url = $5, bio = $6 WHERE id = $7",
		7,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "Failed to update user: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}


#define COPY_STR(field, num) field = strdup(PQgetvalue(res, 0, num));

void get_user_by_id(PGconn *conn, user_t *user, sc_id_t id) {
	PGresult *res;
	const char *args[1];
	char user_id_str[64], *endptr, *user_val;
	sprintf(user_id_str, "%llu", id);

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	args[0] = user_id_str;

	res = PQexecParams(
		conn,
		"SELECT * FROM users WHERE id = $1",
		1,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Failed to get user: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	if (PQntuples(res) == 0) {
		fprintf(stderr, "User not found\n");
		PQclear(res);
		return;
	}

	user_val = PQgetvalue(res, 0, 0);

	user->id = strtoul(user_val, &endptr, 10);
	COPY_STR(user->username, 1);
	COPY_STR(user->display_name, 2);
	COPY_STR(user->email, 3);
	COPY_STR(user->password_hash, 4);
	COPY_STR(user->avatar_url, 5);
	COPY_STR(user->bio, 6);

	PQclear(res);
}

sc_id_t get_user_id_from_username(PGconn *conn, const char *username) {
	PGresult *res;
	char /* user_id_str[64], */ *endptr, *user_val;
	const char *args[1];

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return -1;
	}

	args[0] = username;

	res = PQexecParams(
		conn,
		"SELECT * FROM users WHERE username = $1",
		1,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Failed to get user: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return -1;
	}

	if (PQntuples(res) == 0) {
		fprintf(stderr, "User not found\n");
		PQclear(res);
		return -1;
	}

	user_val = PQgetvalue(res, 0, 0);
	PQclear(res);
	return strtoul(user_val, &endptr, 10);
}

void free_user(user_t *user) {
	if (user->username) free(user->username);
	if (user->display_name) free(user->display_name);
	if (user->email) free(user->email);
	if (user->password_hash) free(user->password_hash);
	if (user->avatar_url) free(user->avatar_url);
	if (user->bio) free(user->bio);
}
