#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$script_dir/common.sh"

through="latest"
output=""
work_root="${RUNEON_WINE_WORK_ROOT:-$repo_root/.work}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --through)
      [[ $# -ge 2 ]] || die "--through requires a Wine tag or latest"
      through="$2"
      shift 2
      ;;
    --output)
      [[ $# -ge 2 ]] || die "--output requires a path"
      output="$2"
      shift 2
      ;;
    --work-root)
      [[ $# -ge 2 ]] || die "--work-root requires a path"
      work_root="$2"
      shift 2
      ;;
    -h|--help)
      printf 'usage: %s [--through TAG|latest] [--output PATH] [--work-root PATH]\n' "$0"
      exit 0
      ;;
    *) die "unknown argument: $1" ;;
  esac
done

load_base_manifest
require_command git

latest_tag() {
  git ls-remote --tags "$upstream_repository" 'refs/tags/wine-11.*' \
    | awk '{sub("refs/tags/", "", $2); sub(/\^\{\}$/, "", $2); print $2}' \
    | sort -u -V \
    | tail -1
}

if [[ "$through" == "latest" ]]; then
  through="$(latest_tag)"
  [[ -n "$through" ]] || die "could not resolve the latest Wine 11.x tag"
fi

mirror="$work_root/upstream/wine.git"
mkdir -p "$(dirname "$mirror")"
if [[ ! -d "$mirror" ]]; then
  git clone --bare --filter=blob:none "$upstream_repository" "$mirror"
else
  git -C "$mirror" fetch --prune origin '+refs/tags/wine-11.*:refs/tags/wine-11.*'
fi

git -C "$mirror" rev-parse --verify "$upstream_base_tag^{commit}" >/dev/null \
  || die "upstream base tag not found: $upstream_base_tag"
git -C "$mirror" rev-parse --verify "$through^{commit}" >/dev/null \
  || die "upstream audit tag not found: $through"

if [[ -z "$output" ]]; then
  output="$repo_root/dist/upstream-audit-$upstream_base_tag-to-$through.tsv"
fi
mkdir -p "$(dirname "$output")"

git -C "$mirror" log --reverse --date=short --format='@@@%H%x09%ad%x09%s' --name-only \
  "$upstream_base_tag..$through" \
  | awk '
    function classify(paths) {
      if (paths ~ /(^|,)(dlls\/ntdll\/|server\/|dlls\/wow64|loader\/|dlls\/winemac\.drv\/|dlls\/win32u\/|include\/wine\/server_protocol\.h)/)
        return "abi-sensitive";
      if (paths ~ /(^|,)(dlls\/wined3d\/|dlls\/d3d|dlls\/dxgi\/|dlls\/vulkan|dlls\/winegstreamer\/|dlls\/user32\/|configure|configure\.ac|tools\/)/)
        return "subsystem-sensitive";
      return "candidate";
    }
    function emit() {
      if (!commit) return;
      print commit "\t" date "\t" classify(paths) "\tunreviewed\t" subject "\t" paths;
    }
    BEGIN {
      print "commit\tdate\triskTier\tequivalence\tsubject\tpaths";
    }
    /^@@@/ {
      emit();
      line = substr($0, 4);
      split(line, fields, "\t");
      commit = fields[1];
      date = fields[2];
      subject = fields[3];
      for (i = 4; i <= length(fields); i++) subject = subject " " fields[i];
      paths = "";
      next;
    }
    NF {
      paths = paths (paths ? "," : "") $0;
    }
    END { emit(); }
  ' >"$output"

printf 'audit tag: %s\n' "$through" >&2
printf '%s\n' "$output"

