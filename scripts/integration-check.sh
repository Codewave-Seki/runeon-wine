#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

[[ $# -eq 1 ]] || die "usage: $0 UNPATCHED_WINE_SOURCE_ROOT"
input_source_root="$(cd "$1" && pwd)"

load_base_manifest
assert_wine_source_root "$input_source_root"

temp_parent="${TMPDIR:-/tmp}"
temp_root="$(mktemp -d "$temp_parent/runeon-wine-integration.XXXXXX")"
candidate_root="$temp_root/wine"
cleanup() {
  rm -rf "$temp_root"
}
trap cleanup EXIT INT TERM

if command -v ditto >/dev/null 2>&1; then
  ditto "$input_source_root" "$candidate_root"
else
  cp -a "$input_source_root" "$candidate_root"
fi

"$script_dir/apply-series.sh" "$candidate_root"
"$script_dir/verify-source.sh" "$candidate_root"

if "$script_dir/apply-series.sh" "$candidate_root" >"$temp_root/reapply.stdout" 2>"$temp_root/reapply.stderr"; then
  die "a second patch application unexpectedly succeeded"
fi
grep -Fq 'patch set is already recorded' "$temp_root/reapply.stderr" \
  || die "reapply failure did not use the explicit patch-set marker gate"

printf 'integration checks passed for %s\n' "$patch_set_id"
