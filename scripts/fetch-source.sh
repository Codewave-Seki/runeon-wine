#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

work_root="${RUNEON_WINE_WORK_ROOT:-$repo_root/.work}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --work-root)
      [[ $# -ge 2 ]] || die "--work-root requires a path"
      work_root="$2"
      shift 2
      ;;
    -h|--help)
      printf 'usage: %s [--work-root PATH]\n' "$0"
      exit 0
      ;;
    *) die "unknown argument: $1" ;;
  esac
done

load_base_manifest

archive_dir="$work_root/downloads"
extract_root="$work_root/extracted/$base_id"
archive="$archive_dir/$base_archive_name"
source_root="$extract_root/$base_source_root"
provenance_file="$extract_root/.runeon-base-source-sha256"

mkdir -p "$archive_dir" "$extract_root"

if [[ ! -f "$archive" ]]; then
  require_command curl
  printf 'downloading %s\n' "$base_source_url" >&2
  curl --fail --location --progress-bar "$base_source_url" -o "$archive"
fi

actual_sha256="$(sha256_file "$archive")"
[[ "$actual_sha256" == "$base_archive_sha256" ]] \
  || die "base archive SHA-256 mismatch: expected $base_archive_sha256, got $actual_sha256"

if [[ ! -d "$source_root" ]]; then
  require_command tar
  tar -xzf "$archive" -C "$extract_root" "$base_source_root"
  printf '%s\n' "$actual_sha256" >"$provenance_file"
fi

[[ -f "$provenance_file" ]] \
  || die "cached source has no provenance marker: $provenance_file"
[[ "$(<"$provenance_file")" == "$base_archive_sha256" ]] \
  || die "cached source provenance does not match the manifest"

assert_wine_source_root "$source_root"
printf '%s\n' "$source_root"

