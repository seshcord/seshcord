#
# Seshcord - Server Makefile
# Copyright (C) 2025 Techflash
#

override ROOT := ..
override SRC := $(ROOT)/common/init.c $(ROOT)/common/packet.c $(ROOT)/common/event.c $(ROOT)/common/state.c
override SRC := $(SRC) $(ROOT)/server/channel.c $(ROOT)/server/user.c $(ROOT)/server/main.c

override OBJ := $(patsubst $(ROOT)/%.c,$(ROOT)/build/%.o,$(SRC))
override OUT := $(ROOT)/bin/seshcord_server

CFLAGS ?= -Wall -Wextra -Wformat=2 -Wno-variadic-macros -std=gnu89 -pedantic -fno-permissive -O2 -g
override CFLAGS := $(CFLAGS) -I$(ROOT)/include -D_SESHCORD_SERVER -I/opt/homebrew/Cellar/libpq/18.4/include


LDFLAGS := -L/opt/homebrew/Cellar/libpq/18.4/lib -lpq

.PHONY: all clean
all: $(OUT)

$(OUT): $(OBJ)
	@mkdir -p $(@D)
	$(info $s  LD    $(patsubst $(ROOT)/%,%,$@))
	@$(CC) $(LDFLAGS) $^ -o $@

$(ROOT)/build/%.o: $(ROOT)/%.c
	@mkdir -p $(@D)
	$(info $s  CC    $(patsubst $(ROOT)/%,%,$<))
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(ROOT)/bin $(ROOT)/build
