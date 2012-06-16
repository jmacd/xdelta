bin_PROGRAMS = xdelta3 xdelta3regtest

common_SOURCES = \
	  xdelta3-blkcache.h \
	  xdelta3-decode.h \
	  xdelta3-djw.h \
	  xdelta3-fgk.h \
	  xdelta3-hash.h \
	  xdelta3-internal.h \
	  xdelta3-list.h \
	  xdelta3-main.h \
	  xdelta3-merge.h \
	  xdelta3-python.h \
	  xdelta3-second.h \
	  xdelta3-test.h \
          xdelta3-cfgs.h \
	  xdelta3.h

xdelta3_SOURCES = $(common_SOURCES) xdelta3.c

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

WFLAGS = -Wall -Wshadow -fno-builtin -Wextra -Wsign-compare \
	 -Wextra -Wno-unused-parameter
common_CFLAGS = $(WFLAGS) \
	      -DGENERIC_ENCODE_TABLES=0 \
	      -DREGRESSION_TEST=1 \
	      -DSECONDARY_DJW=1 \
	      -DSECONDARY_FGK=1 \
	      -DXD3_POSIX=1 \
	      -DXD3_USE_LARGEFILE64=1 \
	      -DXD3_MAIN=1

xdelta3_CFLAGS = $(common_CFLAGS) 

xdelta3regtest_CXXFLAGS = $(common_CFLAGS) -DNOT_MAIN=1 -DXD3_DEBUG=1
xdelta3regtest_CFLAGS = $(common_CFLAGS) -DNOT_MAIN=1 -DXD3_DEBUG=1

EXTRA_DIST = \
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
	examples/iOS/xdelta3-ios-test/xdelta3-ios-test/xdelta3-ios-test-Prefix.pch
