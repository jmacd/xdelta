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

TARGETS = xdelta3-debug \
	  xdelta3 \
	  xdelta3-debug2 \
	  xdelta3-debug3 \
	  xdelta3.o \
	  xdelta3_wrap.o \
	  xdelta3module.so \
	  xdelta3-32 \
	  xdelta3-64 \
	  xdelta3-everything \
	  xdelta3-Opg \
	  xdelta3-64-O \
	  xdelta3-Op \
	  xdelta3-decoder xdelta3-decoder-nomain.o \
	  xdelta3-nosec.o xdelta3-all.o xdelta3-fgk.o \
	  xdelta3-noext xdelta3-tools xdelta3-tune \
	  xdelta3-notools \
	  xdelta3_wrap.c xdelta3.py \
	  $(PYTGT) $(SWIGTGT)

PYTHON = python

WIXDIR = "/cygdrive/c/Program Files/wix2.0.4820"

#SWIGTGT = xdelta3module.so
SWIGTGT = xdelta3module.dll

#PYTGT = build/lib.linux-i686-2.4/xdelta3main.so
PYTGT = build/lib.cygwin-1.5.24-i686-2.4/xdelta3main.dll

EXTRA = Makefile COPYING linkxd3lib.c badcopy.c xdelta3.swig \
        draft-korn-vcdiff.txt xdelta3.vcproj badcopy.vcproj \
	xdelta3-regtest.py xdelta3-test.py setup.py \
	examples/Makefile examples/small_page_test.c \
	examples/README examples/encode_decode_test.c \
	examples/compare_test.c examples/speed_test.c \
	xdelta3.py xdelta3_wrap.c xdelta3.wxs xdelta3.wxi \
	README readme.txt

SWIG_FLAGS = -DXD3_DEBUG=0 \
	      -DEXTERNAL_COMPRESSION=0 \
	      -DXD3_USE_LARGEFILE64=1 \
	      -DGENERIC_ENCODE_TABLES=1 \
              -DSECONDARY_DJW=1 \
	      -DVCDIFF_TOOLS=1 \
              -DSWIG_MODULE=1

# $Format: "REL=$Xdelta3Version$" $
REL=3.0s
RELDIR = xdelta$(REL)

all: xdelta3-debug xdelta3

all-py: all $(PYTGT) $(SWIGTGT)

all-targets: $(TARGETS)

all-targets-test: all-targets test

pytgt: $(PYTGT)
swigtgt: $(SWIGTGT)

test:
	./xdelta3-debug test

tar:
	tar --exclude ".svn" -czf /tmp/$(RELDIR)-tmp.tar.gz $(SOURCES) $(EXTRA)
	rm -rf /tmp/$(RELDIR)
	mkdir /tmp/$(RELDIR)
	(cd /tmp/$(RELDIR) && tar -xzf ../$(RELDIR)-tmp.tar.gz)
	tar -czf ./$(RELDIR).tar.gz -C /tmp $(RELDIR)
	+tar -tzf ./$(RELDIR).tar.gz
	rm -rf /tmp/$(RELDIR)

zip:
	tar --exclude ".svn" -czf /tmp/$(RELDIR)-tmp.tar.gz $(SOURCES) $(EXTRA)
	rm -rf /tmp/$(RELDIR)
	mkdir /tmp/$(RELDIR)
	(cd /tmp/$(RELDIR) && tar -xzf ../$(RELDIR)-tmp.tar.gz)
	tar -czf ./$(RELDIR).tar.gz -C /tmp $(RELDIR)
	+zip -r $(RELDIR).zip /tmp/$(RELDIR)
	rm -rf /tmp/$(RELDIR)

clean:
	rm -f $(TARGETS)
	rm -rf build Debug Release core cifs* *.stackdump *.exe \
		xdelta3.ncb xdelta3.suo xdelta3.sln xdelta3.wixobj xdelta3.msi

wix: xdelta3.wxs xdelta3.wxi readme.txt Release\xdelta3.exe
	$(WIXDIR)/candle.exe xdelta3.wxs -out xdelta3.wixobj
	$(WIXDIR)/light.exe xdelta3.wixobj -out xdelta3.msi

xdelta3: $(SOURCES)
	$(CC) -O3 -Wall -Wshadow -fno-builtin xdelta3.c -lm -o xdelta3 \
              -DXD3_DEBUG=0 \
              -DXD3_USE_LARGEFILE64=1 \
              -DREGRESSION_TEST=1 \
              -DSECONDARY_DJW=1 \
              -DSECONDARY_FGK=1 \
	      -DUNALIGNED_OK=1 \
              -DXD3_MAIN=1 \
              -DXD3_POSIX=1

xdelta3-32: $(SOURCES)
	$(CC) -O3 -Wall -Wshadow -fno-builtin xdelta3.c -lm -o xdelta3-32 \
              -DXD3_DEBUG=1 \
              -DXD3_USE_LARGEFILE64=0 \
              -DREGRESSION_TEST=1 \
              -DSECONDARY_DJW=1 \
              -DSECONDARY_FGK=1 \
              -DXD3_MAIN=1 \
              -DXD3_POSIX=1

xdelta3-debug: $(SOURCES)
	$(CC) -g -Wall -Wshadow \
		xdelta3.c -o xdelta3-debug \
		-DXD3_DEBUG=1 \
		-DXD3_MAIN=1 \
		-DXD3_STDIO=1 \
		-DXD3_USE_LARGEFILE64=1 \
		-DGENERIC_ENCODE_TABLES=1 \
		-DREGRESSION_TEST=1 \
	        -DUNALIGNED_OK=1 \
		-DSECONDARY_DJW=1 \
		-DSECONDARY_FGK=1 \
		-lm

xdelta3-debug2: $(SOURCES)
	$(CC) -g -Wall -Wshadow \
		xdelta3.c -o xdelta3-debug2 \
		-DXD3_DEBUG=2 \
		-DXD3_MAIN=1 \
		-DXD3_STDIO=1 \
		-DXD3_USE_LARGEFILE64=1 \
		-DGENERIC_ENCODE_TABLES=1 \
		-DREGRESSION_TEST=1 \
		-DSECONDARY_DJW=1 \
		-DSECONDARY_FGK=1 \
		-lm

xdelta3-debug3: $(SOURCES)
	$(CC) -g -Wall -Wshadow xdelta3.c -o xdelta3-debug3 -DXD3_MAIN=1 -DGENERIC_ENCODE_TABLES=1 \
		-DXD3_USE_LARGEFILE64=1 -DXD3_STDIO=1 -DREGRESSION_TEST=1 -DXD3_DEBUG=3 -DSECONDARY_DJW=1 -DSECONDARY_FGK=1 -lm

$(PYTGT): $(SOURCES) setup.py
	$(PYTHON) setup.py install --verbose --compile --force

xdelta3_wrap.c xdelta3.py: xdelta3.swig
	swig -python xdelta3.swig

xdelta3.o: $(SOURCES)
	$(CC) -O3 -Wall -Wshadow -fno-builtin -c xdelta3.c $(SWIG_FLAGS) -o xdelta3.o

xdelta3_wrap.o: xdelta3_wrap.c
	$(CC) $(SWIG_FLAGS) \
              -DHAVE_CONFIG_H \
	      -I/usr/include/python2.5 \
	      -I/usr/lib/python2.5/config \
	      -fpic \
	      -c -O3 -fno-builtin xdelta3_wrap.c

xdelta3module.dll: xdelta3_wrap.o xdelta3.o
	gcc -shared -Wl,--enable-auto-image-base xdelta3.o xdelta3_wrap.o -L/usr/lib/python2.5/config -lpython2.5 -o xdelta3module.dll
	cp $(SWIGTGT) /usr/lib/python2.5/site-packages

xdelta3module.so: xdelta3_wrap.o xdelta3.o
	ld -shared xdelta3.o xdelta3_wrap.o -o xdelta3module.so /usr/lib/libpython2.4.so -lgcc_s -lc

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

xdelta3-Opg: $(SOURCES)
	$(CC) -pg -g -O3 -fno-builtin -Wall -Wshadow xdelta3.c -o xdelta3-Opg \
		-DXD3_MAIN=1 \
		-DSECONDARY_DJW=1 \
		-DSECONDARY_FGK=1 \
		-DXD3_POSIX=1 \
		-DXD3_USE_LARGEFILE64=1 \
		-DREGRESSION_TEST=1

xdelta3-nosec.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=0 -DSECONDARY_DJW=0 -o xdelta3-nosec.o

xdelta3-all.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=1 -DSECONDARY_DJW=1 -o xdelta3-all.o

xdelta3-fgk.o: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow -c xdelta3.c -DSECONDARY_FGK=1 -DSECONDARY_DJW=0 -o xdelta3-fgk.o

xdelta3-noext: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -DXD3_MAIN=1 -DEXTERNAL_COMPRESSION=0 -o xdelta3-noext

xdelta3-tools: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -DXD3_MAIN=1 -o xdelta3-tools

xdelta3-notools: $(SOURCES)
	$(CC) -O2 -Wall -Wshadow xdelta3.c -DXD3_MAIN=1 -DVCDIFF_TOOLS=0 -o xdelta3-notools
