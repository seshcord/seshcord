/*
 * Seshcord - Internal headers - State
 *
 * Copyright (C) 2025 Techflash
 */

#ifndef _SESHCORD_INTERNAL_STATE_H
#define _SESHCORD_INTERNAL_STATE_H

#include <seshcord/event.h>

extern struct seshcordState {
	int initialized;
	seshcordCallback callbacks[SESHCORD_EVENT_MAX];
} seshcordState;

#endif /* _SESHCORD_INTERNAL_STATE_H */
