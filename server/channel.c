#include "types.h"
#include <stdio.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void create_channel(PGconn *conn, channel_t channel) {
	const char *args[7];
	char server_id[64];
	PGresult *res;

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	sprintf(server_id, "%llu", channel.server_id);

	args[0] = server_id;
	args[1] = channel.name;
	args[2] = channel.type;
	args[3] = channel.category;
	args[4] = channel.desc;

	res = PQexecParams(conn,
		"INSERT INTO channels (server_id, name, type, category, description) "
		"VALUES ($1, $2, $3, $4, $5) RETURNING id;",
		5,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Failed to insert channel: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}

void delete_channel(PGconn *conn, channel_t channel) {
	PGresult *res;
	const char *args[1];

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	args[0] = (const char *)&channel.id;
	res = PQexecParams(conn,
			"DELETE FROM channels WHERE id = $1;",
			1,					/* number of parameters */
			NULL,					/* let PostgreSQL infer types */
			(const char *const *)&args,		/* params */
			NULL,					/* param lengths */
			NULL,					/* param formats */
			0					/* text results */
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "Failed to delete channel: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}

void update_channel(PGconn *conn, channel_t channel) {
	PGresult *res;
	char *args[5];
	char channel_id[64];

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	sprintf(channel_id, "%llu", channel.id);

	args[0] = channel_id;
	args[1] = channel.name;
	args[2] = channel.type;
	args[3] = channel.category;
	args[4] = channel.desc;

	res = PQexecParams(conn,
		"UPDATE channels SET name = $2, type = $3, category = $4, description = $5 WHERE id = $1;",
		5,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "Failed to update channel: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}

void get_channel(PGconn *conn, channel_t channel) {
	PGresult *res;
	const char *args[1];
	char channel_id[64];

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	sprintf(channel_id, "%llu", channel.id);
	args[0] = channel_id;

	res = PQexecParams(conn,
		"SELECT * FROM channels WHERE id = $1;",
		1,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param formats */
		NULL,				/* result formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Failed to get channel: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

	PQclear(res);
}

void get_channels(PGconn *conn, server_t server) {
	PGresult *res;
	const char *args[1];
	char server_id[64];

	if (conn == NULL) {
		fprintf(stderr, "Connection pointer is NULL\n");
		return;
	}

	sprintf(server_id, "%llu", server.id);
	args[0] = server_id;

	res = PQexecParams(conn,
		"SELECT * FROM channels WHERE server_id = $1;",
		1,				/* number of parameters */
		NULL,				/* let PostgreSQL infer types */
		(const char *const *)&args,	/* params */
		NULL,				/* param lengths */
		NULL,				/* param formats */
		0				/* text results */
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Failed to get channels: %s\n", PQerrorMessage(conn));
		PQclear(res);
		return;
	}

    PQclear(res);
}
