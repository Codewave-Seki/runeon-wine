#!/usr/bin/env python3
"""Compile the patched graphics route parser against a deterministic registry stub.

Checks how init_graphics_route() in dlls/ntdll/unix/loadorder.c reads and parses
the route table: value type, length and terminator checks, per-entry validation,
directory boundaries, overlap handling, buffer growth and the size limit. When
the source carries the mesagl backend, it also checks that route, that it alone
enables the table, and that it falls back to plain Wine without a directory. When
it carries the d3dmetal-metalfx backend, it checks the extra directory, load order
and D3DM_ variables of that route, the fallback to plain d3dmetal for every missing
or invalid input, and that processes outside the route drop the variables only
while RUNEON_GFX_DIR_METALFX is set. When it carries the dxmt-metalfx backend, it
checks the load order and DXMT_ variables of that route, the fallback to plain dxmt,
that processes outside the route drop the variables only while RUNEON_DXMT_NGX_PATH
is set, and that each MetalFX route drops the other's variables. Without a backend,
it checks that its id is skipped as malformed. It does not start Wine, load any
module, check that D3DMetal or DXMT honours the variables, or touch a prefix. Temporary C source and executable are
removed when the check finishes.

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
typedef int BOOL;
#define TRUE 1
#define FALSE 0
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
static const char *prepended, *prepended_second;
static const WCHAR *override_set;
static int queries, opens;
static void prepend_dll_path(const char *p) { if (prepended) prepended_second = p; else prepended = p; }
static void add_load_order_override(const WCHAR *entry) { override_set = entry; }
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
    prepended = prepended_second = NULL;
    override_set = NULL;
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
    /* an older build skips the id as malformed; without its variables a newer one
     * behaves as the d3dmetal route */
    set_table("C:\\Game=d3dmetal-metalfx");
    check("metalfx id without its variables -> d3dmetal", exe, "apple");
    if (prepended_second || override_set) { printf("FAIL metalfx id without its variables added more\n"); failures++; }
#ifdef HAS_DXMT_METALFX
    set_table("C:\\Game=dxmt-metalfx");
    check("dxmt-metalfx id without its variable -> dxmt", exe, "dxmt");
#else
    set_table("C:\\Game=dxmt-metalfx");
    check("dxmt-metalfx id is skipped -> d3dmetal", exe, "apple");
#endif
    if (prepended_second || override_set) { printf("FAIL dxmt-metalfx id without its variable added more\n"); failures++; }

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
#ifdef HAS_MESAGL
    setenv("RUNEON_GFX_DIR_D3DMETAL", "apple", 1);
    setenv("RUNEON_GFX_DIR_DXMT", "dxmt", 1);
    setenv("RUNEON_GFX_DIR_MESAGL", "mesa", 1);
    set_table("C:\\Game=mesagl");
    check("mesagl route", exe, "mesa");
    check("mesagl boundary Game vs Game2", "C:\\Game2\\game.exe", "apple");
    set_table("C:\\Game=MESAGL|C:\\Game=mesag|C:\\Game=mesagl ");
    check("malformed mesagl ids are skipped", exe, "apple");
    set_table("C:\\Game=mesagl|C:\\Game\\Sub=dxmt");
    check("mesagl overlap -> default", exe, "apple");
    set_table("C:\\Game=mesagl");
    unsetenv("RUNEON_GFX_DIR_MESAGL");
    check("mesagl route without mesagl dir loads plain Wine", exe, "none");
    unsetenv("RUNEON_GFX_DIR_D3DMETAL");
    unsetenv("RUNEON_GFX_DIR_DXMT");
    setenv("RUNEON_GFX_DIR_MESAGL", "mesa", 1);
    opens = 0;
    check("mesagl dir alone enables the table", exe, "mesa");
    if (!opens) { printf("FAIL mesagl dir alone did not read the registry\n"); failures++; }
#endif
#ifdef HAS_METALFX
    {
        static const char * const vars[] = { "D3DM_ENABLE_METALFX", "D3DM_SUPPORT_DXR", "D3DM_NVNGX_PATH" };
        char ngx129[130], ngx130[131];
        unsigned int i;
#define INHERIT() do { setenv(vars[0], "1", 1); setenv(vars[1], "0", 1); setenv(vars[2], "C:\\x\\y", 1); } while (0)
#define VARS_ARE(name, a, b, c) do { const char *want[3] = { a, b, c }; \
        for (i = 0; i < 3; i++) { const char *got = getenv(vars[i]); \
            if (want[i] ? !got || strcmp(got, want[i]) : got != NULL) \
            { printf("FAIL %s: %s is %s\n", name, vars[i], got ? got : "unset"); failures++; } } } while (0)
#define PLAIN(name) do { if (prepended_second || override_set) \
        { printf("FAIL %s: metalfx additions applied\n", name); failures++; } } while (0)

        setenv("RUNEON_GFX_DIR_D3DMETAL", "apple", 1);
        setenv("RUNEON_GFX_DIR_DXMT", "dxmt", 1);
        unsetenv("RUNEON_GFX_DIR_MESAGL");
        setenv("RUNEON_GFX_DIR_METALFX", "/fx", 1);
        setenv("RUNEON_METALFX_NGX_PATH", "C:\\runeon\\ngx", 1);
        set_table("C:\\Game=d3dmetal-metalfx");
        unsetenv(vars[0]); unsetenv(vars[1]); unsetenv(vars[2]);
        check("metalfx route loads d3dmetal", exe, "apple");
        if (!prepended_second || strcmp(prepended_second, "/fx"))
        { printf("FAIL metalfx alias directory not prepended\n"); failures++; }
        {
            char o[64] = ""; unsigned int n = 0;
            if (override_set) for (; override_set[n] && n < sizeof(o) - 1; n++) o[n] = (char)override_set[n];
            o[n] = 0;
            if (strcmp(o, "nvngx,nvapi64=n,b")) { printf("FAIL metalfx load order is '%s'\n", o); failures++; }
        }
        VARS_ARE("metalfx route sets the variables", "1", "0", "C:\\runeon\\ngx");
        check("metalfx route outside the directory", "C:\\Game2\\game.exe", "apple");
        PLAIN("metalfx route outside the directory");
        VARS_ARE("process outside the route drops inherited variables", NULL, NULL, NULL);

        memset(ngx129, 'a', sizeof(ngx129)); memcpy(ngx129, "C:\\", 3); ngx129[129] = 0;
        setenv("RUNEON_METALFX_NGX_PATH", ngx129, 1);
        check("129-character NGX path is used", exe, "apple");
        VARS_ARE("129-character NGX path is used", "1", "0", ngx129);

        {
            static const char * const bad[] = { "C:\\runeon\\ngx\\", "C:/runeon/ngx", "C:\\runeon/ngx",
                "runeon\\ngx", "\\\\?\\C:\\ngx", "C:\\", "1:\\ngx", "C:\\r\xc3\xa9", "C:\\a\tb", "" };
            memset(ngx130, 'a', sizeof(ngx130)); memcpy(ngx130, "C:\\", 3); ngx130[130] = 0;
            for (unsigned int b = 0; b <= ARRAY_SIZE(bad); b++)
            {
                setenv("RUNEON_METALFX_NGX_PATH", b < ARRAY_SIZE(bad) ? bad[b] : ngx130, 1);
                INHERIT();
                check("invalid NGX path -> d3dmetal", exe, "apple");
                PLAIN("invalid NGX path");
                VARS_ARE("invalid NGX path drops inherited variables", NULL, NULL, NULL);
            }
        }
        unsetenv("RUNEON_METALFX_NGX_PATH");
        INHERIT();
        check("missing NGX path -> d3dmetal", exe, "apple");
        PLAIN("missing NGX path");
        VARS_ARE("missing NGX path drops inherited variables", NULL, NULL, NULL);
        setenv("RUNEON_METALFX_NGX_PATH", "C:\\runeon\\ngx", 1);

        setenv("RUNEON_GFX_DIR_METALFX", "fx", 1);
        INHERIT();
        check("relative metalfx directory -> d3dmetal", exe, "apple");
        PLAIN("relative metalfx directory");
        VARS_ARE("relative metalfx directory drops inherited variables", NULL, NULL, NULL);
        setenv("RUNEON_GFX_DIR_METALFX", "", 1);
        check("empty metalfx directory -> d3dmetal", exe, "apple");
        PLAIN("empty metalfx directory");

        /* manual testing: without the launcher's directory nothing is touched */
        unsetenv("RUNEON_GFX_DIR_METALFX");
        INHERIT();
        check("no metalfx directory -> d3dmetal", exe, "apple");
        PLAIN("no metalfx directory");
        VARS_ARE("no metalfx directory keeps the caller's variables", "1", "0", "C:\\x\\y");
        set_table("C:\\Game=dxmt");
        check("no metalfx directory, other route", exe, "dxmt");
        VARS_ARE("no metalfx directory, other route keeps the variables", "1", "0", "C:\\x\\y");
        setenv("RUNEON_GFX_DIR_METALFX", "/fx", 1);
        check("dxmt route in a managed launch", exe, "dxmt");
        VARS_ARE("dxmt route drops inherited variables", NULL, NULL, NULL);

        set_table("C:\\Game=d3dmetal-metalfx");
        unsetenv("RUNEON_GFX_DIR_D3DMETAL");
        INHERIT();
        check("metalfx route without d3dmetal dir loads plain Wine", exe, "none");
        PLAIN("metalfx route without d3dmetal dir");
        VARS_ARE("metalfx route without d3dmetal dir drops inherited variables", NULL, NULL, NULL);
        setenv("RUNEON_GFX_DIR_D3DMETAL", "apple", 1);

        set_table("C:\\Game=d3dmetal-metalfx|C:\\Game\\Sub=dxmt");
        INHERIT();
        check("metalfx overlap -> default", exe, "apple");
        PLAIN("metalfx overlap");
        VARS_ARE("metalfx overlap drops inherited variables", NULL, NULL, NULL);
        set_table("C:\\Game=D3DMETAL-METALFX|C:\\Game=d3dmetal-metalf|C:\\Game=d3dmetal-metalfx |C:\\Game=d3dmetalmetalfx");
        check("malformed metalfx ids are skipped", exe, "apple");
        PLAIN("malformed metalfx ids");

        unsetenv("RUNEON_GFX_DIR_D3DMETAL");
        unsetenv("RUNEON_GFX_DIR_DXMT");
        set_table("C:\\Other=dxmt");
        INHERIT();
        opens = 0;
        check("metalfx dir alone enables the table", exe, "none");
        if (!opens) { printf("FAIL metalfx dir alone did not read the registry\n"); failures++; }
        VARS_ARE("metalfx dir alone drops inherited variables", NULL, NULL, NULL);
    }
#endif
#ifdef HAS_DXMT_METALFX
    {
        static const char * const dvars[] = { "DXMT_ENABLE_NVEXT", "DXMT_NVEXT_NGX_PATH" };
        static const char * const fxvars[] = { "D3DM_ENABLE_METALFX", "D3DM_SUPPORT_DXR", "D3DM_NVNGX_PATH" };
        char ngx129[130];
        unsigned int i;
#define DINHERIT() do { setenv(dvars[0], "1", 1); setenv(dvars[1], "C:\\x\\y", 1); } while (0)
#define DVARS_ARE(name, a, b) do { const char *want[2] = { a, b }; \
        for (i = 0; i < 2; i++) { const char *got = getenv(dvars[i]); \
            if (want[i] ? !got || strcmp(got, want[i]) : got != NULL) \
            { printf("FAIL %s: %s is %s\n", name, dvars[i], got ? got : "unset"); failures++; } } } while (0)
#define DPLAIN(name) do { if (prepended_second || override_set) \
        { printf("FAIL %s: dxmt-metalfx additions applied\n", name); failures++; } } while (0)

        setenv("RUNEON_GFX_DIR_D3DMETAL", "apple", 1);
        setenv("RUNEON_GFX_DIR_DXMT", "dxmt", 1);
        unsetenv("RUNEON_GFX_DIR_MESAGL");
        unsetenv("RUNEON_GFX_DIR_METALFX");
        unsetenv("RUNEON_METALFX_NGX_PATH");
        setenv("RUNEON_DXMT_NGX_PATH", "C:\\runeon\\ngx-dxmt", 1);
        set_table("C:\\Game=dxmt-metalfx");
        unsetenv(dvars[0]); unsetenv(dvars[1]);
        check("dxmt-metalfx route loads dxmt", exe, "dxmt");
        if (prepended_second) { printf("FAIL dxmt-metalfx route prepended a second directory\n"); failures++; }
        {
            char o[64] = ""; unsigned int n = 0;
            if (override_set) for (; override_set[n] && n < sizeof(o) - 1; n++) o[n] = (char)override_set[n];
            o[n] = 0;
            if (strcmp(o, "nvngx,nvapi64=n,b")) { printf("FAIL dxmt-metalfx load order is '%s'\n", o); failures++; }
        }
        DVARS_ARE("dxmt-metalfx route sets the variables", "1", "C:\\runeon\\ngx-dxmt");
        check("dxmt-metalfx route outside the directory", "C:\\Game2\\game.exe", "apple");
        DPLAIN("dxmt-metalfx route outside the directory");
        DVARS_ARE("process outside the dxmt-metalfx route drops inherited variables", NULL, NULL);

        memset(ngx129, 'a', sizeof(ngx129)); memcpy(ngx129, "C:\\", 3); ngx129[129] = 0;
        setenv("RUNEON_DXMT_NGX_PATH", ngx129, 1);
        check("129-character DXMT NGX path is used", exe, "dxmt");
        DVARS_ARE("129-character DXMT NGX path is used", "1", ngx129);

        {
            static const char * const bad[] = { "C:\\runeon\\ngx-dxmt\\", "C:/runeon/ngx-dxmt",
                "runeon\\ngx", "C:\\", "C:\\r\xc3\xa9", "C:\\a\tb", "" };
            char ngx130[131];
            memset(ngx130, 'a', sizeof(ngx130)); memcpy(ngx130, "C:\\", 3); ngx130[130] = 0;
            for (unsigned int b = 0; b <= ARRAY_SIZE(bad); b++)
            {
                setenv("RUNEON_DXMT_NGX_PATH", b < ARRAY_SIZE(bad) ? bad[b] : ngx130, 1);
                DINHERIT();
                check("invalid DXMT NGX path -> dxmt", exe, "dxmt");
                DPLAIN("invalid DXMT NGX path");
                DVARS_ARE("invalid DXMT NGX path drops inherited variables", NULL, NULL);
            }
        }

        /* manual testing: without the launcher's NGX path nothing is touched */
        unsetenv("RUNEON_DXMT_NGX_PATH");
        DINHERIT();
        check("missing DXMT NGX path -> dxmt", exe, "dxmt");
        DPLAIN("missing DXMT NGX path");
        DVARS_ARE("missing DXMT NGX path keeps the caller's variables", "1", "C:\\x\\y");
        set_table("C:\\Game=dxmt");
        check("no DXMT NGX path, dxmt route", exe, "dxmt");
        DVARS_ARE("no DXMT NGX path, dxmt route keeps the variables", "1", "C:\\x\\y");
        setenv("RUNEON_DXMT_NGX_PATH", "C:\\runeon\\ngx-dxmt", 1);
        check("dxmt route in a managed launch", exe, "dxmt");
        DVARS_ARE("dxmt route drops inherited DXMT variables", NULL, NULL);

        set_table("C:\\Game=dxmt-metalfx");
        unsetenv("RUNEON_GFX_DIR_DXMT");
        DINHERIT();
        check("dxmt-metalfx route without dxmt dir loads plain Wine", exe, "none");
        DPLAIN("dxmt-metalfx route without dxmt dir");
        DVARS_ARE("dxmt-metalfx route without dxmt dir drops inherited variables", NULL, NULL);
        setenv("RUNEON_GFX_DIR_DXMT", "", 1);
        DINHERIT();
        check("dxmt-metalfx route with empty dxmt dir loads plain Wine", exe, "none");
        DPLAIN("dxmt-metalfx route with empty dxmt dir");
        setenv("RUNEON_GFX_DIR_DXMT", "dxmt", 1);

        set_table("C:\\Game=dxmt-metalfx|C:\\Game\\Sub=dxmt");
        DINHERIT();
        check("dxmt-metalfx overlap -> default", exe, "apple");
        DPLAIN("dxmt-metalfx overlap");
        DVARS_ARE("dxmt-metalfx overlap drops inherited variables", NULL, NULL);
        set_table("C:\\Game=DXMT-METALFX|C:\\Game=dxmt-metalf|C:\\Game=dxmt-metalfx |C:\\Game=dxmtmetalfx");
        check("malformed dxmt-metalfx ids are skipped", exe, "apple");
        DPLAIN("malformed dxmt-metalfx ids");

        /* the two MetalFX routes in one session: each drops the other's variables */
        setenv("RUNEON_GFX_DIR_METALFX", "/fx", 1);
        setenv("RUNEON_METALFX_NGX_PATH", "C:\\runeon\\ngx", 1);
        set_table("C:\\Game=dxmt-metalfx");
        setenv(fxvars[0], "1", 1); setenv(fxvars[1], "0", 1); setenv(fxvars[2], "C:\\runeon\\ngx", 1);
        unsetenv(dvars[0]); unsetenv(dvars[1]);
        check("dxmt-metalfx route in a session with both", exe, "dxmt");
        DVARS_ARE("dxmt-metalfx route in a session with both", "1", "C:\\runeon\\ngx-dxmt");
        for (i = 0; i < 3; i++) if (getenv(fxvars[i]))
        { printf("FAIL dxmt-metalfx route kept %s\n", fxvars[i]); failures++; }
        set_table("C:\\Game=d3dmetal-metalfx");
        DINHERIT();
        check("d3dmetal-metalfx route in a session with both", exe, "apple");
        DVARS_ARE("d3dmetal-metalfx route drops DXMT variables", NULL, NULL);
        if (!getenv(fxvars[0])) { printf("FAIL d3dmetal-metalfx route lost its variables\n"); failures++; }
        unsetenv("RUNEON_GFX_DIR_METALFX");
        unsetenv("RUNEON_METALFX_NGX_PATH");

        unsetenv("RUNEON_GFX_DIR_D3DMETAL");
        unsetenv("RUNEON_GFX_DIR_DXMT");
        set_table("C:\\Other=dxmt");
        DINHERIT();
        opens = 0;
        check("DXMT NGX path alone enables the table", exe, "none");
        if (!opens) { printf("FAIL DXMT NGX path alone did not read the registry\n"); failures++; }
        DVARS_ARE("DXMT NGX path alone drops inherited variables", NULL, NULL);
    }
#endif
    return failures != 0;
}
'''


def extract(source):
    # A launcher detects the table format by this string in the built module;
    # the TRACE format string is what keeps it there.
    if ('#define GRAPHICS_ROUTES_PROTOCOL "runeon-graphics-routes-protocol-1"' not in source
            or 'TRACE( GRAPHICS_ROUTES_PROTOCOL ": ' not in source):
        raise SystemExit('graphics route protocol marker missing')
    if 'GRAPHICS_D3DMETAL_METALFX' in source and (
            '#define GRAPHICS_ROUTES_METALFX "runeon-graphics-routes-metalfx-1"' not in source
            or 'TRACE( GRAPHICS_ROUTES_METALFX ": ' not in source):
        raise SystemExit('graphics route metalfx marker missing')
    if 'GRAPHICS_DXMT_METALFX' in source and (
            '#define GRAPHICS_ROUTES_DXMT_METALFX "runeon-graphics-routes-dxmt-metalfx-1"' not in source
            or 'TRACE( GRAPHICS_ROUTES_DXMT_METALFX ": ' not in source):
        raise SystemExit('graphics route dxmt-metalfx marker missing')
    start = source.index('/* Runeon graphics backend ids;')
    end = source.index('/***************************************************************************\n *\tget_load_order ', start)
    return source[start:end]


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f'usage: {sys.argv[0]} SOURCE_ROOT')
    path = Path(sys.argv[1]).resolve(strict=True) / 'dlls/ntdll/unix/loadorder.c'
    source = extract(path.read_text())
    code = (('#define HAS_MESAGL 1\n' if 'GRAPHICS_MESAGL' in source else '')
            + ('#define HAS_METALFX 1\n' if 'GRAPHICS_D3DMETAL_METALFX' in source else '')
            + ('#define HAS_DXMT_METALFX 1\n' if 'GRAPHICS_DXMT_METALFX' in source else '')
            + HARNESS + source + MAIN)
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
