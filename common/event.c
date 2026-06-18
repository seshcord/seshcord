/*
 * Seshcord - Internal - Events
 *
 * Copyright (C) 2025 Techflash
 */

#include <stddef.h>
#include <seshcord/event.h>
#include "state.h"

void seshcordSetEventCallback(enum seshcordEvent event, seshcordCallback cb) {
	if (event > 0 && event < SESHCORD_EVENT_MAX)
		seshcordState.callbacks[event] = cb;
}

void seshcordEventFire(enum seshcordEvent event, void *data) {
	if (event > 0 && event < SESHCORD_EVENT_MAX && seshcordState.callbacks[event])
		seshcordState.callbacks[event](data);
}
