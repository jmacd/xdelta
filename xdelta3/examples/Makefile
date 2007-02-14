CFLAGS = -g -Wall -I ..

SOURCES = small_page_test.c

small_page_test: $(SOURCES)
	$(CC) $(CFLAGS) small_page_test.c -o small_page_test -DXD3_USE_LARGEFILE64=1 -DSECONDARY_DJW=1 -DXD3_DEBUG=1

clean:
	rm -f *.exe *.stackdump
