#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <output_dir> [prof_file]"
  echo "examples:"
  echo "  $0 profile_res_before"
  echo "  $0 profile_res_before prof_memory_loop"
  echo "  $0 profile_res_before examples/prof_memory_loop.s"
  echo "  $0 profile_res_before examples/prof_memory_loop.o"
  exit 1
fi

out_dir="$1"
target="${2:-}"

mkdir -p "$out_dir"

resolve_sources() {
  if [[ -z "$target" ]]; then
    printf '%s\n' examples/prof_*.s
    return
  fi

  if [[ "$target" == *.s || "$target" == *.o ]]; then
    printf '%s\n' "$target"
    return
  fi

  if [[ -f "examples/${target}.s" ]]; then
    printf '%s\n' "examples/${target}.s"
    return
  fi

  if [[ -f "examples/${target}.o" ]]; then
    printf '%s\n' "examples/${target}.o"
    return
  fi

  echo "unknown prof file: $target" >&2
  exit 1
}

run_one() {
  local input="$1"
  local obj
  local name

  if [[ "$input" == *.s ]]; then
    obj="${input%.s}.o"
    name="$(basename "${input%.s}")"
    ./build-profile/assembler/assembler "$input" "$obj"
  elif [[ "$input" == *.o ]]; then
    obj="$input"
    name="$(basename "$input" .o)"
  else
    echo "unsupported input file: $input" >&2
    exit 1
  fi

  perf stat -r 10 -o "${out_dir}/${name}_perf_stat.txt" \
    -- ./build-profile/emulator/emulator "$obj"
}

while IFS= read -r src; do
  run_one "$src"
done < <(resolve_sources)
