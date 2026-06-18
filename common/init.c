/*
 * Seshcord - Common - Initialization
 *
 * Copyright (C) 2025 Techflash
 */

#include <string.h>
#include <seshcord/init.h>
#include "state.h"

void seshcordInit(void) {
	seshcordState.initialized = 1;
	memset(seshcordState.callbacks, 0, sizeof(seshcordState.callbacks));

	return;
}
