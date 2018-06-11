CFLAGS = -g -Wall -I.. -DXD3_DEBUG=1 -DNDEBUG=0 -DSIZEOF_SIZE_T=8 -DSIZEOF_UNSIGNED_LONG_LONG=8
#CFLAGS = -O3 -Wall -I.. -DXD3_DEBUG=0 -fno-builtin -DNDEBUG=1
# -pg

SOURCES = small_page_test.c encode_decode_test.c speed_test.c

DEPS = ../*.h ../*.c *.h

TARGETS = small_page_test encode_decode_test speed_test32 speed_test64 compare_test checksum_test

all: $(TARGETS)

small_page_test: small_page_test.c $(DEPS)
	$(CC) $(CFLAGS) small_page_test.c -o small_page_test -DXD3_USE_LARGEFILE64=0 -DSECONDARY_DJW=1

encode_decode_test: encode_decode_test.c $(DEPS)
	$(CC) $(CFLAGS) encode_decode_test.c -o encode_decode_test

speed_test32: speed_test.c $(DEPS)
	$(CC) $(CFLAGS) -DXD3_USE_LARGEFILE64=0 speed_test.c -o speed_test32

speed_test64: speed_test.c $(DEPS)
	$(CC) $(CFLAGS) -DXD3_USE_LARGEFILE64=1 speed_test.c -o speed_test64

compare_test: compare_test.c
	$(CC) $(CFLAGS) compare_test.c -o compare_test

checksum_test: checksum_test.cc
	$(CXX) $(CFLAGS) checksum_test.cc -o checksum_test

clean:
	rm -r -f *.exe *.stackdump $(TARGETS) *.dSYM *~
