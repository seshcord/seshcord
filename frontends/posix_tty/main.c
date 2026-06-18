/*
 * Seshcord - Frontends - POSIX TTY - Main initialization
 *
 * Copyright (C) 2025-2026 Techflash
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <seshcord/config.h>
#include <seshcord/endian.h>
#include <seshcord/event.h>
#include <seshcord/init.h>
#include <seshcord/packet.h>

static void messageCallback(void *data) {
	struct seshcordSVMsg *msg = data;
	puts("got message");
}

int main(void) {
	int sockfd, error;
	struct seshpkt *pkt;
	struct seshcordCLHandshakeInit handshakeInit;
	struct addrinfo *addrs, hints;

	/* initialize seshcord internal state */
	seshcordInit();

	/* set up callback for message recieved */
	seshcordSetEventCallback(SESHCORD_EVENT_MSG, messageCallback);

	/* initialize the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* DNS */
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = 0;
	error = getaddrinfo(SESHCORD_DOMAIN, SESHCORD_PORT_STR, &hints, &addrs);
	if (error) {
		perror("failed to resolve " SESHCORD_DOMAIN);
		return 1;
	}

	/* connect the client socket to server socket */
	if (connect(sockfd, addrs->ai_addr, sizeof(*addrs->ai_addr)) != 0) {
		perror("connect() to " SESHCORD_DOMAIN " failed");
		return 1;
	}

	puts("Connected");

	/* send init packet */
	handshakeInit.clientName = "Your mother";
	handshakeInit.clientVer = "v6.9.0";
	handshakeInit.hostInfo.cpuArch = "PowerPC";
	handshakeInit.hostInfo.os = "Linux";
	handshakeInit.hostInfo.deviceModel = "Nintendo(R) Wii(TM)";
	seshcordEncodePacket(&pkt, SESHCORD_CL_HANDSHAKE_INIT, &handshakeInit);
	write(sockfd, pkt, sizeof(struct seshpkt) + seshcord_be32_to_cpu(pkt->dataLength));

	/* clean up */
	close(sockfd);

	return 0;
}
