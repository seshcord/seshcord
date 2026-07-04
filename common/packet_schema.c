/*
 * This is an automatically generated file. Do not edit.
 * 
 * To regenerate this file, edit spec.yaml and call
 * spec2packetsh.py
 */
#include "packet_schema.h"

/* server-side packets. */

/* previous C->S packet had error */
enum packet_items SESHCORD_SV_ERR_SCHEMA[] = {
    PKT_ITEM_UINT16,
};

/* message sent or editted */
enum packet_items SESHCORD_SV_MSG_SCHEMA[] = {
    PKT_ITEM_UUID,
    PKT_ITEM_UUID,
    PKT_ITEM_UUID,
    PKT_ITEM_STR,
    PKT_ITEM_UINT8,
    PKT_ITEM_LIST,
    PKT_ITEM_STR,
    PKT_ITEM_UINT32,
    PKT_ITEM_STR,
    PKT_ITEM_END,
};

/* message was deleted */
enum packet_items SESHCORD_SV_MSG_DEL_SCHEMA[] = {
    PKT_ITEM_UUID,
    PKT_ITEM_UUID,
};

/* client got a friend request */
enum packet_items SESHCORD_SV_FRIEND_REQ_SCHEMA[] = {
    PKT_ITEM_UUID,
};

/* Minimum and maximum pacet numbers */
#define PACKET_SERVER_MIN -1
#define PACKET_SERVER_MAX 2

/* List of packet type information */
struct packet_info server_packet_dispatcher[] = {
    { SESHCORD_SV_ERR_SCHEMA, SESHCORD_SV_ERR_SCHEMA_LEN, (packet_callback) callback_seshcord_sv_err },
    { SESHCORD_SV_MSG_SCHEMA, SESHCORD_SV_MSG_SCHEMA_LEN, (packet_callback) callback_seshcord_sv_msg },
    { SESHCORD_SV_MSG_DEL_SCHEMA, SESHCORD_SV_MSG_DEL_SCHEMA_LEN, (packet_callback) callback_seshcord_sv_msg_del },
    { SESHCORD_SV_FRIEND_REQ_SCHEMA, SESHCORD_SV_FRIEND_REQ_SCHEMA_LEN, (packet_callback) callback_seshcord_sv_friend_req },
};
/* client-side packets. */

/* previous S->C packet had error */
enum packet_items SESHCORD_CL_ERROR_SCHEMA[] = {
    PKT_ITEM_UINT16,
};

/* client wants to initialize a connection to the server */
enum packet_items SESHCORD_CL_HANDSHAKE_INIT_SCHEMA[] = {
    PKT_ITEM_STR,
    PKT_ITEM_STR,
    PKT_ITEM_STRUCT,
    PKT_ITEM_STR,
    PKT_ITEM_STR,
    PKT_ITEM_STR,
    PKT_ITEM_END,
};

/* client wants to create a new account */
enum packet_items SESHCORD_CL_AUTH_REGISTER_SCHEMA[] = {
    PKT_ITEM_STR,
    PKT_ITEM_STR,
    PKT_ITEM_STR,
    PKT_ITEM_STR,
};

/* client wants to log in to an existing account */
enum packet_items SESHCORD_CL_AUTH_LOGIN_SCHEMA[] = {
    PKT_ITEM_STR,
    PKT_ITEM_STR,
    PKT_ITEM_STR,
};

/* Message sent from client */
enum packet_items SESHCORD_CL_SEND_MSG_SCHEMA[] = {
    PKT_ITEM_UUID,
    PKT_ITEM_STR,
    PKT_ITEM_UINT8,
    PKT_ITEM_LIST,
    PKT_ITEM_STR,
    PKT_ITEM_UINT32,
    PKT_ITEM_BINARY,
    PKT_ITEM_END,
};


/* client wants to get list of DMs */
enum packet_items SESHCORD_CL_GET_DMS_SCHEMA[] = {
    PKT_ITEM_TIME,
};


/* Minimum and maximum pacet numbers */
#define PACKET_CLIENT_MIN -1
#define PACKET_CLIENT_MAX 6

/* List of packet type information */
struct packet_info client_packet_dispatcher[] = {
    { SESHCORD_CL_ERROR_SCHEMA, SESHCORD_CL_ERROR_SCHEMA_LEN, (packet_callback) callback_seshcord_cl_error },
    { SESHCORD_CL_HANDSHAKE_INIT_SCHEMA, SESHCORD_CL_HANDSHAKE_INIT_SCHEMA_LEN, (packet_callback) callback_seshcord_cl_handshake_init },
    { SESHCORD_CL_AUTH_REGISTER_SCHEMA, SESHCORD_CL_AUTH_REGISTER_SCHEMA_LEN, (packet_callback) callback_seshcord_cl_auth_register },
    { SESHCORD_CL_AUTH_LOGIN_SCHEMA, SESHCORD_CL_AUTH_LOGIN_SCHEMA_LEN, (packet_callback) callback_seshcord_cl_auth_login },
    { SESHCORD_CL_SEND_MSG_SCHEMA, SESHCORD_CL_SEND_MSG_SCHEMA_LEN, (packet_callback) callback_seshcord_cl_send_msg },
    { NULL, 0, (packet_callback) callback_seshcord_cl_get_servers },
    { SESHCORD_CL_GET_DMS_SCHEMA, SESHCORD_CL_GET_DMS_SCHEMA_LEN, (packet_callback) callback_seshcord_cl_get_dms },
    { NULL, 0, (packet_callback) callback_seshcord_cl_get_friends },
};
