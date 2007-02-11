
INCLUDES = -I$(top_srcdir)/libedsio $(GLIB_CFLAGS)

bin_PROGRAMS    = xdelta
bin_SCRIPTS	= xdelta-config

xdelta_SOURCES  = xdmain.c getopt.c getopt1.c

xdelta_LDADD    = libxdelta.la				\
		  $(top_srcdir)/libedsio/libedsio.la	\
		  $(GLIB_LIBS)				\
		  -lz

include_HEADERS = xdelta.h xd_edsio.h
noinst_HEADERS  = xdeltapriv.h getopt.h

lib_LTLIBRARIES = libxdelta.la

libxdelta_la_SOURCES = xdelta.c xdapply.c $(SER_SOURCES)
libxdelta_la_LIBADD  = $(GLIB_LIBS)

EXTRA_DIST = xd.ser $(SER_OUT) xdelta.magic xdelta.prj xdelta.m4	\
	autogen.sh xdelta.dsp xdelta.dsw stamp-ser xdrsync.c

SUBDIRS = libedsio . test doc djgpp

m4datadir = $(datadir)/aclocal
m4data_DATA = xdelta.m4

## $Format: "libxdelta_la_LDFLAGS = -version-info $LibCurrent$:$LibRevision$:$LibAge$" $
libxdelta_la_LDFLAGS = -version-info 2:0:0

#
# Rules for the generated code
#

stamp-ser: $(top_srcdir)/libedsio/edsio.el xd.ser
	$(top_srcdir)/libedsio/edsio-comp xd.ser
	touch stamp-ser

SER_OUT = xd_edsio.h xd_edsio.c

$(SER_OUT): stamp-ser

SER_SOURCES   = xd_edsio.c
BUILT_SOURCES = $(SER_SOURCES)

#
#
#
