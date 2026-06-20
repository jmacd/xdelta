# Language interfaces

!!! note "Historical"
    The bindings described here date from the Xdelta 3.0 era and are not built
    or tested by the current CMake-based sources. They are documented for
    reference. For embedding xdelta3 today, use the C API directly (see the
    [Programming guide](programming-guide.md)).

## Scripting languages

A Python module named `xdelta3main` installed via `setup.py` and supported a
single method, `xdelta3main.xd3_main_cmdline()`, which ran the `xdelta3`
`main()` without `os.fork()` / `os.execl()`.

The [SWIG](https://www.swig.org/)-generated `xdelta3` module exported
`xdelta3.xd3_encode_memory()`, `xdelta3.xd3_decode_memory()`, and
`xdelta3.xd3_main_cmdline()`. The historical `xdelta3-test.py` and
`xdelta3-regtest.py` showed example usage:

```python
import xdelta3

# in-memory interface
source = 'source source input0 source source'
target = 'source source target source source'

result1, patch   = xdelta3.xd3_encode_memory(target, source, len(target))
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
# same as xdelta3main.xd3_main_cmdline(command_args)
```
