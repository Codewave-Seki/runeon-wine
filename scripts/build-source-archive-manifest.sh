#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

[[ $# -eq 3 ]] || die "usage: $0 PATCHSET_ARCHIVE SOURCE_ARCHIVE OUTPUT_JSON"
patchset_archive="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
source_archive="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
output_json="$3"
release_state="${RUNEON_WINE_RELEASE_STATE:-prerelease}"

case "$release_state" in
  stable|prerelease) ;;
  *) die "RUNEON_WINE_RELEASE_STATE must be stable or prerelease" ;;
esac

load_base_manifest
require_file "$patchset_archive"
require_file "$source_archive"
require_command git
require_command jq

commit="$(git -C "$repo_root" rev-parse HEAD)"
patchset_sha256="$(sha256_file "$patchset_archive")"
source_sha256="$(sha256_file "$source_archive")"
patchset_size="$(stat -f%z "$patchset_archive" 2>/dev/null || stat -c%s "$patchset_archive")"
source_size="$(stat -f%z "$source_archive" 2>/dev/null || stat -c%s "$source_archive")"

mkdir -p "$(dirname "$output_json")"
jq -n \
  --arg patchSetID "$patch_set_id" \
  --arg commit "$commit" \
  --arg patchsetFile "$(basename "$patchset_archive")" \
  --arg patchsetSHA "$patchset_sha256" \
  --argjson patchsetSize "$patchset_size" \
  --arg sourceFile "$(basename "$source_archive")" \
  --arg sourceSHA "$source_sha256" \
  --argjson sourceSize "$source_size" \
  --arg releaseURL "https://github.com/Codewave-Seki/runeon-wine/releases/tag/$patch_set_id" \
  --arg releaseState "$release_state" \
  '{
    schemaVersion: 1,
    patchSetID: $patchSetID,
    commit: $commit,
    repository: "Codewave-Seki/runeon-wine",
    repositoryVisibility: "public",
    availability: "public-release",
    releaseURL: $releaseURL,
    releaseState: $releaseState,
    retention: "permanent",
    assets: {
      "patch-set": {fileName: $patchsetFile, sha256: $patchsetSHA, sizeBytes: $patchsetSize},
      "corresponding-source": {fileName: $sourceFile, sha256: $sourceSHA, sizeBytes: $sourceSize}
    }
  }' >"$output_json"

printf '%s\n' "$output_json"
