#!/usr/bin/env bash
#
# Format (or check) xdelta3's own C/C++ sources with clang-format.
#
# Usage:
#   ./format.sh            Reformat files in place.
#   ./format.sh --check    Report files that are not formatted; non-zero exit
#                          if any differ.  Does not modify files.
#
# The Objective-C iOS example is intentionally excluded.
set -euo pipefail

cd "$(dirname "$0")"

CLANG_FORMAT="${CLANG_FORMAT:-}"
if [[ -z "$CLANG_FORMAT" ]]; then
  for c in clang-format clang-format-20 \
           /opt/homebrew/opt/llvm@20/bin/clang-format \
           /usr/local/opt/llvm@20/bin/clang-format; do
    if command -v "$c" >/dev/null 2>&1; then
      CLANG_FORMAT="$c"
      break
    fi
  done
fi
if [[ -z "$CLANG_FORMAT" ]]; then
  echo "error: clang-format not found; set CLANG_FORMAT to its path" >&2
  exit 1
fi

FILES=()
while IFS= read -r f; do
  FILES+=("$f")
done < <(
  git ls-files '*.c' '*.h' '*.cc' '*.cpp' '*.hpp' \
    | grep -vE '^examples/iOS/'
)

if [[ "${1:-}" == "--check" ]]; then
  "$CLANG_FORMAT" --style=file --dry-run --Werror "${FILES[@]}"
  echo "All files are correctly formatted."
else
  # clang-format is occasionally non-idempotent on long trailing macro
  # comments, so run a second pass to reach a stable fixed point.
  "$CLANG_FORMAT" --style=file -i "${FILES[@]}"
  "$CLANG_FORMAT" --style=file -i "${FILES[@]}"
  echo "Formatted ${#FILES[@]} files."
fi
