
EXTRA_DIST = xdeltatest.c README.test

INCLUDES = -I$(top_srcdir)/libedsio $(GLIB_CFLAGS)

noinst_PROGRAMS    = xdeltatest

xdeltatest_SOURCES  = xdeltatest.c

xdeltatest_LDADD = $(top_srcdir)/libxdelta.la		\
		  $(top_srcdir)/libedsio/libedsio.la	\
		  -lz					\
		  $(GLIB_LIBS)
