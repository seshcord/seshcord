CFLAGS=-DDEBUG -I../include -I../common
TESTS=enctest ememcpytest

.PHONY: all clean

all: $(TESTS)
clean:
	rm -f $(TESTS) *.o

# This is *really* quick and dirty but it'll do for now
enctest: ../common/packet_encoding.o ../common/packet_schema.o ../common/endian_memcpy.o ../common/malloc_group.o ../common/blurt.o enctest.o
	gcc $(CFLAGS) -o $@ $^

ememcpytest: ../common/endian_memcpy.o ../common/malloc_group.o ememcpytest.o
	gcc $(CFLAGS) -o $@ $^











