/*
 * Seshcord - Common - Packet handling
 *
 * Copyright (C) 2025-2026 Techflash
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <seshcord/endian.h>
#include <seshcord/event.h>
#include <seshcord/packet.h>

static uint16_t seqID = 0;

void seshcordDecodePacket(const struct seshpkt *_pkt, size_t len) {
	void *cur;
	struct seshpkt pkt, *orig;
	union {
		struct seshcordSVErr *svErr;
		struct seshcordCLHandshakeInit clHandshakeInit;
	} data;

	orig = (void *)_pkt;
again:

	memcpy(&pkt, _pkt, sizeof(struct seshpkt));
	pkt.type = seshcord_be16_to_cpu(pkt.type);
	pkt.seq = seshcord_be16_to_cpu(pkt.seq);
	pkt.dataLength = seshcord_be32_to_cpu(pkt.dataLength);

	printf("Decode packet (len=%lu): type=0x%04x, seq_id=%u, dataLength=%u\r\n", len, pkt.type, pkt.seq, pkt.dataLength);
	if (memcmp(pkt.magic, SESHPKT_MAGIC, SESHPKT_MAGIC_SIZE) || len < (pkt.dataLength + sizeof(struct seshpkt))) {
		puts("DEBUG: Invalid seshpkt!");
		return;
	}

	cur = SESHPKT_DATA(_pkt);

	switch (pkt.type) {
	#ifdef _SESHCORD_SERVER
	case SESHCORD_CL_HANDSHAKE_INIT: {
		printf("clientName offset read = %lu\r\n", ((uintptr_t)cur) - ((uintptr_t)_pkt));
		SESHPKT_READ_STR(cur, data.clHandshakeInit.clientName);
		printf("clientVer offset read = %lu\r\n", ((uintptr_t)cur) - ((uintptr_t)_pkt));
		SESHPKT_READ_STR(cur, data.clHandshakeInit.clientVer);
		printf("cpuArch offset read = %lu\r\n", ((uintptr_t)cur) - ((uintptr_t)_pkt));
		SESHPKT_READ_STR(cur, data.clHandshakeInit.hostInfo.cpuArch);
		printf("os offset read = %lu\r\n", ((uintptr_t)cur) - ((uintptr_t)_pkt));
		SESHPKT_READ_STR(cur, data.clHandshakeInit.hostInfo.os);
		printf("deviceModel offset read = %lu\r\n", ((uintptr_t)cur) - ((uintptr_t)_pkt));
		SESHPKT_READ_STR(cur, data.clHandshakeInit.hostInfo.deviceModel);
		printf("Got CL_HANDSHAKE_INIT from: clientName=\"%s\" clientVer=\"%s\" cpuArch=\"%s\" os=\"%s\", deviceModel=\"%s\"\r\n",
			data.clHandshakeInit.clientName, data.clHandshakeInit.clientVer, data.clHandshakeInit.hostInfo.cpuArch, data.clHandshakeInit.hostInfo.os, data.clHandshakeInit.hostInfo.deviceModel);
		break;
	}
	#elif defined(_SESHCORD_CLIENT)
	case SESHCORD_SV_MSG: {
		seshcordEventFire(SESHCORD_EVENT_MSG, &data);
		break;
	}
	case SESHCORD_SV_MSG_DEL: {
		seshcordEventFire(SESHCORD_EVENT_MSG_DEL, &data);
		break;
	}
	case SESHCORD_EVENT_FRIEND_REQUEST: {
		seshcordEventFire(SESHCORD_EVENT_FRIEND_REQUEST, &data);
		break;
	}
	case SESHCORD_SV_ERR: {
		data.svErr = SESHPKT_DATA(&pkt);
		printf("got SESHCORD_SV_ERR for client seq_id=%u\r\n", data.svErr->faultedCLSeqId);
		/* TODO: handle it somehow */
		break;
	}

	#endif /* _SESHCORD_CLIENT */
	}

	_pkt = SESHPKT_NEXT(_pkt);
	if ((uintptr_t)_pkt < ((uintptr_t)orig) + len)
		goto again;

	return;
}

void seshcordEncodePacket(struct seshpkt **out, int id, const void *data) {
	const struct seshcordCLHandshakeInit *clHandshakeInit;
	void *outCur = malloc(sizeof(struct seshpkt));

	*out = outCur;
	memcpy((*out)->magic, SESHPKT_MAGIC, SESHPKT_MAGIC_SIZE);
	(*out)->type = seshcord_cpu_to_be16(id);
	(*out)->seq = seshcord_cpu_to_be16(seqID++);
	if (!data) {
		(*out)->dataLength = 0;
		return;
	}

	switch (id) {
	#ifdef _SESHCORD_SERVER
	#elif defined(_SESHCORD_CLIENT)
	case SESHCORD_CL_HANDSHAKE_INIT: {
		clHandshakeInit = data;
		*out = realloc(*out,
			sizeof(struct seshpkt) +
			strlen(clHandshakeInit->clientName) +
			strlen(clHandshakeInit->clientVer) +
			strlen(clHandshakeInit->hostInfo.cpuArch) +
			strlen(clHandshakeInit->hostInfo.os) +
			strlen(clHandshakeInit->hostInfo.deviceModel)
		);

		outCur = SESHPKT_DATA(*out);
		printf("clientName offset write = %lu\r\n", ((uintptr_t)outCur) - ((uintptr_t)*out));
		SESHPKT_WRITE_STR(outCur, clHandshakeInit->clientName);
		printf("clientVer offset write = %lu\r\n", ((uintptr_t)outCur) - ((uintptr_t)*out));
		SESHPKT_WRITE_STR(outCur, clHandshakeInit->clientVer);
		printf("cpuArch offset write = %lu\r\n", ((uintptr_t)outCur) - ((uintptr_t)*out));
		SESHPKT_WRITE_STR(outCur, clHandshakeInit->hostInfo.cpuArch);
		printf("os offset write = %lu\r\n", ((uintptr_t)outCur) - ((uintptr_t)*out));
		SESHPKT_WRITE_STR(outCur, clHandshakeInit->hostInfo.os);
		printf("deviceModel offset write = %lu\r\n", ((uintptr_t)outCur) - ((uintptr_t)*out));
		SESHPKT_WRITE_STR(outCur, clHandshakeInit->hostInfo.deviceModel);
		break;
	}
	#endif /* _SESHCORD_CLIENT */
	default:
		assert(!"Unimplemented packet");
	}

	(*out)->dataLength = seshcord_cpu_to_be32(((uintptr_t)outCur) - ((uintptr_t)SESHPKT_DATA(*out)));
	return;
}

void seshcordDestroyPacket(struct seshpkt *pkt) {
	(void)pkt;
	return;
}
