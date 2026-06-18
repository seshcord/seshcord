 /*
  * Seshcord - Server - Types
  *
  * Copyright (C) 2025 Mineman
  */

#ifndef _SESHCORD_SERVER_TYPES_H
#define _SESHCORD_SERVER_TYPES_H

#include <seshcord/types.h>
#include <stdint.h>

typedef struct user {
	sc_id_t id;
	char* username;
	char* display_name;
	char* email;
	char* password_hash;
	char* avatar_url;
	char* created_at;
	char* bio;
} user_t;

typedef struct channel {
	sc_id_t id;
	sc_id_t server_id;
	char* name;
	char* type;
	char* category;
	char* desc;
	char* created_at;
} channel_t;

typedef struct server_t {
	sc_id_t id;
	sc_id_t owner_id;
	char* name;
	char* icon_url;
	char* created_at;
} server_t;

#endif /* _SESHCORD_SERVER_TYPES_H */
