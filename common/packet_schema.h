/*
 * This is an automatically generated file. Do not edit.
 * 
 * To regenerate this file, edit spec.yaml and call
 * spec2packetsh.py
 */
#include <stdlib.h> 
#include <stdint.h>
#pragma pack(1)

typedef uint64_t timestamp;
typedef uint8_t uuid[16];

/* Handler function type for received packets */
typedef void (*packet_callback)( void * );

/*
 * The types of data that may be included in a packet.
 * See the specification for what these types mean.
 * PKT_ITEM_END marks the end of a list or struct.
 */
enum packet_items
{
    PKT_ITEM_INT8,
    PKT_ITEM_UINT8,
    PKT_ITEM_INT16,
    PKT_ITEM_UINT16,
    PKT_ITEM_INT32,
    PKT_ITEM_UINT32,
    PKT_ITEM_INT64,
    PKT_ITEM_UINT64,
    PKT_ITEM_STR,
    PKT_ITEM_TIME,
    PKT_ITEM_UUID,
    PKT_ITEM_BINARY,
    PKT_ITEM_LIST,
    PKT_ITEM_STRUCT,
    PKT_ITEM_END,
};

/*
 * Information for handling a particular packet. This includes
 * schema (which descrives the physical packet structure), and
 * a callback function to handle it.
 */
struct packet_info
{
    enum packet_items * schema; /* Packet schema */
    int len; /* Packet schema length */
    packet_callback callback; /* Handler function */
};

/* server-side packets. */

/* previous C->S packet had error */
#define SESHCORD_SV_ERR -1
struct seshcord_sv_err
{
    uint16_t id; /* sequence ID of faulted packet */
};
extern enum packet_items SESHCORD_SV_ERR_SCHEMA[];
#define SESHCORD_SV_ERR_SCHEMA_LEN 1
void callback_seshcord_sv_err( struct seshcord_sv_err );

/* message sent or editted */
#define SESHCORD_SV_MSG 0
struct seshcord_sv_msg
{
    uuid id; /* message ID of this message (if it's an existing message, the client treats it as an edit) */
    uuid chat; /* uuid of chat, or user in case of dm */
    uuid sender; /* user ID who sent the message */
    char * content; /* message content */
    uint8_t attachCount; /* Number of attachments */
    struct
    {
        char * filename; /* Filename */
        uint32_t size; /* File size */
        char * path; /* CDN file path; client constructs web link based on secure */
    } *attachments;
};
extern enum packet_items SESHCORD_SV_MSG_SCHEMA[];
#define SESHCORD_SV_MSG_SCHEMA_LEN 10
void callback_seshcord_sv_msg( struct seshcord_sv_msg );

/* message was deleted */
#define SESHCORD_SV_MSG_DEL 1
struct seshcord_sv_msg_del
{
    uuid id; /* Message ID */
    uuid by; /* Deletor (may be original user, or mod) */
};
extern enum packet_items SESHCORD_SV_MSG_DEL_SCHEMA[];
#define SESHCORD_SV_MSG_DEL_SCHEMA_LEN 2
void callback_seshcord_sv_msg_del( struct seshcord_sv_msg_del );

/* client got a friend request */
#define SESHCORD_SV_FRIEND_REQ 2
struct seshcord_sv_friend_req
{
    uuid id; /* Requestor */
};
extern enum packet_items SESHCORD_SV_FRIEND_REQ_SCHEMA[];
#define SESHCORD_SV_FRIEND_REQ_SCHEMA_LEN 1
void callback_seshcord_sv_friend_req( struct seshcord_sv_friend_req );

/* client-side packets. */

/* previous S->C packet had error */
#define SESHCORD_CL_ERROR -1
struct seshcord_cl_error
{
    uint16_t sid; /* sequence ID of faulted packet */
};
extern enum packet_items SESHCORD_CL_ERROR_SCHEMA[];
#define SESHCORD_CL_ERROR_SCHEMA_LEN 1
void callback_seshcord_cl_error( struct seshcord_cl_error );

/* client wants to initialize a connection to the server */
#define SESHCORD_CL_HANDSHAKE_INIT 0
struct seshcord_cl_handshake_init
{
    char * clientName; /* Client name */
    char * clientVer; /* Client version */
    struct
    {
        char * cpuArch; /* CPU architecture */
        char * os; /* Operating system name and approximate version (e.g. Windows 10) */
        char * deviceModel; /* Device model name */
    } hostInfo;
};
extern enum packet_items SESHCORD_CL_HANDSHAKE_INIT_SCHEMA[];
#define SESHCORD_CL_HANDSHAKE_INIT_SCHEMA_LEN 7
void callback_seshcord_cl_handshake_init( struct seshcord_cl_handshake_init );

/* client wants to create a new account */
#define SESHCORD_CL_AUTH_REGISTER 1
struct seshcord_cl_auth_register
{
    char * username; /* Username */
    char * email; /* EMail */
    char * password; /* Password (hashed) */
    char * antiSpam; /* anti-spam check value if applicable (captcha?) */
};
extern enum packet_items SESHCORD_CL_AUTH_REGISTER_SCHEMA[];
#define SESHCORD_CL_AUTH_REGISTER_SCHEMA_LEN 4
void callback_seshcord_cl_auth_register( struct seshcord_cl_auth_register );

/* client wants to log in to an existing account */
#define SESHCORD_CL_AUTH_LOGIN 2
struct seshcord_cl_auth_login
{
    char * username; /* username or email */
    char * password; /* Password (hashed) */
    char * antiSpam; /* as above */
};
extern enum packet_items SESHCORD_CL_AUTH_LOGIN_SCHEMA[];
#define SESHCORD_CL_AUTH_LOGIN_SCHEMA_LEN 3
void callback_seshcord_cl_auth_login( struct seshcord_cl_auth_login );

/* Message sent from client */
#define SESHCORD_CL_SEND_MSG 3
struct seshcord_cl_send_msg
{
    uuid chat; /* Chat or user to send message to */
    char * content; /* Message text */
    uint8_t attachCount; /* Number of attachments */
    struct
    {
        char * filename; /* Filename */
        uint32_t size; /* File size */
        char * data; /* The attachment itself. */
    } *attachments;
};
extern enum packet_items SESHCORD_CL_SEND_MSG_SCHEMA[];
#define SESHCORD_CL_SEND_MSG_SCHEMA_LEN 8
void callback_seshcord_cl_send_msg( struct seshcord_cl_send_msg );

/* client wants to get list of joined servers */
#define SESHCORD_CL_GET_SERVERS 4
void callback_seshcord_cl_get_servers( void * );

/* client wants to get list of DMs */
#define SESHCORD_CL_GET_DMS 5
struct seshcord_cl_get_dms
{
    uint64_t since; /* How far back to retrieve DMs */
};
extern enum packet_items SESHCORD_CL_GET_DMS_SCHEMA[];
#define SESHCORD_CL_GET_DMS_SCHEMA_LEN 1
void callback_seshcord_cl_get_dms( struct seshcord_cl_get_dms );

/* client wants to get list of friends */
#define SESHCORD_CL_GET_FRIENDS 6
void callback_seshcord_cl_get_friends( void * );

