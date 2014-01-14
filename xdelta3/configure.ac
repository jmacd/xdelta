AC_INIT([Xdelta3], [3.0.8], [josh.macdonald@gmail.com], 
	[xdelta3], [http://xdelta.org/])
AC_PREREQ([2.68])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AM_INIT_AUTOMAKE([1.9 no-define foreign tar-ustar])
#AC_DISABLE_STATIC
AX_CHECK_ALIGNED_ACCESS_REQUIRED
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_HEADERS([lzma.h])
AC_CHECK_LIB(lzma, lzma_easy_buffer_encode)
AC_CHECK_SIZEOF(size_t)
#AM_PATH_PYTHON(,, [:])
#AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])
#AX_PYTHON_DEVEL()
#AX_PKG_SWIG(2.0.0,,)
#AX_SWIG_PYTHON

dnl --enable-debug-symbols : build with debug symbols?
AC_ARG_ENABLE(debug-symbols,
   AS_HELP_STRING(--enable-debug-symbols,[Build with debug symbols (default is NO)]),,enableval=no)
AM_CONDITIONAL([DEBUG_SYMBOLS], [test ${enableval} = "yes"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
