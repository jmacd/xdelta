bin_PROGRAMS = xdelta3
xdelta3_SOURCES = \
	  xdelta3-blkcache.h \
          xdelta3-cfgs.h \
	  xdelta3-decode.h \
	  xdelta3-djw.h \
	  xdelta3-fgk.h \
	  xdelta3-hash.h \
	  xdelta3-list.h \
	  xdelta3-main.h \
	  xdelta3-merge.h \
	  xdelta3-python.h \
	  xdelta3-second.h \
	  xdelta3-test.h \
	  xdelta3.c \
	  xdelta3.h

WFLAGS = -Wall -Wshadow -fno-builtin -Wextra -Wsign-compare \
	 -Wextra -Wno-unused-parameter 
# -Wconversion
xdelta3_CFLAGS = $(WFLAGS) \
	      -DGENERIC_ENCODE_TABLES=0 \
	      -DREGRESSION_TEST=1 \
	      -DSECONDARY_DJW=1 \
	      -DSECONDARY_FGK=1 \
	      -DXD3_DEBUG=0 \
	      -DXD3_MAIN=1 \
	      -DXD3_POSIX=1 \
	      -DXD3_USE_LARGEFILE64=1
