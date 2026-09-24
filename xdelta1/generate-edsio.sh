#!/bin/sh

set -eu

emacs_bin=${EMACS:-emacs}
srcdir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if ! command -v "$emacs_bin" >/dev/null 2>&1; then
  echo "error: Emacs executable not found: $emacs_bin" >&2
  exit 1
fi

(
  cd "$srcdir/libedsio"
  "$emacs_bin" --batch -Q -l edsio.el \
    --eval '(generate-ser "edsio.ser" "edsio")'
)

(
  cd "$srcdir"
  "$emacs_bin" --batch -Q -l libedsio/edsio.el \
    --eval '(generate-ser "xd.ser" "xd")'
)
