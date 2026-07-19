#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <source_dir>"
  echo "example:"
  echo "  $0 examples"
  exit 1
fi

src_dir="$1"
assembler="./build-profile/assembler/assembler"

if [[ ! -d "$src_dir" ]]; then
  echo "source directory does not exist: $src_dir" >&2
  exit 1
fi

if [[ ! -x "$assembler" ]]; then
  echo "assembler not found or not executable: $assembler" >&2
  exit 1
fi

shopt -s nullglob
sources=("$src_dir"/*.s)

if [[ ${#sources[@]} -eq 0 ]]; then
  echo "no .s files found in: $src_dir" >&2
  exit 1
fi

for src in "${sources[@]}"; do
  obj="${src%.s}.o"
  "$assembler" "$src" "$obj"
done
