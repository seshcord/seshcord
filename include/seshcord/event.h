 /*
  * Seshcord - Common - Event handling
  *
  * Copyright (C) 2025-2026 Techflash
  */

#ifndef _SESHCORD_EVENT_H
#define _SESHCORD_EVENT_H

enum seshcordEvent {
	SESHCORD_EVENT_MSG,
	SESHCORD_EVENT_MSG_DEL,
	SESHCORD_EVENT_FRIEND_REQUEST,
	SESHCORD_EVENT_MAX
};

typedef void (*seshcordCallback)(void *data);
extern void seshcordSetEventCallback(enum seshcordEvent event, seshcordCallback cb);
extern void seshcordEventFire(enum seshcordEvent event, void *data);

#endif /* _SESHCORD_EVENT_H */
