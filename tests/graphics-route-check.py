#!/usr/bin/env python3
"""Compile the patched graphics route parser against a deterministic registry stub.

Checks how init_graphics_route() in dlls/ntdll/unix/loadorder.c reads and parses
the route table: value type, length and terminator checks, per-entry validation,
directory boundaries, overlap handling, buffer growth and the size limit. It does
not start Wine, load any module or touch a prefix. Temporary C source and
executable are removed when the check finishes.

usage: graphics-route-check.py SOURCE_ROOT
"""
from pathlib import Path
import subprocess
import sys
import tempfile

HARNESS = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
typedef uint16_t WCHAR;
typedef uint32_t DWORD;
typedef uint32_t NTSTATUS;
typedef void *HANDLE;
typedef struct { uint16_t Length, MaximumLength; WCHAR *Buffer; } UNICODE_STRING;
typedef struct { DWORD TitleIndex, Type, DataLength; unsigned char Data[1]; } KEY_VALUE_PARTIAL_INFORMATION;
#define STATUS_SUCCESS 0
#define STATUS_NO_MEMORY 0xC0000017
#define STATUS_BUFFER_OVERFLOW 0x80000005
#define STATUS_OBJECT_NAME_NOT_FOUND 0xC0000034
#define REG_SZ 1
#define REG_MULTI_SZ 7
#define KeyValuePartialInformation 2
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define WARN(...) ((void)0)
#define TRACE(...) ((void)0)
static const char *debugstr_wn(const WCHAR *s, unsigned int n) { return ""; }
static const char *debugstr_us(const UNICODE_STRING *s) { return ""; }
static const char *debugstr_a(const char *s) { return ""; }
static int wcsnicmp(const WCHAR *a, const WCHAR *b, unsigned int n)
{
    while (n--) { int x = tolower(*a++), y = tolower(*b++); if (x != y) return x - y; if (!x) return 0; }
    return 0;
}
static void init_unicode_string(UNICODE_STRING *s, const WCHAR *d)
{
    unsigned int n = 0; while (d[n]) n++;
    s->Length = n * 2; s->MaximumLength = s->Length + 2; s->Buffer = (WCHAR *)d;
}
static const char *prepended;
static int queries, opens;
static void prepend_dll_path(const char *p) { prepended = p; }
static int has_value, value_type;
static unsigned char value[2 * 1024 * 1024 + 64];
static DWORD value_len;
static NTSTATUS open_hkcu_key(const char *p, HANDLE *key)
{
    opens++;
    if (strcmp(p, "Software\\Runeon\\GraphicsRoutes")) return STATUS_OBJECT_NAME_NOT_FOUND;
    *key = (HANDLE)1;
    return 0;
}
static NTSTATUS NtClose(HANDLE key) { return 0; }
static NTSTATUS NtQueryValueKey(HANDLE key, const UNICODE_STRING *name, int cls, void *buf, DWORD len, DWORD *res)
{
    KEY_VALUE_PARTIAL_INFORMATION *info = buf;
    DWORD need = offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data) + value_len;
    queries++;
    if (!has_value) return STATUS_OBJECT_NAME_NOT_FOUND;
    *res = need;
    if (len < need) return STATUS_BUFFER_OVERFLOW;
    info->TitleIndex = 0; info->Type = value_type; info->DataLength = value_len;
    memcpy(info->Data, value, value_len);
    return STATUS_SUCCESS;
}
/* Build a REG_MULTI_SZ from '|'-separated entries: "a|b" -> "a\0b\0\0". */
static void set_table(const char *entries)
{
    WCHAR *w = (WCHAR *)value; unsigned int n = 0;
    for (const char *p = entries; *p; p++) w[n++] = *p == '|' ? 0 : (unsigned char)*p;
    w[n++] = 0; w[n++] = 0;
    has_value = 1; value_type = REG_MULTI_SZ; value_len = n * 2;
}
'''

MAIN = r'''
static int failures;
static void check(const char *name, const char *exe, const char *want)
{
    WCHAR path[256]; unsigned int n = 0;
    UNICODE_STRING image;
    for (const char *p = exe; *p; p++) path[n++] = (unsigned char)*p;
    path[n] = 0;
    image.Length = n * 2; image.MaximumLength = image.Length + 2; image.Buffer = path;
    prepended = NULL;
    init_graphics_route(&image);
    const char *got = prepended ? prepended : "none";
    if (strcmp(got, want)) { printf("FAIL %s: want %s got %s\n", name, want, got); failures++; }
    else printf("pass %s\n", name);
}
int main(void)
{
    const char *exe = "C:\\Game\\Sub\\game.exe";
    setenv("RUNEON_GFX_DIR_D3DMETAL", "apple", 1);
    setenv("RUNEON_GFX_DIR_DXMT", "dxmt", 1);

    has_value = 0;
    check("no table -> default", exe, "apple");
    set_table("C:\\Game=dxmt");
    check("routed directory", exe, "dxmt");
    check("case-insensitive directory", "c:\\GAME\\sub\\game.exe", "dxmt");
    check("boundary Game vs Game2", "C:\\Game2\\game.exe", "apple");
    check("directory itself is not inside", "C:\\Game", "apple");
    set_table("C:\\Game=wine");
    check("wine route prepends nothing", exe, "none");
    set_table("C:\\Other=bogus|C:\\Game=dxmt");
    check("invalid entry does not hide later route", exe, "dxmt");
    set_table("C:\\Game=dxmt|C:/x=dxmt|C:\\Game\\Sub=wine");
    check("invalid entry does not hide overlap", exe, "apple");
    set_table("C:\\Game\\=dxmt|Game=dxmt|C:\\=dxmt|C:\\Game=dxmt=dxmt|C:\\Game=DXMT|C:\\Game=dxmt ");
    check("malformed entries are all skipped", exe, "apple");
    set_table("C:\\Game=dxmt");
    value_len -= 1;
    check("odd byte length rejects the table", exe, "apple");
    set_table("C:\\Game=dxmt");
    value_len -= 4;
    check("missing terminator rejects the table", exe, "apple");
    set_table("C:\\Game=dxmt");
    value_type = REG_SZ;
    check("wrong type rejects the table", exe, "apple");
    /* the launcher's empty table: one terminator pair */
    set_table("");
    value_len = 2;
    check("empty table -> default", exe, "apple");
    set_table("C:\\Game=dxmt");
    value_len = 0;
    check("zero-length value rejects the table", exe, "apple");
    set_table("C:\\Game=dxmt");
    check("NT path prefix is not a drive path", "\\\\?\\C:\\Game\\game.exe", "apple");
    check("UNC path is not a drive path", "\\\\server\\share\\Game\\game.exe", "apple");
    setenv("RUNEON_GFX_DIR_DXMT", "", 1);
    check("empty backend directory prepends nothing", exe, "none");
    setenv("RUNEON_GFX_DIR_DXMT", "dxmt", 1);

    /* ~9 KB table: needs the buffer to grow past its first size */
    {
        static char big[20000]; char *p = big;
        for (int i = 0; i < 400; i++) p += sprintf(p, "C:\\Other%03d=dxmt|", i);
        strcpy(p, "C:\\Game=dxmt");
        set_table(big);
        queries = 0;
        check("large table grows the buffer", exe, "dxmt");
        if (queries < 2) { printf("FAIL large table was read in one query\n"); failures++; }
    }
    /* exactly at the 1 MiB limit (12-byte header + data) is read; 2 bytes more is not.
     * A malformed filler entry precedes the route, so only the size decides. */
    {
        WCHAR *w = (WCHAR *)value;
        const char *route = "C:\\Game=dxmt";
        unsigned int filler = (1024 * 1024 - 12) / 2 - 15, n;
        for (int extra = 0; extra < 2; extra++)
        {
            n = 0;
            for (unsigned int i = 0; i < filler + extra; i++) w[n++] = 'x';
            w[n++] = 0;
            for (const char *p = route; *p; p++) w[n++] = (unsigned char)*p;
            w[n++] = 0; w[n++] = 0;
            has_value = 1; value_type = REG_MULTI_SZ; value_len = n * 2;
            check(extra ? "2 bytes over the limit -> default" : "exactly at the limit is read",
                  exe, extra ? "apple" : "dxmt");
        }
    }
    /* over the 1 MiB limit: ignored as a whole */
    {
        set_table("C:\\Game=dxmt");
        WCHAR *w = (WCHAR *)value;
        for (unsigned int i = 0; i < 600000; i++) w[i] = 'x';
        w[600000] = 0; w[600001] = 0;
        value_len = 600002 * 2;
        check("oversized table -> default", exe, "apple");
    }

    set_table("C:\\Game=dxmt");
    unsetenv("RUNEON_GFX_DIR_DXMT");
    check("dxmt route without dxmt dir loads plain Wine", exe, "none");
    unsetenv("RUNEON_GFX_DIR_D3DMETAL");
    opens = 0;
    check("capability off: nothing prepended", exe, "none");
    if (opens) { printf("FAIL capability off still read the registry\n"); failures++; }
    return failures != 0;
}
'''


def extract(source):
    # A launcher detects the table format by this string in the built module;
    # the TRACE format string is what keeps it there.
    if ('#define GRAPHICS_ROUTES_PROTOCOL "runeon-graphics-routes-protocol-1"' not in source
            or 'TRACE( GRAPHICS_ROUTES_PROTOCOL ": ' not in source):
        raise SystemExit('graphics route protocol marker missing')
    start = source.index('/* Runeon graphics backend ids;')
    end = source.index('/***************************************************************************\n *\tget_load_order ', start)
    return source[start:end]


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f'usage: {sys.argv[0]} SOURCE_ROOT')
    path = Path(sys.argv[1]).resolve(strict=True) / 'dlls/ntdll/unix/loadorder.c'
    code = HARNESS + extract(path.read_text()) + MAIN
    with tempfile.TemporaryDirectory(prefix='graphics-route-check-') as tmp:
        c = Path(tmp) / 'check.c'
        binary = Path(tmp) / 'check'
        c.write_text(code)
        subprocess.run(['cc', '-std=gnu11', '-Wall', '-Werror', '-Wno-unused-function',
                        '-D_DEFAULT_SOURCE', '-D_DARWIN_C_SOURCE',
                        str(c), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], text=True, capture_output=True)
    print(result.stdout, end='')
    if result.returncode:
        raise SystemExit('graphics route check failed')
    print('graphics route parser checks passed')


if __name__ == '__main__':
    main()
