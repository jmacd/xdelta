CFLAGS = -g -O2 -Wall -I ..

SOURCES = small_page_test.c

small_page_test: $(SOURCES)
	$(CC) $(CFLAGS) small_page_test.c -o small_page_test -DXD3_USE_LARGEFILE64=1 -DSECONDARY_DJW=1
