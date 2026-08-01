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
if [[ "$latest_tag" != "$upstream_audit_through" ]]; then
  die "new upstream Wine tag detected: audited through $upstream_audit_through, latest is $latest_tag"
fi

printf 'upstream audit is current through %s\n' "$upstream_audit_through"

