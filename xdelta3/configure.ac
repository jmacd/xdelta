AC_INIT([Xdelta3], [3.0.1], [josh.macdonald@gmail.com], 
	[xdelta3], [http://xdelta.org/])
AC_PREREQ([2.68])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AM_INIT_AUTOMAKE([1.11 no-define foreign tar-ustar])
AC_DISABLE_STATIC
AC_PROG_CC
AC_PROG_CXX
AM_PATH_PYTHON([])
AX_PYTHON_DEVEL([])
AX_PKG_SWIG([])
AX_SWIG_PYTHON
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

dnl See the note in Makefile.am; this isn't working.
dnl AM_CONDITIONAL(PYTHON_WRONG_SITE_PREFIX, [python -c 
dnl   "import distutils.sysconfig; 
dnl    exit(0 if distutils.sysconfig.PREFIX.startswith(
dnl                  distutils.sysconfig.project_base) else 1)"])

AC_OUTPUT
