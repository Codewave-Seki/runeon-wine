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

# Documents are a fixed set: review records and patch notes are sections of the
# two files under docs/, in English. Only the README is translated.
for root_document in "$repo_root"/*.md; do
  case "$(basename "$root_document")" in
    README.md|README.zh-CN.md|README.ja.md|BUILDING.md|MAINTENANCE.md|SECURITY.md|CLAUDE.md|AGENTS.md) ;;
    *) die "unexpected root document (add a section to a docs/ file instead): $root_document" ;;
  esac
done
while IFS= read -r docs_document; do
  case "${docs_document#"$repo_root"/}" in
    docs/upstream-reviews.md|docs/patch-notes.md) ;;
    *) die "unexpected document under docs/ (add a section instead): $docs_document" ;;
  esac
done < <(find "$repo_root/docs" -name '*.md')

readme_names=(README.md README.zh-CN.md README.ja.md)
for readme in "${readme_names[@]}"; do
  require_file "$repo_root/$readme"
  for name in "${readme_names[@]}"; do
    grep -Fq "($name)" "$repo_root/$readme" \
      || die "README language link is missing $name: $readme"
  done
done

while IFS= read -r english_document; do
  if perl -CSD -ne '$found ||= /\p{Han}|\p{Hiragana}|\p{Katakana}/; END { exit($found ? 0 : 1) }' "$english_document"; then
    die "English documentation contains Chinese or Japanese text: $english_document"
  fi
done < <(find "$repo_root" -maxdepth 1 -name '*.md' ! -name '*.zh-CN.md' ! -name '*.ja.md'; find "$repo_root/docs" -name '*.md')
perl -CSD -ne '$found ||= /\p{Han}/; END { exit($found ? 0 : 1) }' "$repo_root/README.zh-CN.md" \
  || die "Chinese README does not contain Chinese text"
perl -CSD -ne '$found ||= /\p{Hiragana}|\p{Katakana}/; END { exit($found ? 0 : 1) }' "$repo_root/README.ja.md" \
  || die "Japanese README does not contain Japanese text"

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

# The marked README line names the current stable source: the newest patch set
# whose release manifest is stable.
current_stable_id="$(for manifest in "$repo_root"/release-manifests/*.source-archive.json; do
  [[ "$(json_value "$manifest" '.releaseState')" == "stable" ]] && json_value "$manifest" '.patchSetID'
done | sort -V | tail -n 1)"
for readme in "$repo_root"/README.md "$repo_root"/README.zh-CN.md "$repo_root"/README.ja.md; do
  marker_count="$(grep -cF '<!-- release-facts:current-patch-set -->' "$readme" || true)"
  [[ "$marker_count" == "1" ]] \
    || die "README must carry exactly one release-facts:current-patch-set marker: $readme"
  current_source_line="$(grep -A1 -F '<!-- release-facts:current-patch-set -->' "$readme" | tail -n 1)"
  grep -Fq "$current_stable_id" <<<"$current_source_line" \
    || die "README current source must name $current_stable_id: $readme"
  stale_patch_set_ids="$(grep -oE 'cx[0-9]+\.[0-9]+-wine[0-9]+\.[0-9]+-runeon\.[0-9]+' <<<"$current_source_line" \
    | grep -Fxv "$current_stable_id" || true)"
  [[ -z "$stale_patch_set_ids" ]] \
    || die "README current source also names another patch set: $readme"
done

python3 "$repo_root/tests/verification-marker-check.py" >/dev/null \
  || die "verification marker self-test failed: tests/verification-marker-check.py"

printf 'static checks passed for %s\n' "$patch_set_id"
