ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = xdelta3
noinst_PROGRAMS = xdelta3regtest xdelta3decode

common_SOURCES = \
	  xdelta3-blkcache.h \
	  xdelta3-decode.h \
	  xdelta3-djw.h \
	  xdelta3-fgk.h \
	  xdelta3-hash.h \
	  xdelta3-internal.h \
	  xdelta3-list.h \
	  xdelta3-lzma.h \
	  xdelta3-main.h \
	  xdelta3-merge.h \
	  xdelta3-second.h \
	  xdelta3-test.h \
          xdelta3-cfgs.h \
	  xdelta3.h

xdelta3_SOURCES = $(common_SOURCES) xdelta3.c

xdelta3decode_SOURCES = $(common_SOURCES) xdelta3.c

xdelta3regtest_SOURCES = $(common_SOURCES) \
	testing/cmp.h \
	testing/delta.h \
	testing/file.h \
	testing/modify.h \
	testing/random.h \
	testing/regtest.cc \
	testing/regtest_c.c \
	testing/segment.h \
	testing/sizes.h \
	testing/test.h

# Note: for extra sanity checks, enable -Wconversion. Note there
# are a lot of false positives.
WFLAGS = -Wall -Wshadow -fno-builtin -Wextra -Wsign-compare \
	 -Wextra -Wno-unused-parameter

C_WFLAGS = $(WFLAGS) -pedantic -std=c99
CXX_WFLAGS = $(WFLAGS)

common_CFLAGS = \
	      -DGENERIC_ENCODE_TABLES=0 \
	      -DREGRESSION_TEST=1 \
	      -DSECONDARY_DJW=1 \
	      -DSECONDARY_FGK=1 \
	      -DXD3_POSIX=1 \
	      -DXD3_USE_LARGEFILE64=1 \
	      -DXD3_MAIN=1

if DEBUG_SYMBOLS
  common_CFLAGS += -g
endif

# For additional debugging, add -DXD3_DEBUG=1, 2, 3, ...
xdelta3_CFLAGS = $(C_WFLAGS) $(common_CFLAGS) -DXD3_DEBUG=0
xdelta3_LDADD = -lm

xdelta3decode_CFLAGS = \
	$(C_WFLAGS) \
	-DGENERIC_ENCODE_TABLES=0 \
	-DREGRESSION_TEST=0 \
	-DSECONDARY_DJW=0 \
	-DSECONDARY_FGK=0 \
	-DSECONDARY_LZMA=0 \
	-DXD3_USE_LARGEFILE64=1 \
	-DXD3_MAIN=1 \
	-DXD3_ENCODER=0 \
	-DXD3_STDIO=1 \
	-DEXTERNAL_COMPRESSION=0 \
	-DVCDIFF_TOOLS=0

xdelta3regtest_CXXFLAGS = \
	$(CXX_WFLAGS) $(common_CFLAGS) -DNOT_MAIN=1 -DXD3_DEBUG=1
xdelta3regtest_CFLAGS = \
	$(C_WFLAGS) $(common_CFLAGS) -DNOT_MAIN=1 -DXD3_DEBUG=1
xdelta3regtest_LDADD = -lm

man1_MANS = xdelta3.1

EXTRA_DIST = \
	draft-korn-vcdiff.txt \
	README \
	examples/Makefile \
	examples/README \
	examples/checksum_test.cc \
	examples/compare_test.c \
	examples/encode_decode_test.c \
	examples/small_page_test.c \
	examples/speed_test.c \
	examples/test.h \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test.xcodeproj/project.pbxproj \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/Xd3iOSAppDelegate.h \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/Xd3iOSAppDelegate.m \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/Xd3iOSViewController.h \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/Xd3iOSViewController.m \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/en.lproj/InfoPlist.strings \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/en.lproj/MainStoryboard_iPad.storyboard \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/en.lproj/MainStoryboard_iPhone.storyboard \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/file_v1.bin \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/file_v1_to_v2.bin \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/file_v2.bin \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/main.m \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/xdelta3-ios-test-Info.plist \
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/xdelta3-ios-test-Prefix.pch \
	Makefile.mingw \
	Makefile.orig \
	testing/xdelta3-regtest.py \
	testing/xdelta3-test.py \
	xdelta3.1 \
	xdelta3.i \
	xdelta3.vcxproj \
	xdelta3.wxi \
	xdelta3.wxs

# Broken, removed from distribution:
#	xdelta3_pywrap.c
#	xdelta3.py

#PYFILES = xdelta3_pywrap.c xdelta3.py
#XDELTA3PY = xdelta3.py
#XDELTA3PYLIB = xdelta3.la

#BUILT_SOURCES = $(PYFILES)

#xdelta3_pywrap.c xdelta3.py : xdelta3.i
#	$(SWIG) -python -o xdelta3_pywrap.c xdelta3.i

# OS X for some reason requires:
# pythondir = $(PYTHON_SITE_PKG)
# pyexecdir = $(PYTHON_SITE_PKG)

#python_PYTHON = $(XDELTA3PY)
#pyexec_LTLIBRARIES = $(XDELTA3PYLIB)
#_xdelta3_la_SOURCES = $(srcdir)/xdelta3_pywrap.c $(xdelta3_SOURCES)
#_xdelta3_la_CFLAGS = $(common_CFLAGS) -DNOT_MAIN=1 $(PYTHON_CPPFLAGS)
#_xdelta3_la_LDFLAGS = -module
