#!/usr/bin/env python3
"""Check verify_source_markers() in scripts/common.sh against a scratch tree.

Covers presence markers and the optional "occurrences" count, including the
case it exists for: a fix that makes a line a copy of one already in the file.
It does not read a Wine tree. The scratch directory is removed on exit.
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

common = Path(__file__).resolve().parent.parent / "scripts" / "common.sh"
line = "    DWORD len = RtlUnicodeStringToOemSize( uni );"


def verify(root, markers):
    manifest = root / "manifest.json"
    manifest.write_text(json.dumps({"patches": [{"verification": markers}]}))
    return subprocess.run(
        ["bash", "-c", 'source "$0"; verify_source_markers "$1" "$2"', common, manifest, root],
        capture_output=True, text=True).returncode == 0


with tempfile.TemporaryDirectory() as scratch:
    root = Path(scratch)
    source = root / "dlls" / "ntdll" / "rtlstr.c"
    source.parent.mkdir(parents=True)
    marker = {"path": "dlls/ntdll/rtlstr.c", "contains": line.strip()}
    cases = []

    source.write_text(line + "\n")  # unpatched: the copy exists once
    cases += [("presence passes before the fix", verify(root, [marker]), True),
              ("count rejects the unpatched tree", verify(root, [dict(marker, occurrences=2)]), False)]

    source.write_text(line + "\n" + line + "\n")  # patched: a second copy
    cases += [("count accepts the patched tree", verify(root, [dict(marker, occurrences=2)]), True),
              ("count rejects too many lines", verify(root, [dict(marker, occurrences=1)]), False),
              ("missing text fails", verify(root, [dict(marker, contains="absent line")]), False),
              ("unsafe path fails", verify(root, [dict(marker, path="../rtlstr.c")]), False)]

failed = [name for name, got, want in cases if got != want]
for name, got, want in cases:
    print(("ok   " if got == want else "FAIL ") + name)
sys.exit(1 if failed else 0)
