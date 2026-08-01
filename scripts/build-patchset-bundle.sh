#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

[[ $# -eq 1 ]] || die "usage: $0 OUTPUT_DIRECTORY"
output_dir="$1"

load_base_manifest
"$script_dir/static-check.sh" >/dev/null

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"
temp_parent="${TMPDIR:-/tmp}"
temp_root="$(mktemp -d "$temp_parent/runeon-wine-patchset-bundle.XXXXXX")"
bundle_name="runeon-wine-patchset-$patch_set_id"
bundle_root="$temp_root/$bundle_name"
archive="$output_dir/$bundle_name.tar.gz"
checksum="$archive.sha256"

cleanup() {
  rm -rf "$temp_root"
}
trap cleanup EXIT INT TERM

mkdir -p "$bundle_root"
cp -R "$repo_root/base" "$bundle_root/base"
cp -R "$repo_root/patches" "$bundle_root/patches"
cp -R "$repo_root/scripts" "$bundle_root/scripts"
cp -R "$repo_root/LICENSES" "$bundle_root/LICENSES"
cp "$repo_root/series" "$repo_root/README.md" "$repo_root/BUILDING.md" "$repo_root/MAINTENANCE.md" "$bundle_root/"

jq -n \
  --arg baseId "$base_id" \
  --arg patchSetId "$patch_set_id" \
  --arg baseSourceSha256 "$base_archive_sha256" \
  '{schemaVersion: 1, baseId: $baseId, patchSetId: $patchSetId, baseSourceSha256: $baseSourceSha256}' \
  >"$bundle_root/PATCHSET-MANIFEST.json"

create_reproducible_tar_gz "$bundle_root" "$archive"
archive_sha256="$(sha256_file "$archive")"
printf '%s  %s\n' "$archive_sha256" "$(basename "$archive")" >"$checksum"

printf '%s\n' "$archive"
printf '%s\n' "$checksum"
