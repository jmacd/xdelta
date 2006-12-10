# xdelta 3 - delta compression tools and library
# Copyright (C) 2001, 2003, 2004, 2005, 2006.  Joshua P. MacDonald

SOURCES = xdelta3-cfgs.h \
          xdelta3-decode.h \
          xdelta3-djw.h \
          xdelta3-fgk.h \
          xdelta3-list.h \
          xdelta3-main.h \
          xdelta3-python.h \
          xdelta3-second.h \
          xdelta3-test.h \
          xdelta3.c \
          xdelta3.h

TARGETS = xdelta3 xdelta3-debug xdelta3-64 xdelta3-everything \
	  xdelta3-Opg xdelta3-64-O xdelta3-Op xdelta3-O \
	  xdelta3-decoder xdelta3-decoder-nomain.o \
	  $(PYTGT) \
	  xdelta3-nosec.o xdelta3-all.o xdelta3-fgk.o xdelta3-djw.o \
	  xdelta3-noext xdelta3-tools xdelta3-tune \
	  xdelta3-notools

PYTHON = python
PYTGT = build/temp.linux-i686-2.3/xdelta3.so

PYFILES = xdelta3-regtest.py setup.py

EXTRA = Makefile COPYING linkxd3lib.c badcopy.c www

# $Format: "REL=$Xdelta3Version$" $
REL=0h_pre0
RELDIR = xdelta3$(REL)

XDELTA1 = ../xdelta11/xdelta

all: xdelta3-debug xdelta3 $(PYTGT)

all-targets: $(TARGETS)

tar:
	tar --exclude ".svn" -czf /tmp/$(RELDIR)-tmp.tar.gz $(SOURCES) $(PYFILES) $(EXTRA)
	rm -rf /tmp/$(RELDIR)
	mkdir /tmp/$(RELDIR)
	(cd /tmp/$(RELDIR) && tar -xzf ../$(RELDIR)-tmp.tar.gz)
	tar -czf ./$(RELDIR).tar.gz -C /tmp $(RELDIR)
	+tar -tzf ./$(RELDIR).tar.gz
	rm -rf /tmp/$(RELDIR)

clean:
	rm -f $(TARGETS)

xdelta3: $(SOURCES)
	$(CC) -O3 -Wall -Wshadow xdelta3.c -lm -o xdelta3 \
              -DXD3_DEBUG=0 \
              -DXD3_USE_LARGEFILE64=1 \
              -DREGRESSION_TEST=1 \
              -DSECONDARY_DJW=1 \
              -DSECONDARY_FGK=1 \
              -DXD3_MAIN=1 \
              -DXD3_POSIX=1

xdelta3-debug: $(SOURCES)
	$(CC) -g -Wall -Wshadow xdelta3.c -o xdelta3-debug -DXD3_MAIN=1 -DGENERIC_ENCODE_TABLES=1 \
		-DXD3_USE_LARGEFILE64=1 -DXD3_STDIO=1 -DREGRESSION_TEST=1 -DXD3_DEBUG=1 -DSECONDARY_DJW=1 -DSECONDARY_FGK=1 -lm

$(PYTGT): $(SOURCES)
	$(PYTHON) setup.py install --verbose --compile --force

xdelta3-decoder: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c \
	    -DXD3_ENCODER=0 -DXD3_MAIN=1 -DSECONDARY_FGK=0 -DSECONDARY_DJW=0 \
	    -DXD3_STDIO=1 -DEXTERNAL_COMPRESSION=0 -DVCDIFF_TOOLS=0 \
	    -o xdelta3-decoder

xdelta3-decoder-nomain.o: $(SOURCES) linkxd3lib.c
	$(CC) -O2 -Wall -Wshadow xdelta3.c linkxd3lib.c \
	    -DXD3_ENCODER=0 -DSECONDARY_FGK=0 -DSECONDARY_DJW=0 \
	    -o xdelta3-decoder-nomain.o
	strip xdelta3-decoder-nomain.o

xdelta3-O: $(SOURCES)
	$(CC) -g -O2 -Wall -Wshadow xdelta3.c -o xdelta3-O -DXD3_MAIN=1 -DSECONDARY_DJW=1 -DREGRESSION_TEST=1 -lm

xdelta3-O++: $(SOURCES)
	$(CXX) -g -O2 -Wall -Wshadow xdelta3.c -o xdelta3-O++ -DXD3_MAIN=1 -DSECONDARY_DJW=1 -DREGRESSION_TEST=1 -lm

xdelta3-Op: $(SOURCES)
	$(CC) -g -O2 -Wall -Wshadow xdelta3.c -o xdelta3-Op -DXD3_POSIX=1 -DXD3_MAIN=1 -DREGRESSION_TEST=1 -lm

xdelta3-64: $(SOURCES)
	$(CC) -g -Wall -Wshadow xdelta3.c -o xdelta3-64 -DXD3_POSIX=1 -DXD3_MAIN=1 -DREGRESSION_TEST=1 \
					-DXD3_DEBUG=0 -DXD3_USE_LARGEFILE64=1 -lm

xdelta3-64-O: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -o xdelta3-64-O -DXD3_POSIX=1 -DXD3_MAIN=1 \
					-DXD3_USE_LARGEFILE64=1 -lm

xdelta3-everything: $(SOURCES)
	$(CC) -g -Wall -Wshadow xdelta3.c -o xdelta3-everything \
					-DXD3_MAIN=1 -DVCDIFF_TOOLS=1 -DREGRESSION_TEST=1 \
					-DSECONDARY_FGK=1 -DSECONDARY_DJW=1 \
					-DGENERIC_ENCODE_TABLES=1 \
					-DGENERIC_ENCODE_TABLES_COMPUTE=1 \
					-DXD3_POSIX=1 \
					-DEXTERNAL_COMPRESSION=1 \
					-DXD3_DEBUG=1 -lm

xdelta3-tune: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -o xdelta3-tune -DXD3_MAIN=1 \
		-DSECONDARY_FGK=1 -DSECONDARY_DJW=1 -DTUNE_HUFFMAN=1

xdelta3-Opg: $(SOURCES)
	$(CC) -pg -g -O3 -Wall -Wshadow xdelta3.c -o xdelta3-Opg -DXD3_MAIN=1 \
		-DSECONDARY_DJW=1 -DXD3_POSIX=1 -DXD3_USE_LARGEFILE64=1

xdelta3-nosec.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=0 -DSECONDARY_DJW=0 -o xdelta3-nosec.o

xdelta3-all.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=1 -DSECONDARY_DJW=1 -o xdelta3-all.o

xdelta3-fgk.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=1 -DSECONDARY_DJW=0 -o xdelta3-fgk.o

xdelta3-djw.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=0 -DSECONDARY_DJW=1 -o xdelta3-djw.o

xdelta3-noext: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -DXD3_MAIN=1 -DEXTERNAL_COMPRESSION=0 -o xdelta3-noext

xdelta3-tools: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -DXD3_MAIN=1 -o xdelta3-tools

xdelta3-notools: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -DXD3_MAIN=1 -DVCDIFF_TOOLS=0 -o xdelta3-notools
