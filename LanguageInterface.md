# Scripting languages #

A Python module named xdelta3main installs via `setup.py`, supports a single method: `xdelta3main.xd3_main_cmdline()`, which runs the `xdelta3` main() without os.fork() and os.execl().

The (Swig) xdelta3 module exports `xdelta3.xd3_encode_memory()`, `xdelta3.xdelta3_decode_memory()`, and `xdelta3.xd3_main_cmdline()`. See `xdelta3-test.py` and `xdelta3-regtest.py` for examples.

```
import xdelta3

# in memory interface
source = 'source source input0 source source'
target = 'source source target source source'

result1, patch = xdelta3.xd3_encode_memory(target, source, len(target))
result2, target1 = xdelta3.xd3_decode_memory(patch, source, len(target))

assert result1 == 0
assert result2 == 0
assert target1 == target
assert len(patch) < len(target)

# command-line interface

source_file = ...
target_file = ...
command_args = ['xdelta3', '-f', '-s', source_file, target_file, output_file]

xdelta3.xd3_main_cmdline(command_args)

# same as
# xdelta3main.xd3_main_cmdline(command_args)
```