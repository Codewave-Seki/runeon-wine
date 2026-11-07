#!/usr/bin/env python3
"""Compile the patched collapse_path() and get_full_path() and check them.

keep_dots keeps the dots and spaces ending a name, still collapses . and ..
elements, and leaves other paths as the Windows rule does. Without keep_dots
the Windows rule is unchanged. get_full_path() runs with the drive lookup
stubbed to find no drive: only an absolute Unix name keeps its dots. It does
not run Wine, the real drive lookup or Win32 path APIs.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

PRELUDE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL; typedef char16_t_ WCHAR; typedef unsigned int ULONG; typedef unsigned int NTSTATUS;
typedef struct { WCHAR *Buffer; } UNICODE_STRING;
#define TRUE 1
#define FALSE 0
#define STATUS_SUCCESS 0
#define STATUS_NO_MEMORY 0xc0000017
#define FILE_OPEN 1
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define IS_SEPARATOR(ch) ((ch) == '\\' || (ch) == '/')
#define towupper(c) ((c) >= 'a' && (c) <= 'z' ? (c) - 32 : (c))
static const WCHAR unix_prefixW[] = {'\\','?','?','\\','u','n','i','x'};
static const WCHAR nt_prefixW[] = {'\\','?','?','\\'};
static const WCHAR dos_prefixW[] = {'\\','?','?','\\','A',':','\\'};
static const WCHAR unc_prefixW[] = {'\\','?','?','\\','U','N','C','\\'};
static size_t wcslen( const WCHAR *s ) { size_t n = 0; while (s[n]) n++; return n; }
static NTSTATUS find_drive_nt_root( char *name, unsigned int len, WCHAR **nt, UINT_ disposition )
{ (void)name; (void)len; (void)disposition; *nt = NULL; return STATUS_SUCCESS; }
static int ntdll_umbstowcs( const char *src, size_t srclen, WCHAR *dst, size_t dstlen )
{ size_t i; for (i = 0; i < srclen && i < dstlen; i++) dst[i] = (unsigned char)src[i]; return (int)i; }
static void init_unicode_string( UNICODE_STRING *str, WCHAR *data ) { str->Buffer = data; }
'''.replace("char16_t_", "uint16_t").replace("UINT_", "unsigned int")

CHECKS = r'''
static void check_full( const char *in, const char *out )
{
    static const WCHAR curdir[] = {'\\','?','?','\\','C',':','\\','d','.','\\',0};
    WCHAR want[128];
    char name[128];
    UNICODE_STRING nt;
    size_t i;
    strcpy( name, in );
    for (i = 0; (want[i] = (unsigned char)out[i]); i++) ;
    assert( !get_full_path( name, curdir, &nt ) );
    assert( !memcmp( nt.Buffer, want, (wcslen( want ) + 1) * sizeof(WCHAR) ) );
    free( nt.Buffer );
}

static void check( const char *in, BOOL keep_dots, const char *out )
{
    WCHAR buf[128], want[128];
    size_t i;
    for (i = 0; (buf[i] = (unsigned char)in[i]); i++) ;
    for (i = 0; (want[i] = (unsigned char)out[i]); i++) ;
    collapse_path( buf, keep_dots );
    assert( !memcmp( buf, want, (wcslen( want ) + 1) * sizeof(WCHAR) ) );
}

int main(void)
{
    static const char *const plain[][2] = {
        { "\\??\\Z:\\Users\\name\\lib", "\\??\\Z:\\Users\\name\\lib" },
        { "\\??\\Z:/a/./b/../c", "\\??\\Z:\\a\\c" },
        { "\\??\\Z:\\a.b\\.hidden\\c", "\\??\\Z:\\a.b\\.hidden\\c" },
        { "\\??\\Z:\\", "\\??\\Z:\\" },
    };
    size_t i;

    for (i = 0; i < sizeof(plain) / sizeof(plain[0]); i++)
    {
        check( plain[i][0], FALSE, plain[i][1] );
        check( plain[i][0], TRUE, plain[i][1] );
    }
    check( "\\??\\Z:\\Users\\name.\\lib", TRUE, "\\??\\Z:\\Users\\name.\\lib" );
    check( "\\??\\Z:\\Users\\name.", TRUE, "\\??\\Z:\\Users\\name." );
    check( "\\??\\Z:\\Users\\name ", TRUE, "\\??\\Z:\\Users\\name " );
    check( "\\??\\Z:\\a.\\.\\b\\..\\c", TRUE, "\\??\\Z:\\a.\\c" );
    check( "\\??\\unix\\Users\\name.\\lib", TRUE, "\\??\\unix\\Users\\name.\\lib" );
    check( "\\??\\Z:\\Users\\name.\\lib", FALSE, "\\??\\Z:\\Users\\name\\lib" );
    check( "\\??\\Z:\\Users\\name.", FALSE, "\\??\\Z:\\Users\\name" );
    check( "\\??\\Z:\\Users\\name ", FALSE, "\\??\\Z:\\Users\\name" );
    check_full( "/Users/name./app.exe", "\\??\\unix\\Users\\name.\\app.exe" );
    check_full( "C:\\a.\\app.exe", "\\??\\C:\\a\\app.exe" );
    check_full( "x.\\app.exe", "\\??\\C:\\d\\x\\app.exe" );
    check_full( "\\\\?\\C:\\a.\\app.exe", "\\??\\C:\\a\\app.exe" );
    return 0;
}
'''


def main():
    source = Path(sys.argv[1], "dlls/ntdll/unix/file.c").read_text(errors="replace")
    match = re.search(r"^static WCHAR \*collapse_path\( WCHAR \*path, BOOL keep_dots \)\n\{.*?^\}\n",
                      source, re.M | re.S)
    full = re.search(r"^NTSTATUS get_full_path\( .*?\n\{.*?^\}\n", source, re.M | re.S)
    if not match or not full:
        sys.exit("collapse_path( path, keep_dots ) or get_full_path() not found")
    with tempfile.TemporaryDirectory() as temp:
        c_file = Path(temp, "check.c")
        binary = Path(temp, "check")
        c_file.write_text(PRELUDE + match.group(0) + full.group(0) + CHECKS)
        subprocess.run(["cc", "-std=c11", "-Wall", "-Werror", "-o", str(binary), str(c_file)], check=True)
        subprocess.run([str(binary)], check=True)
    print("unix path trailing dot check passed")


if __name__ == "__main__":
    main()
