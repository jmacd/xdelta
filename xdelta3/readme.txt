Xdelta 3.x readme.txt
Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007
<josh.macdonald@gmail.com>


Thanks for downloading Xdelta!

This directory contains the Xdelta3 command-line interface (CLI) and source
distribution for VCDIFF differential compression, a.k.a. delta
compression. The latest information and downloads are available here:

  http://xdelta.org/
  http://code.google.com/p/xdelta/

The command-line syntax:

  http://code.google.com/p/xdelta/wiki/CommandLineSyntax

Run 'xdelta3 -h' for brief help.  Run 'xdelta3 test' for built-in tests.

Sample commands (like gzip, -e means encode, -d means decode)

  xdelta3 -9 -S djw -e -vfs OLD_FILE NEW_FILE DELTA_FILE
  xdelta3 -d -vfs OLD_FILE DELTA_FILE DECODED_FILE

File bug reports and browse open support issues here:

  http://code.google.com/p/xdelta/issues/list

The source distribution contains the C/C++/Python APIs, Unix, Microsoft VC++
and Cygwin builds.  Xdelta3 is covered under the terms of the GPL, see
COPYING.

Commercial inquiries welcome, please contact <josh.macdonald@gmail.com>
