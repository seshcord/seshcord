CFLAGS=-DDEBUG -I../include -I../common

enctest: ../common/packet_encoding.c ../common/packet_schema.c ../common/endian_memcpy.c ../common/malloc_group.c ../common/blurt.c enctest.c
	gcc $(CFLAGS) -o $@ $^






