# Command-line reference

Xdelta3 encodes and decodes binary differences in the
[VCDIFF (RFC 3284)](https://www.rfc-editor.org/rfc/rfc3284) format. Its
command-line syntax resembles **gzip**, with the additional `-s SOURCE`
argument:

```text
xdelta3 [command] [options] [input [output]]
```

The default command is `encode`. Unless named, `input` is standard input and
`output` is standard output. The source is always separate and is selected
with `-s`.

## Common operations

Create a delta from `old-file` to `new-file`:

```sh
xdelta3 -e -s old-file new-file update.vcdiff
```

Apply the delta:

```sh
xdelta3 -d -s old-file update.vcdiff reconstructed-file
```

The command names `encode` and `decode` are equivalent to `-e` and `-d`:

```sh
xdelta3 encode -s old-file new-file update.vcdiff
xdelta3 decode -s old-file update.vcdiff reconstructed-file
```

Input and output can be streams:

```sh
xdelta3 -e -s old-file < new-file > update.vcdiff
xdelta3 -d -s old-file < update.vcdiff > reconstructed-file
```

Run xdelta3 from a terminal. With no arguments at an interactive terminal it
displays help. With redirected standard input, no-argument operation remains
an encoding filter:

```sh
xdelta3 < target > target.vcdiff
```

Without `-s`, this produces a VCDIFF encoding that uses only target copies and
literal data. Standard input cannot be both the source (`-s -`) and input.

## Commands

Some commands depend on compile-time features. Run `xdelta3 config` to inspect
the current build.

| Command | Description |
| ------- | ----------- |
| `encode` | Encode the differences between the source and target as a VCDIFF delta. This is the default. |
| `decode` | Apply a VCDIFF delta to its source and reconstruct the target. |
| `config` | Print compile-time features, default buffer sizes, and type sizes. |
| `test` | Run the built-in tests; present only in regression-test builds. |
| `printhdr` | Print the VCDIFF file header and first-window information. |
| `printhdrs` | Print the file header and a summary of every VCDIFF window. |
| `printdelta` | Print the header and every VCDIFF window and instruction. |
| `recode` | Write an equivalent delta with new application-header or secondary-compression settings. |
| `merge` | Collapse an ordered chain of deltas into one delta. |

The inspection commands write human-readable output to standard output. They
do not require a source file. `recode` reads one VCDIFF delta and writes an
equivalent delta; options such as `-A`, `-S`, and `-n` control the new encoding.

### Merging a delta chain

Given deltas `1→2`, `2→3`, and `3→4`, merge them into one `1→4` delta by
passing every delta except the last with `-m`, in order:

```sh
xdelta3 merge \
  -m 1-2.vcdiff \
  -m 2-3.vcdiff \
  3-4.vcdiff \
  1-4.vcdiff
```

When every input is [armored](armor.md), xdelta3 verifies that each target
digest matches the next source digest and armors the merged result.

## Operands, streams, and output files

`input` is the target during encode and the VCDIFF delta during decode,
inspection, recode, or as the final input to `merge`. Use `-` or omit the
operand to read standard input.

`output` is the delta during encode/recode/merge and the reconstructed target
during decode. If it is omitted, xdelta3 may use the filename stored in the
application header; otherwise it writes standard output. `-c` always selects
standard output and overrides an output operand.

During decode, xdelta3 may also use the source filename stored in the
application header when `-s` is omitted. For reliable scripting, specify both
`-s SOURCE` and the output operand explicitly.

Xdelta3 refuses to replace an existing output by default. Pass `-f` to permit
replacement.

## Option reference

### Standard options

| Option | Description |
| ------ | ----------- |
| `-0` … `-9` | Encoder string-matching level. `-0` disables matching, `-1` is fastest, and `-9` is most thorough and memory-intensive. Default: `-3`. |
| `-c` | Write to standard output, overriding any output operand. |
| `-d` | Decode; equivalent to the `decode` command. |
| `-e` | Encode; equivalent to the `encode` command and the default. |
| `-f` | Overwrite an existing output; during decode, also ignore trailing garbage after a valid VCDIFF stream. |
| `-F` | Pass force mode to an external compression/decompression subprocess. |
| `-h` | Display version and usage information. Help exits with status 1. |
| `-q` | Suppress non-error informational output and clear `-v`. |
| `-v` | Increase verbosity; repeat for more encoder statistics and diagnostics. Clears `-q`. |
| `-V` | Display version and license information, then exit. |

### Memory options

Numeric values are decimal byte or entry counts. Suffixes such as `K` and `M`
are not accepted.

| Option | Description |
| ------ | ----------- |
| `-B BYTES` | Source window/cache size. Default: 64 MiB; minimum: 16 KiB. |
| `-W BYTES` | Input (VCDIFF window) size. Default: 8 MiB; minimum: 16 KiB. The build-specific maximum appears in `xdelta3 config`. |
| `-P ENTRIES` | Target duplicate-position table. Default: 262144 entries; normally no larger than `-W`. |
| `-I ENTRIES` | Pending encoder instruction buffer. Default: 32768 entries; `0` means unlimited. |

These settings have important compression-ratio and decoder-memory
implications. See [Tuning the memory budget](tuning-memory.md).

### Compression and format options

| Option | Description |
| ------ | ----------- |
| `-s SOURCE` | Select the source file. It may be specified only once. |
| `-S TYPE` | Select a compiled-in secondary compressor: `lzma`, `djw`, or optional `fgk`. `djw` may have a tuning level (`djw0` … `djw9`). Use `-S=` to disable secondary compression. |
| `-N` | Disable small string matching, leaving literal data and source copies. |
| `-D` | Disable automatic external decompression of recognized inputs. |
| `-R` | Disable automatic external recompression of decoded output. |
| `-G` | Omit the detected external-compression level from the application header, producing a legacy descriptor older xdelta3 versions recognize. |
| `-n` | Omit target-window Adler-32 checksums during encode or skip their verification during decode. |
| `-a` | Disable armor (whole-file BLAKE3 verification), which is otherwise on by default when compiled in. |
| `-A APPHEADER` | Store explicit application-header data. Use `-A=` to disable the application header. |
| `-J` | Process and verify input without writing output. |
| `-m DELTA` | Add a delta to the start of an ordered `merge` chain; repeat for all but the final input delta. |
| `-C VALUES` | Advanced, unstable encoder string-matcher configuration: seven comma-separated nonnegative integers. |

`-F`, `-D`, `-R`, and `-G` have no effect in builds without external
compression. `-a` has no effect when armor is compiled out. Unsupported
secondary compressor names fail with an error. Use `xdelta3 config` to see the
features in a particular binary.

## Detailed option behavior

### Compression level and `-S`

`-0` … `-9` tune VCDIFF string matching; secondary compression is a separate
stage selected by `-S`. When LZMA support is compiled in, LZMA secondary
compression is enabled by default. Otherwise secondary compression defaults to
off. `-S djw`, `-S lzma`, or an FGK-enabled build can make the choice explicit;
`-S=` disables all secondary compression. See
[Better compression](better-compression.md).

### Armor and `-a`

Armor embeds BLAKE3 digests of the logical source and target and verifies them
when decoding. It is on by default and requires seekable source and target
files while encoding. Pass `-a` to use the legacy application-header format
and non-seekable encoding behavior. See [Armor mode](armor.md).

### Application headers and `-A`

By default the application header stores the source and target filenames,
[armor](armor.md) digests, and descriptors used by
[external compression](external-compression.md). View it with
`xdelta3 printhdr`. Supply custom bytes with `-A APPHEADER`, or disable the
header with `-A=`.

Legacy builds may interpret armored `name#digest` fields as literal filenames.
For interoperability, provide explicit `-s` and output operands.

### External compression: `-D`, `-R`, `-F`, and `-G`

On supported platforms, xdelta3 recognizes externally compressed source and
target files, invokes the corresponding decompressor, and records information
needed to recompress the target during decode:

- `-D` processes the compressed bytes directly instead.
- `-R` decodes to uncompressed output.
- `-F` forces the external subprocess.
- `-G` omits the detected compression level from the application header for
  older-decoder compatibility.

External recompression may differ byte-for-byte across compressor versions and
settings. See [External compression](external-compression.md).

### Checksums and verification: `-n` and `-J`

VCDIFF window checksums are enabled by default. `-n` prevents the encoder from
writing them and prevents the decoder from checking them. It does not disable
armor's whole-file verification.

`-J` suppresses output but still performs decoding, checksum validation, and
armor verification. It is useful for validating a delta:

```sh
xdelta3 -J -d -s old-file update.vcdiff
```

### Advanced string matcher: `-C`

`-C` replaces the compression-level preset with seven comma-separated
nonnegative integer fields:

```text
large-look,large-step,small-look,small-chain,small-lazy-chain,max-lazy,long-enough
```

This is an unstable expert interface tied to the current encoder
implementation. Prefer `-0` … `-9` unless reproducing a known configuration.

## Environment

`XDELTA` may contain whitespace-separated arguments that are prepended to the
command line. This is useful when another program invokes xdelta3 as a filter:

```sh
XDELTA="-s source-x.y.tar.gz" \
tar --use-compress-program=xdelta3 \
    -cf target-x.z.tar.gz.vcdiff target-x.y/
```

Values are split on whitespace; shell quoting inside `XDELTA` is not a general
argument-escaping mechanism. Compressor-specific variables such as `GZIP` can
also control external recompression settings.

## Exit status

| Code | Meaning |
| ---- | ------- |
| `0` | Success. |
| `1` | Error, including invalid arguments, I/O failures, wrong sources, malformed input, or displaying help. |
| `2` | An armored patch was already applied: the supplied source matches the recorded target digest, so nothing was written. |

Scripts should distinguish status `2` from both success and generic failure
when applying armored patches.

## Built-in usage summary

`xdelta3 -h` prints a concise, build-specific summary. `xdelta3 config` reports
which optional commands and compressors are available, as well as numeric
defaults and limits. The detailed reference on this page describes the full
source distribution; a reduced build may omit the encoder, test command,
VCDIFF tools, external compression, armor, LZMA, or FGK.
