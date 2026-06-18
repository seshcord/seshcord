#
# Seshcord - Frontends - POSIX TTY - Makefile
# Copyright (C) 2025 Techflash
#

override ROOT := ../..
override SRC := $(ROOT)/common/init.c $(ROOT)/common/packet.c $(ROOT)/common/event.c $(ROOT)/common/state.c
override SRC := $(SRC) $(ROOT)/frontends/posix_tty/main.c

override OBJ := $(patsubst $(ROOT)/%.c,$(ROOT)/build/%.o,$(SRC))
override OUT := $(ROOT)/bin/seshcord_client_posix_tty

CFLAGS ?= -Wall -Wextra -Wformat=2 -Wno-variadic-macros -std=gnu89 -pedantic -fno-permissive -O2 -g
override CFLAGS := $(CFLAGS) -I$(ROOT)/include -D_SESHCORD_CLIENT -fstack-protector-strong

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
