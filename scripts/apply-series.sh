#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

[[ $# -eq 1 ]] || die "usage: $0 WINE_SOURCE_ROOT"
source_root="$(cd "$1" && pwd)"

load_base_manifest
assert_wine_source_root "$source_root"
require_command patch

marker="$source_root/.runeon-patchset.json"
[[ ! -e "$marker" ]] || die "patch set is already recorded at $marker"

while IFS= read -r relative_patch || [[ -n "$relative_patch" ]]; do
  case "$relative_patch" in
    ''|'#'*) continue ;;
  esac

  patch_file="$repo_root/$relative_patch"
  require_file "$patch_file"
  expected_sha256="$(jq -er --arg path "$relative_patch" '.patches[] | select(.path == $path) | .sha256' "$patch_manifest")"
  actual_sha256="$(sha256_file "$patch_file")"
  [[ "$actual_sha256" == "$expected_sha256" ]] \
    || die "patch SHA-256 mismatch for $relative_patch"

  if patch --directory "$source_root" --strip 1 --forward --force --dry-run --silent <"$patch_file"; then
    printf 'applying %s\n' "$relative_patch" >&2
    patch --directory "$source_root" --strip 1 --forward --force <"$patch_file"
  elif patch --directory "$source_root" --strip 1 --reverse --force --dry-run --silent <"$patch_file"; then
    die "patch is already applied but the patch-set marker is missing: $relative_patch"
  else
    die "patch does not apply cleanly to $base_id: $relative_patch"
  fi
done <"$repo_root/series"

jq -n \
  --arg baseId "$base_id" \
  --arg patchSetId "$patch_set_id" \
  --arg baseSourceSha256 "$base_archive_sha256" \
  '{schemaVersion: 1, baseId: $baseId, patchSetId: $patchSetId, baseSourceSha256: $baseSourceSha256}' \
  >"$marker"

"$script_dir/verify-source.sh" "$source_root"

