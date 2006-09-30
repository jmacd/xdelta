
INCLUDES = $(GLIB_CFLAGS)

noinst_SCRIPTS = edsio-comp

lib_LTLIBRARIES = libedsio.la

noinst_PROGRAMS = edsiotest

edsiotest_SOURCES = edsiotest.c

edsiotest_LDADD = libedsio.la $(GLIB_LIBS)

noinst_HEADERS = maketime.h partime.h

include_HEADERS = edsio.h edsio_edsio.h

libedsio_la_LIBADD = $(GLIB_LIBS)

libedsio_la_SOURCES =				\
	library.c				\
	simple.c				\
	edsio.c					\
	edsio_edsio.c				\
	sha.c					\
	md5c.c					\
	fh.c					\
	generic.c				\
	default.c				\
	base64.c				\
	maketime.c				\
	partime.c

EXTRA_DIST = edsio.el edsio.ser $(SER_OUT1) edsio-comp.in edsio.prj stamp-ser1

#
# Rules for the generated code
#

stamp-ser1: $(top_srcdir)/libedsio/edsio.el edsio.ser
	$(top_srcdir)/libedsio/edsio-comp edsio.ser
	touch stamp-ser1

SER_OUT1 = edsio_edsio.c edsio_edsio.h

$(SER_OUT1): stamp-ser1

BUILT_SOURCES = edsio_edsio.c

#
#
#
