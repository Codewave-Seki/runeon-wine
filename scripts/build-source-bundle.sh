#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

[[ $# -eq 2 ]] || die "usage: $0 PATCHED_WINE_SOURCE_ROOT OUTPUT_DIRECTORY"
source_root="$(cd "$1" && pwd)"
output_dir="$2"

load_base_manifest
require_release_tag_checkout_if_published
"$script_dir/verify-source.sh" "$source_root" >/dev/null

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"
temp_parent="${TMPDIR:-/tmp}"
temp_root="$(mktemp -d "$temp_parent/runeon-wine-source-bundle.XXXXXX")"
bundle_name="runeon-wine-source-$patch_set_id"
bundle_root="$temp_root/$bundle_name"
archive="$output_dir/$bundle_name.tar.gz"
checksum="$archive.sha256"

cleanup() {
  rm -rf "$temp_root"
}
trap cleanup EXIT INT TERM

mkdir -p "$bundle_root"
if command -v ditto >/dev/null 2>&1; then
  ditto "$source_root" "$bundle_root/wine"
else
  cp -a "$source_root" "$bundle_root/wine"
fi
rm -rf "$bundle_root/wine/.git"

cp -R "$repo_root/base" "$bundle_root/base"
cp -R "$repo_root/patches" "$bundle_root/patches"
cp "$patch_manifest" "$bundle_root/patches/manifest.json"
cp -R "$repo_root/scripts" "$bundle_root/scripts"
cp "$repo_root/LICENSE" "$bundle_root/LICENSE"
cp "$series_file" "$bundle_root/series"
cp "$repo_root/README.md" "$repo_root/BUILDING.md" "$repo_root/MAINTENANCE.md" "$bundle_root/"

jq -n \
  --arg baseId "$base_id" \
  --arg patchSetId "$patch_set_id" \
  --arg baseSourceUrl "$base_source_url" \
  --arg baseSourceSha256 "$base_archive_sha256" \
  '{schemaVersion: 1, baseId: $baseId, patchSetId: $patchSetId, baseSourceUrl: $baseSourceUrl, baseSourceSha256: $baseSourceSha256}' \
  >"$bundle_root/SOURCE-MANIFEST.json"

create_reproducible_tar_gz "$bundle_root" "$archive"
archive_sha256="$(sha256_file "$archive")"
printf '%s  %s\n' "$archive_sha256" "$(basename "$archive")" >"$checksum"

printf '%s\n' "$archive"
printf '%s\n' "$checksum"
