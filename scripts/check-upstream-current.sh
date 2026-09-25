#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

load_base_manifest
require_command git

latest_tag="$(
  git ls-remote --tags "$upstream_repository" 'refs/tags/wine-11.*' \
    | awk '{sub("refs/tags/", "", $2); sub(/\^\{\}$/, "", $2); print $2}' \
    | sort -u -V \
    | tail -1
)"

[[ -n "$latest_tag" ]] || die "could not resolve the latest Wine 11.x tag"

# A sweep never replaces the complete audit boundary; it only records that
# newer tags were reviewed for backports, so the alert waits for the next tag.
reviewed_through="$upstream_audit_through"
if [[ -n "$upstream_sweep_through" ]] \
  && [[ "$(printf '%s\n%s\n' "$upstream_audit_through" "$upstream_sweep_through" | sort -V | tail -1)" == "$upstream_sweep_through" ]]; then
  reviewed_through="$upstream_sweep_through"
fi

if [[ "$latest_tag" != "$reviewed_through" ]]; then
  die "new upstream Wine tag detected: audited through $upstream_audit_through, swept through ${upstream_sweep_through:-none}, latest is $latest_tag"
fi

printf 'upstream reviewed through %s (complete audit through %s)\n' "$reviewed_through" "$upstream_audit_through"

