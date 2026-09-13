#!/usr/bin/env python3
"""Check the real WIC stream_read contract with deterministic IStream stubs.

Uses Python's standard library and clang; no Wine, codec or GPU is started.
Temporary C sources and executables are removed after execution.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


STUBS = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
typedef int32_t HRESULT;
typedef uint32_t ULONG;
#define CDECL
#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define E_FAIL ((HRESULT)0x80004005)
#define E_POINTER ((HRESULT)0x80004003)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
typedef struct
{
    HRESULT result;
    ULONG available;
    int calls, null_count;
} IStream;
static int checks, failures;
static char current_case[96];
static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) { ++failures; printf("{\"case\":\"%s\",\"failed\":\"%s\"}\n", current_case, name); }
}
static HRESULT IStream_Read(IStream *stream, void *buffer, ULONG requested, ULONG *count)
{
    ++stream->calls;
    /* Model a stream that requires a count pointer, including on errors. */
    if (!count) { ++stream->null_count; return E_POINTER; }
    /* Failure deliberately leaves both outputs untouched. */
    if (!SUCCEEDED(stream->result)) return stream->result;
    *count = stream->available < requested ? stream->available : requested;
    memset(buffer, 0x42, *count);
    return stream->result;
}
'''

TESTS = r'''
static void test_read(HRESULT result, ULONG available, HRESULT expected, int caller_count)
{
    IStream stream = {result, available, 0, 0};
    unsigned char buffer[8] = {0};
    ULONG count = 0xdeadbeef;
    HRESULT hr;
    snprintf(current_case, sizeof(current_case), "caller-count-%s stream-result-%08x available-%u",
            caller_count ? "present" : "NULL", (unsigned int)result, (unsigned int)available);
    hr = stream_read(&stream, buffer, sizeof(buffer), caller_count ? &count : NULL);
    check(stream.calls == 1, "exactly one IStream read");
    check(!stream.null_count, "IStream always receives a non-NULL count pointer");
    check(hr == expected, "full/short/failure HRESULT contract");
    if (SUCCEEDED(result))
    {
        if (caller_count) check(count == available, "caller receives actual successful read count");
        check(buffer[0] == 0x42 && (available == 8 || buffer[available] == 0),
                "read data and untouched remainder are preserved");
    }
    else
    {
        check(count == 0xdeadbeef && buffer[0] == 0, "failed read preserves unwritten outputs");
    }
}

static void test_failure_does_not_read_count(void)
{
    long page_size = sysconf(_SC_PAGESIZE);
    IStream stream = {E_FAIL, 0, 0, 0};
    unsigned char buffer[8] = {0};
    void *guard;
    HRESULT hr;
    strcpy(current_case, "E_FAIL with unreadable count slot");
    if (page_size <= 0) { check(0, "obtain guard-page size"); return; }
    guard = mmap(NULL, (size_t)page_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (guard == MAP_FAILED) { check(0, "allocate unreadable count slot"); return; }
    /* Any count read on the failure path would access an unreadable page. */
    hr = stream_read(&stream, buffer, sizeof(buffer), guard);
    check(hr == E_FAIL && stream.calls == 1 && !stream.null_count,
            "E_FAIL returned without accessing an unwritten count");
    check(buffer[0] == 0, "failed read leaves data untouched with protected count");
    check(munmap(guard, (size_t)page_size) == 0, "release guard page");
}

int main(void)
{
    int caller_count;
    for (caller_count = 0; caller_count <= 1; ++caller_count)
    {
        test_read(S_OK, 8, S_OK, caller_count);
        test_read(S_OK, 3, S_FALSE, caller_count);
        test_read(S_FALSE, 3, S_FALSE, caller_count);
        test_read(E_FAIL, 0, E_FAIL, caller_count);
    }
    test_failure_does_not_read_count();
    printf("{\"cases\":9,\"checks\":%d,\"failures\":%d}\n", checks, failures);
    return failures ? 1 : 0;
}
'''


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} SOURCE_ROOT")
    root = Path(sys.argv[1]).resolve(strict=True)
    path = root / "dlls/windowscodecs/wincodecs_common.c"
    source = path.read_text()
    # Wine places the complete function's closing brace at the start of a line.
    match = re.search(r"(?ms)^HRESULT CDECL stream_read\(.*?^\}", source)
    if not match:
        raise ValueError(f"stream_read function not found in {path}")
    function = match.group()
    with tempfile.TemporaryDirectory(prefix="runeon-stream-read-check-") as temporary:
        c_path = Path(temporary) / "check.c"
        executable = Path(temporary) / "check"
        c_path.write_text(STUBS + "\n" + function + "\n" + TESTS)
        command = ["clang", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O0",
                   str(c_path), "-o", str(executable)]
        build = subprocess.run(command, capture_output=True, text=True, timeout=60)
        if build.returncode:
            print(build.stdout + build.stderr, file=sys.stderr)
            return 2
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
        print(json.dumps({
            "sourceRoot": str(root),
            "sourceSha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "functionSha256": hashlib.sha256(function.encode()).hexdigest(),
            "validation": "real stream_read with deterministic IStream stubs; no Wine or codec execution",
            "failureCountAccessCheck": "PROT_NONE output slot with E_FAIL; NULL caller also exercises the unwritten local count",
            "build": "clang -std=c11 -Wall -Wextra -Werror -O0",
            "observations": [json.loads(line) for line in result.stdout.splitlines()],
            "stderr": result.stderr,
            "exitCode": result.returncode,
        }, indent=2))
        return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
