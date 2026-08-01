#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

load_base_manifest
require_command jq
require_file "$repo_root/LICENSE"

jq -e . "$base_manifest" "$patch_manifest" >/dev/null
[[ "$(json_value "$patch_manifest" '.baseId')" == "$base_id" ]] \
  || die "base ID differs between base and patch manifests"
[[ "$(json_value "$patch_manifest" '.patchSetId')" == "$patch_set_id" ]] \
  || die "patch-set ID differs between base and patch manifests"

series_paths="$(sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*$/d' "$series_file")"
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

for release_manifest in "$repo_root"/release-manifests/*.source-archive.json; do
  require_file "$release_manifest"
  [[ "$(json_value "$release_manifest" '.repositoryVisibility')" == "public" ]] \
    || die "release manifest must record repositoryVisibility=public: $release_manifest"
  [[ "$(json_value "$release_manifest" '.availability')" == "public-release" ]] \
    || die "release manifest must record availability=public-release: $release_manifest"
  [[ "$(json_value "$release_manifest" '.retention')" == "permanent" ]] \
    || die "release manifest must record retention=permanent: $release_manifest"
  release_state="$(json_value "$release_manifest" '.releaseState')"
  [[ "$release_state" == "stable" || "$release_state" == "prerelease" ]] \
    || die "release manifest must record stable or prerelease state: $release_manifest"
  release_url="$(json_value "$release_manifest" '.releaseURL')"
  [[ "$release_url" == "https://github.com/Codewave-Seki/runeon-wine/releases/tag/"* ]] \
    || die "release manifest must use the canonical public GitHub release URL: $release_manifest"
done

printf 'static checks passed for %s\n' "$patch_set_id"
