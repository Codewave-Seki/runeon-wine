#!/usr/bin/env python3
"""Check the real mmdevapi static_mask_to_channels against WAVEFORMATEXTENSIBLE order.

Extracts AudioObjectType_to_index and static_mask_to_channels from
dlls/mmdevapi/spatialaudio.c, takes the AudioObjectType and SPEAKER_* values from
the tree's own headers, and checks, for 5.1, 7.1, 7.1.4 and a bed with back
centre, that each static object's channel is its speaker's position among the
mask's bits in ascending order and that unused entries are ~0. It does not start
Wine or touch an audio device. Uses Python's standard library and clang;
temporary files are removed afterwards.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def constants(text, prefix):
    return {name: int(value, 16) for name, value in
            re.findall(r"\b(" + prefix + r"\w+)\s*=?\s*(0x[0-9a-fA-F]+)", text)}


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} SOURCE_ROOT")
    root = Path(sys.argv[1]).resolve(strict=True)
    path = root / "dlls/mmdevapi/spatialaudio.c"
    source = path.read_text()
    functions = []
    for name in ("AudioObjectType_to_index", "static_mask_to_channels"):
        match = re.search(r"(?ms)^static \w+ " + name + r"\(.*?^\}", source)
        if not match:
            raise ValueError(f"{name} not found in {path}")
        functions.append(match.group())

    types = constants((root / "include/spatialaudioclient.idl").read_text(), "AudioObjectType_")
    speakers = {name: int(value, 16) for name, value in re.findall(
        r"#define\s+(SPEAKER_\w+)\s+(0x[0-9a-fA-F]+)", (root / "include/mmreg.h").read_text())}
    # (object type, speaker) pairs as Windows defines the static channels.
    pairs = [("FrontLeft", "FRONT_LEFT"), ("FrontRight", "FRONT_RIGHT"), ("FrontCenter", "FRONT_CENTER"),
             ("LowFrequency", "LOW_FREQUENCY"), ("SideLeft", "SIDE_LEFT"), ("SideRight", "SIDE_RIGHT"),
             ("BackLeft", "BACK_LEFT"), ("BackRight", "BACK_RIGHT"), ("TopFrontLeft", "TOP_FRONT_LEFT"),
             ("TopFrontRight", "TOP_FRONT_RIGHT"), ("TopBackLeft", "TOP_BACK_LEFT"),
             ("TopBackRight", "TOP_BACK_RIGHT"), ("BackCenter", "BACK_CENTER")]
    beds = {
        "5.1": ["FrontLeft", "FrontRight", "FrontCenter", "LowFrequency", "SideLeft", "SideRight"],
        "7.1": ["FrontLeft", "FrontRight", "FrontCenter", "LowFrequency", "SideLeft", "SideRight",
                "BackLeft", "BackRight"],
        "7.1.4": ["FrontLeft", "FrontRight", "FrontCenter", "LowFrequency", "SideLeft", "SideRight",
                  "BackLeft", "BackRight", "TopFrontLeft", "TopFrontRight", "TopBackLeft", "TopBackRight"],
        "back-centre": ["FrontLeft", "FrontRight", "SideLeft", "SideRight", "BackCenter"],
    }

    defines = "".join(f"#define AudioObjectType_{k} 0x{types['AudioObjectType_' + k]:x}u\n" for k, _ in pairs)
    defines += "".join(f"#define SPEAKER_{s} 0x{speakers['SPEAKER_' + s]:x}u\n" for _, s in pairs)
    stubs = r'''
#include <stdint.h>
#include <stdio.h>
typedef uint32_t UINT32, DWORD, AudioObjectType;
typedef uint16_t WORD;
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define TRACE(...) do { } while (0)
''' + defines
    cases = []
    for bed, objects in beds.items():
        mask = sum(types["AudioObjectType_" + o] for o in objects)
        speaker_of = dict(pairs)
        bits = sorted(speakers["SPEAKER_" + speaker_of[o]] for o in objects)
        expected = ["~0u"] * 17
        for o in objects:
            index = types["AudioObjectType_" + o].bit_length() - 2
            expected[index] = str(bits.index(speakers["SPEAKER_" + speaker_of[o]]))
        cases.append((bed, mask, len(objects), sum(bits), expected))
    tests = "static int failures;\nint main(void)\n{\n    WORD count; DWORD mask; UINT32 map[17];\n"
    for bed, mask, count, speaker_mask, expected in cases:
        tests += f"    {{ static const UINT32 want[17] = {{{', '.join(expected)}}}; unsigned int i;\n"
        tests += f"      static_mask_to_channels(0x{mask:x}u, &count, &mask, map);\n"
        tests += f"      if (count != {count} || mask != 0x{speaker_mask:x}u) {{ ++failures; printf(\"{{\\\"bed\\\":\\\"{bed}\\\",\\\"failed\\\":\\\"count/mask\\\"}}\\n\"); }}\n"
        tests += f"      for (i = 0; i < 17; i++) if (map[i] != want[i]) {{ ++failures; printf(\"{{\\\"bed\\\":\\\"{bed}\\\",\\\"index\\\":%u,\\\"got\\\":%d,\\\"want\\\":%d}}\\n\", i, (int)map[i], (int)want[i]); }} }}\n"
    tests += f"    printf(\"{{\\\"cases\\\":{len(cases)},\\\"failures\\\":%d}}\\n\", failures);\n    return failures ? 1 : 0;\n}}\n"

    with tempfile.TemporaryDirectory(prefix="runeon-spatial-map-check-") as temporary:
        c_path = Path(temporary) / "check.c"
        executable = Path(temporary) / "check"
        c_path.write_text(stubs + "\n" + "\n".join(functions) + "\n" + tests)
        build = subprocess.run(["clang", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                                "-O0", str(c_path), "-o", str(executable)],
                               capture_output=True, text=True, timeout=60)
        if build.returncode:
            print(build.stdout + build.stderr, file=sys.stderr)
            return 2
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
        print(json.dumps({
            "sourceRoot": str(root),
            "sourceSha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "validation": "real static_mask_to_channels against ascending speaker-bit order; no Wine or audio device",
            "observations": [json.loads(line) for line in result.stdout.splitlines()],
        }, indent=2))
        return result.returncode


if __name__ == "__main__":
    sys.exit(main())
