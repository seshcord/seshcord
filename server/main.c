/*
 * Seshcord - Server - Main initialization
 *
 * Copyright (C) 2025 Mineman
 * Copyright (C) 2025-2026 Techflash
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <libpq-fe.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <seshcord/config.h>
#include <seshcord/endian.h>
#include <seshcord/packet.h>
#include "./database.h"
#include "./types.h"

enum recvStates {
	RECV_STATE_START_PKT,
	RECV_STATE_CONTINUE_PKT,
	RECV_STATE_GET_DATA
};

int main(void) {
	PGconn *conn;
	char conninfo[128], *dbHost, *dbPort, *dbName, *dbUser, *dbPass;
	user_t user;
	int sockfd, connfd, ret, remainingPktBytes;
	struct sockaddr_in servaddr;
	struct seshpkt pkt;
	void *curRecvBuf, *buf;
	enum recvStates curState;

	/* 1. Get vars */
	dbHost = getenv("DBHOST");
	dbPort = getenv("DBPORT");
	dbName = getenv("DBNAME");
	dbUser = getenv("DBUSER");
	dbPass = getenv("DBPASS");

	if (!dbHost)
		dbHost = "localhost";
	if (!dbPort)
		dbPort = "5432";
	if (!dbName)
		dbName = "seshcorddb";
	if (!dbUser)
		dbUser = "seshcord";
	if (!dbPass) {
		fputs("Missing DBPASS\r\n", stderr);
		return 1;
	}

	/* 2. Prepare connection info */
	snprintf(conninfo, sizeof(conninfo), "host=%s port=%s dbname=%s user=%s password=%s", dbHost, dbPort, dbName, dbUser, dbPass);

	/* 3. Connect to database */
	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "Connection failed: %s\r\n", PQerrorMessage(conn));
		exit_nicely(conn);
	}

	/*
	user.username = "seshcord";
	user.display_name = "SeshCord System";
	user.password_hash = "testpassword";
	user.email = "seshcord@minemans.site";
	user.avatar_url = NULL;
	user.bio = "This is the SeshCord system user.";

	add_user(conn, user);

	memset(&user, 0, sizeof(user));
	user.id = get_user_id_from_username(conn, "seshcord");
	if (user.id == (uint64_t)-1)
		return 1;
	printf("ID: %llu\r\n", user.id);
	get_user_by_id(conn, &user, user.id);
	printf("Username: %s, Display Name: %s\r\n", user.username, user.display_name);
	free_user(&user);
	delete_user(conn, user);
	*/


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket");
		return 1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(SESHCORD_PORT);

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		perror("bind");
		return 1;
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		return 1;
	}

	connfd = accept(sockfd, NULL, NULL);
	if (connfd == -1) {
		perror("accept");
		return 1;
	}

	fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK);
	curState = RECV_STATE_START_PKT;
	buf = NULL;

	while (true) {
		switch (curState) {
		case RECV_STATE_START_PKT: {
			if (buf)
				free(buf);
			curRecvBuf = (void *)&pkt;
			remainingPktBytes = sizeof(struct seshpkt);
			curState = RECV_STATE_CONTINUE_PKT;
			/* fallthrough */
		}
		case RECV_STATE_CONTINUE_PKT: {
			ret = read(connfd, curRecvBuf, remainingPktBytes);
			if (ret == -1 && errno == EAGAIN) {
				usleep(20 * 1000);
				continue; /* TODO: poll() */
			}
			else if (ret < remainingPktBytes) {
				curRecvBuf = (void *)(((uintptr_t)&pkt) + ret);
				remainingPktBytes -= ret;
				continue;
			}

			/* we have a whole packet, now process it */
			else if (memcmp(pkt.magic, SESHPKT_MAGIC, SESHPKT_MAGIC_SIZE)) {
				printf("Bogus packet magic: %c%c%c%c\r\n", pkt.magic[0], pkt.magic[1], pkt.magic[2], pkt.magic[3]);
				break;
			}
			else if (seshcord_be32_to_cpu(pkt.dataLength) > SESHPKT_MAX_DATALEN) {
				printf("Packet too large: %u\r\n", seshcord_be32_to_cpu(pkt.dataLength));
				break;
			}

			remainingPktBytes = seshcord_be32_to_cpu(pkt.dataLength);
			buf = malloc(remainingPktBytes + sizeof(struct seshpkt));
			memcpy(buf, &pkt, sizeof(struct seshpkt));
			curRecvBuf = (void *)(((uintptr_t)buf) + sizeof(struct seshpkt));

			curState = RECV_STATE_GET_DATA;
			/* fallthrough */
		}
		case RECV_STATE_GET_DATA: {
			ret = read(connfd, curRecvBuf, remainingPktBytes);
			if (ret == -1 && errno == EAGAIN)
				continue; /* TODO: poll() */
			else if (ret < remainingPktBytes) {
				curRecvBuf = (void *)(((uintptr_t)&pkt) + ret);
				remainingPktBytes -= ret;
				continue;
			}

			/* got all the data, let's decode it */
			seshcordDecodePacket(buf, seshcord_be32_to_cpu(pkt.dataLength) + sizeof(struct seshpkt));
			break;
		}
		}
	}

	PQfinish(conn);

	return 0;
}
