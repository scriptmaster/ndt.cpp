#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_FILE="$ROOT_DIR/archive/export-code-review.txt"

(
  echo "===== PROJECT TREE (src) ====="
  find "$ROOT_DIR/src" -type f \( -name "*.h" -o -name "*.cpp" \) | sed "s|$ROOT_DIR/||" | sort
  echo
  echo "===== FILE CONTENTS ====="
  find "$ROOT_DIR/src" -type f \( -name "*.h" -o -name "*.cpp" \) ! -name "*.o" ! -path "*/build/*" ! -path "*/bin/*" | sed "s|$ROOT_DIR/||" | sort | while read -r f; do
    echo
    echo "=================================================="
    echo "FILE: $f"
    echo "=================================================="
    sed 's/\t/    /g' "$ROOT_DIR/$f"
  done
) > "$OUT_FILE"
