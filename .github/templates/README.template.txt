Xdelta3 {{PLATFORM}} Binary
======================

This package contains the xdelta3 command-line utility for {{PLATFORM}}.

Command Line Syntax
-------------------

make patch:

  {{EXECUTABLE_NAME}} -e -s old_file new_file delta_file

apply patch:

  {{EXECUTABLE_NAME}} -d -s old_file delta_file decoded_new_file

standard options:
   -0 .. -9     compression level
   -c           use stdout
   -d           decompress
   -e           compress
   -f           force (overwrite, ignore trailing garbage)
   -h           show help
   -q           be quiet
   -v           be verbose (max 2)
   -V           show version

For full documentation, run: {{EXECUTABLE_NAME}} --help
