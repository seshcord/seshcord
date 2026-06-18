/*
 * Seshcord - Common - Packet handling
 *
 * Copyright (C) 2025-2026 Techflash
 */

#ifndef _SESHCORD_PACKET_H
#define _SESHCORD_PACKET_H

#include <stddef.h>
#include <stdint.h>
#include <seshcord/types.h>

/* packet info */
#define SESHPKT_MAGIC "SPKT"
#define SESHPKT_MAGIC_SIZE 4

enum seshpktClientIds {
	SESHCORD_CL_HANDSHAKE_INIT = 0,
	SESHCORD_CL_AUTH_REGISTER = 1,
	SESHCORD_CL_AUTH_LOGIN = 2,
	SESHCORD_CL_SEND_MSG = 3,
	SESHCORD_CL_GET_SERVERS = 4,
	SESHCORD_CL_GET_DMS = 5,
	SESHCORD_CL_GET_FRIENDS = 6,

	SESHCORD_CL_ERROR = -1
};

enum seshpktServerIds {
	SESHCORD_SV_MSG = 0,
	SESHCORD_SV_MSG_DEL = 1,
	SESHCORD_SV_FRIEND_REQ = 2,

	SESHCORD_SV_ERR = -1
};

/* main seshpkt structure */
struct seshpkt {
	uint8_t magic[4];
	uint32_t dataLength;
	int16_t type;
	uint16_t seq;
} __attribute__((packed));

/* various data formats */
struct scAttachment {
	char *filename;
	uint32_t size;
	char *path;
};

/* packet data formats */
struct seshcordCLHandshakeInit {
	char *clientName;
	char *clientVer;
	struct {
		char *cpuArch;
		char *os;
		char *deviceModel;
	} hostInfo;
};

struct seshcordSVMsg {
	sc_id_t id;
	sc_id_t chat;
	sc_id_t sender;
	char *content;
	uint8_t numAttachments;
	struct scAttachment *attachments;
};

struct seshcordSVErr {
	uint16_t faultedCLSeqId;
};

struct seshcordCLErr {
	uint16_t faultedSVSeqId;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* seshpkt helper functions */
extern void seshcordDecodePacket(const struct seshpkt *data, size_t len);
extern void seshcordEncodePacket(struct seshpkt **out, int id, const void *data);
extern void seshcordDestroyPacket(struct seshpkt *pkt);

#define SESHPKT_DATA(pkt) ((void *)(((uintptr_t)pkt) + sizeof(struct seshpkt)))
#define SESHPKT_NEXT(pkt) ((struct seshpkt *)(((uintptr_t)pkt) + sizeof(struct seshpkt) + seshcord_be32_to_cpu(pkt->dataLength)))
#define SESHPKT_WRITE_STR(out, src) strcpy((char *)(out), (src)); \
	out = (uint8_t *)(((uintptr_t)(out)) + strlen((char *)(out)) + 2);
#define SESHPKT_READ_STR(src, destPtr) destPtr = (char *)(src); \
	src = (uint8_t *)(((uintptr_t)(src)) + strlen((char *)(src)) + 2);


#define SESHPKT_MAX_DATALEN 65535

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SESHCORD_PACKET_H */
