#!/usr/bin/env bash
set -euo pipefail

prof_dir="prof_examples"
assembler="./build-profile/assembler/assembler"
emulator="./build-profile/emulator/emulator"

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <output_dir> [prof_file]"
  echo "examples:"
  echo "  $0 profile_res_before"
  echo "  $0 profile_res_before prof_memory_loop"
  echo "  $0 profile_res_before prof_examples/prof_memory_loop.s"
  echo "  $0 profile_res_before prof_examples/prof_memory_loop.o"
  exit 1
fi

out_dir="$1"
target="${2:-}"

mkdir -p "$out_dir"

assemble_if_needed() {
  local input="$1"

  if [[ "$input" == *.o ]]; then
    printf '%s\n' "$input"
    return
  fi

  local obj="${input%.s}.o"
  "$assembler" "$input" "$obj"
  printf '%s\n' "$obj"
}

run_one() {
  local input="$1"
  local obj
  local name
  local callgrind_out

  obj="$(assemble_if_needed "$input")"
  name="$(basename "$obj" .o)"
  callgrind_out="${out_dir}/callgrind.${name}.out"

  valgrind --tool=callgrind \
    --callgrind-out-file="$callgrind_out" \
    "$emulator" "$obj"

  callgrind_annotate "$callgrind_out" \
    > "${out_dir}/${name}_callgrind"
}

if [[ -z "$target" ]]; then
  shopt -s nullglob
  sources=("$prof_dir"/*.s)
else
  if [[ "$target" == *.s || "$target" == *.o ]]; then
    sources=("$target")
  elif [[ -f "$prof_dir/${target}.s" ]]; then
    sources=("$prof_dir/${target}.s")
  elif [[ -f "$prof_dir/${target}.o" ]]; then
    sources=("$prof_dir/${target}.o")
  else
    echo "unknown prof file: $target" >&2
    exit 1
  fi
fi

if [[ ${#sources[@]} -eq 0 ]]; then
  echo "no prof files found in: $prof_dir" >&2
  exit 1
fi

for src in "${sources[@]}"; do
  run_one "$src"
done
