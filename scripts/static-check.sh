#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

load_base_manifest
require_command jq
require_command perl
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

for english_document in "$repo_root"/*.md; do
  case "$english_document" in
    *.zh-CN.md|*.ja.md) continue ;;
  esac

  document_stem="${english_document%.md}"
  chinese_document="$document_stem.zh-CN.md"
  japanese_document="$document_stem.ja.md"
  english_name="$(basename "$english_document")"
  chinese_name="$(basename "$chinese_document")"
  japanese_name="$(basename "$japanese_document")"

  require_file "$chinese_document"
  require_file "$japanese_document"

  for document in "$english_document" "$chinese_document" "$japanese_document"; do
    grep -Fq "$english_name" "$document" \
      || die "documentation language link is missing $english_name: $document"
    grep -Fq "$chinese_name" "$document" \
      || die "documentation language link is missing $chinese_name: $document"
    grep -Fq "$japanese_name" "$document" \
      || die "documentation language link is missing $japanese_name: $document"
  done

  if perl -CSD -ne '$found ||= /\p{Han}|\p{Hiragana}|\p{Katakana}/; END { exit($found ? 0 : 1) }' "$english_document"; then
    die "authoritative English documentation contains Chinese or Japanese text: $english_document"
  fi
  perl -CSD -ne '$found ||= /\p{Han}/; END { exit($found ? 0 : 1) }' "$chinese_document" \
    || die "Chinese documentation does not contain Chinese text: $chinese_document"
  perl -CSD -ne '$found ||= /\p{Hiragana}|\p{Katakana}/; END { exit($found ? 0 : 1) }' "$japanese_document" \
    || die "Japanese documentation does not contain Japanese text: $japanese_document"
done

for translated_document in "$repo_root"/*.zh-CN.md "$repo_root"/*.ja.md; do
  case "$translated_document" in
    *.zh-CN.md) english_document="${translated_document%.zh-CN.md}.md" ;;
    *.ja.md) english_document="${translated_document%.ja.md}.md" ;;
  esac
  require_file "$english_document"
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

# The marked README line identifies the active source definition, which may be
# an unshipped candidate. Shipped source status is recorded separately. Historical
# patch-set checkouts do not have to match the current README definition.
if [[ -z "${RUNEON_WINE_PATCHSET_DEFINITION:-}" ]]; then
  for readme in "$repo_root"/README.md "$repo_root"/README.zh-CN.md "$repo_root"/README.ja.md; do
    require_file "$readme"
    marker_count="$(grep -cF '<!-- release-facts:current-patch-set -->' "$readme" || true)"
    [[ "$marker_count" == "1" ]] \
      || die "README must carry exactly one release-facts:current-patch-set marker: $readme"
    current_source_line="$(grep -A1 -F '<!-- release-facts:current-patch-set -->' "$readme" | tail -n 1)"
    grep -Fq "$patch_set_id" <<<"$current_source_line" \
      || die "README active source must name $patch_set_id: $readme"
    stale_patch_set_ids="$(grep -oE 'cx[0-9]+\.[0-9]+-wine[0-9]+\.[0-9]+-runeon\.[0-9]+' <<<"$current_source_line" \
      | grep -Fxv "$patch_set_id" || true)"
    [[ -z "$stale_patch_set_ids" ]] \
      || die "README active source also names a stale patch set: $readme"
  done
fi

printf 'static checks passed for %s\n' "$patch_set_id"
