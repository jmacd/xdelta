# Command-line syntax

The command-line syntax is like **gzip**, with the additional option
`-s SOURCE`.

As with gzip, `-d` means decompress and the default mode (`-e`) is to compress;
`-c` writes to standard output and `-f` forces overwrite. Unlike gzip, xdelta3
defaults to stdout instead of choosing a file extension automatically. Without
`-s SOURCE`, xdelta3 behaves like gzip for stdin/stdout purposes.

## Compress (encode) examples

```sh
xdelta3 -e -s SOURCE TARGET > OUT
xdelta3 -e -s SOURCE TARGET OUT
xdelta3 -e -s SOURCE < TARGET > OUT
```

## Decompress (decode) examples

```sh
xdelta3 -d -s SOURCE OUT > TARGET
xdelta3 -d -s SOURCE OUT TARGET
xdelta3 -d -s SOURCE < OUT > TARGET
```

## Usage summary

The following reflects the built-in help (`xdelta3 -h`) for the current
release. Some commands and options depend on build-time configuration.

```
usage: xdelta3 [command/options] [input [output]]
make patch:

  xdelta3 -e -s old_file new_file delta_file

apply patch:

  xdelta3 -d -s old_file delta_file decoded_new_file

special command names:
    config      prints xdelta3 configuration
    decode      decompress the input
    encode      compress the input
    test        run the builtin tests
special commands for VCDIFF inputs:
    printdelta  print information about the entire delta
    printhdr    print information about the first window
    printhdrs   print information about all windows
    recode      encode with new application/secondary settings
    merge       merge VCDIFF inputs (see below)
merge patches:

  xdelta3 merge -m 1.vcdiff -m 2.vcdiff 3.vcdiff merged.vcdiff

standard options:
   -0 .. -9     compression level
   -c           use stdout
   -d           decompress
   -e           compress
   -f           force (overwrite, ignore trailing garbage)
   -F           force the external-compression subprocess
   -h           show help
   -q           be quiet
   -v           be verbose (max 2)
   -V           show version
memory options:
   -B bytes     source window size
   -W bytes     input window size
   -P size      compression duplicates window
   -I size      instruction buffer size (0 = unlimited)
compression options:
   -s source    source file to copy from (if any)
   -S [lzma|djw|fgk] enable/disable secondary compression
   -N           disable small string-matching compression
   -D           disable external decompression (encode/decode)
   -R           disable external recompression (decode)
   -n           disable checksum (encode/decode)
   -a           disable armor (whole-file BLAKE3 verification,
                on by default; requires a seekable source)
   -C           soft config (encode, undocumented)
   -A [apphead] disable/provide application header (encode)
   -J           disable output (check/compute only)
   -m           arguments for "merge"
the XDELTA environment variable may contain extra args:
   XDELTA="-s source-x.y.tar.gz" \
   tar --use-compress-program=xdelta3 \
       -cf target-x.z.tar.gz.vcdiff target-x.y
```

## Notes on selected options

### `-S` secondary compression

`-S lzma` selects [LZMA](https://tukaani.org/xz/) secondary compression (only
available when xdelta3 was built with liblzma); `-S djw` and `-S fgk` select the
built-in coders; `-S none` disables it. See
[Better compression](better-compression.md).

### `-a` armor

Armor mode embeds BLAKE3 digests of the source and target in the delta and
verifies them, so applying a patch against the wrong source fails immediately
with a clear message. It is **on by default** and requires a seekable
(regular) source file. Pass `-a` to disable it and restore the legacy
streaming behavior. See [Armor mode](armor.md).

### `-A` application header

The `-A` flag sets application-specific data in the VCDIFF header (view it with
`xdelta3 printhdr`). By default the application header includes the source and
input filenames plus descriptors used by
[external compression](external-compression.md). Disable it with `-A=`.

### `merge` and `recode`

`recode` re-encodes an existing delta with new application/secondary settings.
`merge` collapses a chain of deltas into one; given deltas `1→2` and `2→3` it
produces a `1→3` delta:

```sh
xdelta3 merge -m 1.vcdiff -m 2.vcdiff 3.vcdiff merged.vcdiff
```

## Exit status

| Code | Meaning |
| ---- | ------- |
| `0`  | success |
| `1`  | generic failure (I/O error, wrong source, malformed input, …) |
| `2`  | armored patch already applied: the supplied source already matches the delta's target digest ("already up to date") |

## See also

- [Tuning the memory budget](tuning-memory.md) for `-B`, `-W`, `-I`, `-P`.
- ["Target window checksum mismatch" explained](checksum-mismatch.md).
