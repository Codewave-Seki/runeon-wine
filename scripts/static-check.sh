#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

load_base_manifest
require_command jq

jq -e . "$base_manifest" "$patch_manifest" >/dev/null
[[ "$(json_value "$patch_manifest" '.baseId')" == "$base_id" ]] \
  || die "base ID differs between base and patch manifests"
[[ "$(json_value "$patch_manifest" '.patchSetId')" == "$patch_set_id" ]] \
  || die "patch-set ID differs between base and patch manifests"

series_paths="$(sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*$/d' "$repo_root/series")"
manifest_paths="$(jq -r '.patches | sort_by(.order)[] | .path' "$patch_manifest")"
[[ "$series_paths" == "$manifest_paths" ]] || die "series and patch manifest order differ"

while IFS= read -r relative_patch || [[ -n "$relative_patch" ]]; do
  [[ -n "$relative_patch" ]] || continue
  patch_file="$repo_root/$relative_patch"
  require_file "$patch_file"
  expected_sha256="$(jq -er --arg path "$relative_patch" '.patches[] | select(.path == $path) | .sha256' "$patch_manifest")"
  [[ "$(sha256_file "$patch_file")" == "$expected_sha256" ]] \
    || die "patch SHA-256 mismatch: $relative_patch"
done <<<"$series_paths"

for shell_script in "$repo_root"/scripts/*.sh; do
  bash -n "$shell_script"
done

printf 'static checks passed for %s\n' "$patch_set_id"

