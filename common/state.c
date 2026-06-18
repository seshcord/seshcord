/*
 * Seshcord - Internal - State
 *
 * Copyright (C) 2025 Techflash
 */

#include <stddef.h>
#include "state.h"

struct seshcordState __attribute__((visibility("hidden"))) seshcordState = {
	/* initialized */ 0,
	/* events */ { NULL }
};
