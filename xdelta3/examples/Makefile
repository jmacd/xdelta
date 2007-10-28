#CFLAGS = -g -Wall -I.. -DXD3_DEBUG=1
CFLAGS = -O3 -Wall -I.. -DXD3_DEBUG=0
# -pg

SOURCES = small_page_test.c encode_decode_test.c speed_test.c

DEPS = ../*.h ../*.c

TARGETS = small_page_test encode_decode_test speed_test32 speed_test64

all: $(TARGETS)

small_page_test: small_page_test.c $(DEPS)
	$(CC) $(CFLAGS) small_page_test.c -o small_page_test -DXD3_USE_LARGEFILE64=0 -DSECONDARY_DJW=1

encode_decode_test: encode_decode_test.c $(DEPS)
	$(CC) $(CFLAGS) encode_decode_test.c -o encode_decode_test

speed_test32: speed_test.c $(DEPS)
	$(CC) $(CFLAGS) -DXD3_USE_LARGEFILE64=0 speed_test.c -o speed_test32

speed_test64: speed_test.c $(DEPS)
	$(CC) $(CFLAGS) -DXD3_USE_LARGEFILE64=1 speed_test.c -o speed_test64

clean:
	rm -f *.exe *.stackdump $(TARGETS)
