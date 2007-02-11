# Configure paths for XDELTA
# Owen Taylor     97-11-3

dnl AM_PATH_XDELTA([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for XDELTA, and define XDELTA_CFLAGS and XDELTA_LIBS, if "gmodule" or
dnl
AC_DEFUN(AM_PATH_XDELTA,
[dnl
dnl Get the cflags and libraries from the xdelta-config script
dnl
AC_ARG_WITH(xdelta-prefix,[  --with-xdelta-prefix=PFX   Prefix where XDELTA is installed (optional)],
            xdelta_config_prefix="$withval", xdelta_config_prefix="")
AC_ARG_WITH(xdelta-exec-prefix,[  --with-xdelta-exec-prefix=PFX Exec prefix where XDELTA is installed (optional)],
            xdelta_config_exec_prefix="$withval", xdelta_config_exec_prefix="")
AC_ARG_ENABLE(xdeltatest, [  --disable-xdeltatest       Do not try to compile and run a test XDELTA program],
		    , enable_xdeltatest=yes)

  if test x$xdelta_config_exec_prefix != x ; then
     xdelta_config_args="$xdelta_config_args --exec-prefix=$xdelta_config_exec_prefix"
     if test x${XDELTA_CONFIG+set} != xset ; then
        XDELTA_CONFIG=$xdelta_config_exec_prefix/bin/xdelta-config
     fi
  fi
  if test x$xdelta_config_prefix != x ; then
     xdelta_config_args="$xdelta_config_args --prefix=$xdelta_config_prefix"
     if test x${XDELTA_CONFIG+set} != xset ; then
        XDELTA_CONFIG=$xdelta_config_prefix/bin/xdelta-config
     fi
  fi

  AC_PATH_PROG(XDELTA_CONFIG, xdelta-config, no)
  min_xdelta_version=ifelse([$1], ,1.0.0,$1)
  AC_MSG_CHECKING(for XDELTA - version >= $min_xdelta_version)
  no_xdelta=""
  if test "$XDELTA_CONFIG" = "no" ; then
    no_xdelta=yes
  else
    XDELTA_CFLAGS=`$XDELTA_CONFIG $xdelta_config_args --cflags`
    XDELTA_LIBS=`$XDELTA_CONFIG $xdelta_config_args --libs`
    xdelta_config_major_version=`$XDELTA_CONFIG $xdelta_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    xdelta_config_minor_version=`$XDELTA_CONFIG $xdelta_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    xdelta_config_micro_version=`$XDELTA_CONFIG $xdelta_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_xdeltatest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $XDELTA_CFLAGS"
      LIBS="$LIBS $XDELTA_LIBS"
dnl
dnl Now check if the installed XDELTA is sufficiently new. (Also sanity
dnl checks the results of xdelta-config to some extent
dnl
      rm -f conf.xdeltatest
      AC_TRY_RUN([
#include <xdelta.h>
#include <stdio.h>
#include <stdlib.h>

int
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.xdeltatest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_xdelta_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_xdelta_version");
     exit(1);
   }

  if ((xdelta_major_version != $xdelta_config_major_version) ||
      (xdelta_minor_version != $xdelta_config_minor_version) ||
      (xdelta_micro_version != $xdelta_config_micro_version))
    {
      printf("\n*** 'xdelta-config --version' returned %d.%d.%d, but XDELTA (%d.%d.%d)\n",
             $xdelta_config_major_version, $xdelta_config_minor_version, $xdelta_config_micro_version,
             xdelta_major_version, xdelta_minor_version, xdelta_micro_version);
      printf ("*** was found! If xdelta-config was correct, then it is best\n");
      printf ("*** to remove the old version of XDELTA. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If xdelta-config was wrong, set the environment variable XDELTA_CONFIG\n");
      printf("*** to point to the correct copy of xdelta-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    }
  else if ((xdelta_major_version != XDELTA_MAJOR_VERSION) ||
	   (xdelta_minor_version != XDELTA_MINOR_VERSION) ||
           (xdelta_micro_version != XDELTA_MICRO_VERSION))
    {
      printf("*** XDELTA header files (version %d.%d.%d) do not match\n",
	     XDELTA_MAJOR_VERSION, XDELTA_MINOR_VERSION, XDELTA_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     xdelta_major_version, xdelta_minor_version, xdelta_micro_version);
    }
  else
    {
      if ((xdelta_major_version > major) ||
        ((xdelta_major_version == major) && (xdelta_minor_version > minor)) ||
        ((xdelta_major_version == major) && (xdelta_minor_version == minor) && (xdelta_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of XDELTA (%d.%d.%d) was found.\n",
               xdelta_major_version, xdelta_minor_version, xdelta_micro_version);
        printf("*** You need a version of XDELTA newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** XDELTA is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the xdelta-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of XDELTA, but you can also set the XDELTA_CONFIG environment to point to the\n");
        printf("*** correct copy of xdelta-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_xdelta=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_xdelta" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$XDELTA_CONFIG" = "no" ; then
       echo "*** The xdelta-config script installed by XDELTA could not be found"
       echo "*** If XDELTA was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the XDELTA_CONFIG environment variable to the"
       echo "*** full path to xdelta-config."
     else
       if test -f conf.xdeltatest ; then
        :
       else
          echo "*** Could not run XDELTA test program, checking why..."
          CFLAGS="$CFLAGS $XDELTA_CFLAGS"
          LIBS="$LIBS $XDELTA_LIBS"
          AC_TRY_LINK([
#include <xdelta.h>
#include <stdio.h>
],      [ return ((xdelta_major_version) || (xdelta_minor_version) || (xdelta_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding XDELTA or finding the wrong"
          echo "*** version of XDELTA. If it is not finding XDELTA, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means XDELTA was incorrectly installed"
          echo "*** or that you have moved XDELTA since it was installed. In the latter case, you"
          echo "*** may want to edit the xdelta-config script: $XDELTA_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     XDELTA_CFLAGS=""
     XDELTA_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(XDELTA_CFLAGS)
  AC_SUBST(XDELTA_LIBS)
  rm -f conf.xdeltatest
])
