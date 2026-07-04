CFLAGS=-DDEBUG -I../include -I../common
TESTS=enctest ememcpytest

.PHONY: all clean

all: $(TESTS)
clean:
	rm -f $(TESTS)

# This is *really* quick and dirty but it'll do for now
enctest: ../common/packet_encoding.c ../common/packet_schema.c ../common/endian_memcpy.c ../common/malloc_group.c ../common/blurt.c enctest.c
	gcc $(CFLAGS) -o $@ $^

ememcpytest: ../common/endian_memcpy.c ../common/malloc_group.c ememcpytest.c
	gcc $(CFLAGS) -o $@ $^











