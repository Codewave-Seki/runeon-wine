#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
base_manifest="$repo_root/base/crossover-26.3-wine-11.0.json"
patchset_definition="${RUNEON_WINE_PATCHSET_DEFINITION:-}"
if [[ -n "$patchset_definition" ]]; then
  patchset_definition="$(cd "$repo_root" && cd "$(dirname "$patchset_definition")" && pwd)/$(basename "$patchset_definition")"
  patch_manifest="$patchset_definition/manifest.json"
  series_file="$patchset_definition/series"
else
  patch_manifest="$repo_root/patches/manifest.json"
  series_file="$repo_root/series"
fi

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

require_file() {
  [[ -f "$1" ]] || die "required file not found: $1"
}

json_value() {
  local file="$1"
  local expression="$2"

  require_command jq
  jq -er "$expression" "$file"
}

sha256_file() {
  local file="$1"

  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$file" | awk '{print $1}'
  elif command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" | awk '{print $1}'
  else
    die "shasum or sha256sum is required"
  fi
}

load_base_manifest() {
  require_file "$base_manifest"
  require_file "$patch_manifest"
  require_file "$series_file"

  base_id="$(json_value "$base_manifest" '.baseId')"
  patch_set_id="$(json_value "$patch_manifest" '.patchSetId')"
  base_source_url="$(json_value "$base_manifest" '.sourceUrl')"
  base_archive_name="$(json_value "$base_manifest" '.sourceArchiveName')"
  base_archive_sha256="$(json_value "$base_manifest" '.sourceArchiveSha256')"
  base_source_root="$(json_value "$base_manifest" '.sourceRoot')"
  base_wine_version_line="$(json_value "$base_manifest" '.wineVersionLine')"
  upstream_base_tag="$(json_value "$base_manifest" '.upstreamBaseTag')"
  upstream_audit_through="$(json_value "$base_manifest" '.upstreamAuditThrough')"
  upstream_repository="$(json_value "$base_manifest" '.upstreamRepository')"
  bundle_timestamp="$(json_value "$base_manifest" '.bundleTimestamp')"
}

require_release_tag_checkout_if_published() {
  local tag_commit head_commit

  require_command git
  if ! tag_commit="$(git -C "$repo_root" rev-parse --verify "refs/tags/$patch_set_id^{commit}" 2>/dev/null)"; then
    return 0
  fi
  head_commit="$(git -C "$repo_root" rev-parse HEAD)"
  [[ "$head_commit" == "$tag_commit" ]] \
    || die "published patch set $patch_set_id must be rebuilt from its immutable tag checkout"
}

create_reproducible_tar_gz() {
  local bundle_root="$1"
  local archive="$2"
  local bundle_parent bundle_name file_list

  require_command gzip
  require_command tar
  bundle_parent="$(cd "$(dirname "$bundle_root")" && pwd)"
  bundle_name="$(basename "$bundle_root")"
  file_list="$bundle_parent/.runeon-archive-files"

  TZ=UTC find "$bundle_root" -exec touch -h -t "$bundle_timestamp" {} +
  (
    cd "$bundle_parent"
    LC_ALL=C find "$bundle_name" -print | LC_ALL=C sort >"$file_list"
  )

  if tar --version 2>/dev/null | grep -qi 'bsdtar'; then
    tar --no-recursion --uid 0 --gid 0 --uname root --gname root \
      -cf - -C "$bundle_parent" -T "$file_list" | gzip -n >"$archive"
  else
    tar --no-recursion --owner=0 --group=0 --numeric-owner \
      -cf - -C "$bundle_parent" -T "$file_list" | gzip -n >"$archive"
  fi
}

assert_wine_source_root() {
  local source_root="$1"

  [[ -n "$source_root" && "$source_root" != "/" ]] || die "unsafe Wine source root: $source_root"
  require_file "$source_root/VERSION"
  grep -Fxq "$base_wine_version_line" "$source_root/VERSION" \
    || die "Wine source version mismatch at $source_root/VERSION"
  require_file "$source_root/dlls/kernelbase/process.c"
  require_file "$source_root/dlls/cfgmgr32/main.c"
}
