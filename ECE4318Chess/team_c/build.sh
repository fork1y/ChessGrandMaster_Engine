#!/bin/bash
set -e

if command -v clang >/dev/null 2>&1; then
  CC=clang
elif command -v gcc >/dev/null 2>&1; then
  CC=gcc
else
  echo "No C compiler found (need clang or gcc)." >&2
  exit 1
fi

"$CC" -O2 -std=c11 -Wall -Wextra -o engine src/engine.c
echo "Built ./engine"
