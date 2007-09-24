CFLAGS = -g -Wall -I.. -DXD3_DEBUG=1

SOURCES = small_page_test.c

all: small_page_test encode_decode_test

small_page_test: small_page_test.c
	$(CC) $(CFLAGS) small_page_test.c -o small_page_test -DXD3_USE_LARGEFILE64=1 -DSECONDARY_DJW=1

encode_decode_test: encode_decode_test.c
	$(CC) $(CFLAGS) encode_decode_test.c -o encode_decode_test

clean:
	rm -f *.exe *.stackdump
