#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

[[ $# -eq 1 ]] || die "usage: $0 WINE_SOURCE_ROOT"
source_root="$(cd "$1" && pwd)"

load_base_manifest
assert_wine_source_root "$source_root"

marker="$source_root/.runeon-patchset.json"
require_file "$marker"
[[ "$(json_value "$marker" '.baseId')" == "$base_id" ]] || die "patch marker base ID mismatch"
[[ "$(json_value "$marker" '.patchSetId')" == "$patch_set_id" ]] || die "patch marker patch-set ID mismatch"
[[ "$(json_value "$marker" '.baseSourceSha256')" == "$base_archive_sha256" ]] \
  || die "patch marker base SHA-256 mismatch"

for version_file in \
  "$source_root/dlls/msvcp140/version.rc" \
  "$source_root/dlls/msvcp140_2/version.rc" \
  "$source_root/dlls/vcruntime140_1/version.rc"
do
  grep -Fq '#define WINE_FILEVERSION 14,50,35719,0' "$version_file" \
    || die "VC14 builtin version marker missing: $version_file"
done

process_source="$source_root/dlls/kernelbase/process.c"
for marker_text in \
  'append_steamwebhelper_inprocess_gpu' \
  'append_socialclubhelper_inprocess_gpu' \
  'append_socialclubhelper_unsafe_swiftshader' \
  'append_socialclubhelper_use_angle_swiftshader'
do
  grep -Fq "$marker_text" "$process_source" || die "process patch marker missing: $marker_text"
done

cfgmgr32_source="$source_root/dlls/cfgmgr32/main.c"
cfgmgr32_tests="$source_root/dlls/cfgmgr32/tests/cfgmgr32.c"
grep -Fq '#define CM_NOTIFY_CONTEXT_MAGIC 0xbeef4dad' "$cfgmgr32_source" \
  || die "cfgmgr32 notification magic marker missing"
grep -Fq '__EXCEPT_PAGE_FAULT' "$cfgmgr32_source" \
  || die "cfgmgr32 invalid-pointer exception guard missing"
grep -Fq 'valid pointer but not a handle' "$cfgmgr32_tests" \
  || die "cfgmgr32 invalid-handle upstream test missing"
grep -Fq '(HCMNOTIFICATION)0xdeadbeef' "$cfgmgr32_tests" \
  || die "cfgmgr32 page-fault upstream test missing"

printf 'verified %s on %s\n' "$patch_set_id" "$base_id"

