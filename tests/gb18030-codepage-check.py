#!/usr/bin/env python3
"""Compile the patched kernelbase code page 54936 (GB18030) conversion.

The GB18030 helpers are extracted from dlls/kernelbase/locale.c and built
with the system compiler against the tree's own nls/c_936.nls, loaded the
way RtlInitCodePageTable() reads it. Expected values come from Python's
gb18030 codec, which implements GB18030-2000 as Windows does.

Covered: every BMP character except surrogates encodes to the expected bytes
and decodes back; supplementary characters through surrogate pairs; every
two-byte code decodes as expected; invalid, truncated and unassigned input
decodes to U+FFFD or fails under MB_ERR_INVALID_CHARS; a lone surrogate
encodes to the default character or fails under WC_ERR_INVALID_CHARS; and a
short buffer fails with ERROR_INSUFFICIENT_BUFFER in both directions; codes
where GB18030-2000 and -2005 differ are pinned to 2000. Not covered: the
MultiByteToWideChar()/WideCharToMultiByte() wrappers, GetCPInfo(), the PE
build for i386/x86_64, and running inside Wine; a complete build and an
application run are still required before tagging.
"""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

PRELUDE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned short USHORT, WCHAR;
typedef unsigned int UINT, DWORD;
typedef unsigned char BYTE;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MB_ERR_INVALID_CHARS 0x08
#define WC_ERR_INVALID_CHARS 0x80
#define ERROR_INSUFFICIENT_BUFFER 122
#define ERROR_NO_UNICODE_TRANSLATION 1113
/* only what include/winnls.h defines, so a missing macro fails to compile */
#define IS_HIGH_SURROGATE(ch) ((ch) >= 0xd800 && (ch) <= 0xdbff)
#define IS_LOW_SURROGATE(ch) ((ch) >= 0xdc00 && (ch) <= 0xdfff)
typedef struct
{
    USHORT CodePage, MaximumCharacterSize, DefaultChar, UniDefaultChar, TransDefaultChar, TransUniDefaultChar;
    USHORT DBCSCodePage;
    BYTE LeadByte[12];
    USHORT *MultiByteTable;
    void *WideCharTable;
    USHORT *DBCSRanges;
    USHORT *DBCSOffsets;
} CPTABLEINFO;
static DWORD last_error;
static void SetLastError( DWORD error ) { last_error = error; }
static CPTABLEINFO gbk;
static const CPTABLEINFO *get_codepage_table( UINT codepage ) { return codepage == 936 ? &gbk : NULL; }

static void load_936( const char *path )
{
    FILE *f = fopen( path, "rb" );
    USHORT *ptr, hdr;
    long size;
    if (!f) exit( 2 );
    fseek( f, 0, SEEK_END );
    size = ftell( f );
    fseek( f, 0, SEEK_SET );
    ptr = malloc( size );
    if (fread( ptr, 1, size, f ) != (size_t)size) exit( 2 );
    fclose( f );
    hdr = ptr[0];
    gbk.CodePage = ptr[1];
    gbk.UniDefaultChar = ptr[4];
    ptr += hdr;
    gbk.WideCharTable = ptr + ptr[0] + 1;
    gbk.MultiByteTable = ++ptr;
    ptr += 256;
    if (*ptr++) ptr += 256;
    gbk.DBCSRanges = ptr;
    gbk.DBCSOffsets = ptr + 1;
}
'''

MAIN = r'''
static void *slurp( const char *path, long *size )
{
    FILE *f = fopen( path, "rb" );
    void *buf;
    if (!f) exit( 2 );
    fseek( f, 0, SEEK_END );
    *size = ftell( f );
    fseek( f, 0, SEEK_SET );
    buf = malloc( *size + 1 );
    if (fread( buf, 1, *size, f ) != (size_t)*size) exit( 2 );
    fclose( f );
    return buf;
}

int main( int argc, char **argv )
{
    long size;
    void *in;
    int len, need;

    load_936( argv[1] );
    in = slurp( argv[3], &size );
    if (!strcmp( argv[2], "encode" ))
    {
        char *out;
        BOOL used = TRUE;
        need = wcstombs_gb18030( 0, in, size / 2, NULL, 0, NULL, &used );
        out = malloc( need + 1 );
        len = wcstombs_gb18030( 0, in, size / 2, out, need, NULL, &used );
        if (len != need) return 3;
        printf( "%d %d\n", len, used );
        fwrite( out, 1, len, stderr );
    }
    else if (!strcmp( argv[2], "encode-strict" ))
    {
        len = wcstombs_gb18030( WC_ERR_INVALID_CHARS, in, size / 2, NULL, 0, NULL, NULL );
        printf( "%d %u\n", len, len ? 0 : last_error );
    }
    else if (!strcmp( argv[2], "decode" ))
    {
        WCHAR *out;
        need = mbstowcs_gb18030( 0, in, size, NULL, 0 );
        out = malloc( (need + 1) * sizeof(WCHAR) );
        len = mbstowcs_gb18030( 0, in, size, out, need );
        if (len != need) return 3;
        printf( "%d\n", len );
        fwrite( out, sizeof(WCHAR), len, stderr );
    }
    else if (!strcmp( argv[2], "decode-strict" ))
    {
        last_error = 0;
        len = mbstowcs_gb18030( MB_ERR_INVALID_CHARS, in, size, NULL, 0 );
        printf( "%d %u\n", len, len ? 0 : last_error );
    }
    else if (!strcmp( argv[2], "encode-short" ))
    {
        char out[3];
        last_error = 0;
        len = wcstombs_gb18030( 0, in, size / 2, out, 3, NULL, NULL );
        printf( "%d %u\n", len, last_error );
    }
    else if (!strcmp( argv[2], "decode-short" ))
    {
        WCHAR out[1];
        last_error = 0;
        len = mbstowcs_gb18030( 0, in, size, out, 1 );
        printf( "%d %u\n", len, last_error );
    }
    return 0;
}
'''


def extract(source: str) -> str:
    start = source.find('#define GB18030_SUPPLEMENTARY_LINEAR')
    end_marker = 'static int wcstombs_gb18030('
    end = source.find(end_marker)
    if start < 0 or end < 0:
        raise SystemExit('GB18030 conversion is missing from kernelbase/locale.c')
    end = source.find('\n}\n', end)
    return source[start:end + 3]


def run(binary, nls, mode, data, tmp):
    path = Path(tmp) / 'input.bin'
    path.write_bytes(data)
    result = subprocess.run([str(binary), str(nls), mode, str(path)], capture_output=True)
    if result.returncode != 0:
        raise SystemExit(f'{mode} exited {result.returncode}')
    return result.stdout.decode().split(), result.stderr


def main() -> int:
    root = Path(sys.argv[1])
    source = (root / 'dlls/kernelbase/locale.c').read_text()
    nls = root / 'nls/c_936.nls'
    failures = []
    with tempfile.TemporaryDirectory(prefix='runeon-gb18030.') as tmp:
        c_file = Path(tmp) / 'check.c'
        binary = Path(tmp) / 'check'
        c_file.write_text(PRELUDE + '#define CP_GB18030 54936\n' + extract(source) + MAIN)
        subprocess.run(['cc', '-std=c11', '-O1', '-Wall', '-Werror', '-Wno-unused-function',
                        '-o', str(binary), str(c_file)], check=True)

        bmp = ''.join(chr(u) for u in range(0x10000) if not 0xd800 <= u < 0xe000)
        supplementary = '\U00010000\U00020000\U0002a6d6\U0010ffff'
        for text in (bmp, supplementary):
            expected = text.encode('gb18030')
            (length, used), out = run(binary, nls, 'encode', text.encode('utf-16-le'), tmp)
            if out != expected or used != '0':
                failures.append(f'encode of {len(text)} characters differs')
            (count,), out = run(binary, nls, 'decode', expected, tmp)
            if out.decode('utf-16-le') != text:
                failures.append(f'decode of {len(text)} characters differs')

        # GB18030-2000, not 2005: these codes are where the editions differ.
        editions = {b'\xa8\xbc': '\ue7c7', b'\x81\x35\xf4\x37': '\u1e3f', b'\xa6\xd9': '\ue78d',
                    b'\x84\x31\x82\x36': '\ufe10'}
        for data, text in editions.items():
            (count,), out = run(binary, nls, 'decode', data, tmp)
            if out.decode('utf-16-le') != text:
                failures.append(f'{data.hex()} is not mapped as GB18030-2000')

        two_byte = b''.join(bytes([lead, trail]) for lead in range(0x81, 0xff)
                            for trail in list(range(0x40, 0x7f)) + list(range(0x80, 0xff)))
        (count,), out = run(binary, nls, 'decode', two_byte, tmp)
        if out.decode('utf-16-le') != two_byte.decode('gb18030'):
            failures.append('two-byte decode differs')

        invalid = {
            b'\x80': '�', b'\xff': '�', b'\x81': '�', b'\x81\x30': '�0',
            b'\x81\x20': '� ', b'\x84\x31\xa5\x30': '�', b'\x85\x30\x81\x30': '�',
            b'\xe4\x30\x81\x30': '�', b'a\x00b': 'a\x00b',
        }
        for data, text in invalid.items():
            (count,), out = run(binary, nls, 'decode', data, tmp)
            if out.decode('utf-16-le') != text:
                failures.append(f'decode of {data!r} gave {out.decode("utf-16-le")!r}')
            if '�' in text:
                (length, error), _ = run(binary, nls, 'decode-strict', data, tmp)
                if (length, error) != ('0', '1113'):
                    failures.append(f'strict decode of {data!r} did not fail')

        (length, used), out = run(binary, nls, 'encode', 'a\ud800b'.encode('utf-16-le', 'surrogatepass'), tmp)
        if out != b'a?b' or used != '1':
            failures.append('lone surrogate was not replaced')
        (length, error), _ = run(binary, nls, 'encode-strict', 'a\udc00'.encode('utf-16-le', 'surrogatepass'), tmp)
        if (length, error) != ('0', '1113'):
            failures.append('strict encode of a lone surrogate did not fail')
        (length, error), _ = run(binary, nls, 'encode-short', '\u00a5'.encode('utf-16-le'), tmp)
        if (length, error) != ('0', '122'):
            failures.append('a four-byte sequence was written into a three-byte buffer')
        (length, error), _ = run(binary, nls, 'decode-short', '\U00010000'.encode('gb18030'), tmp)
        if (length, error) != ('0', '122'):
            failures.append('a surrogate pair was written into a one-character buffer')

    for failure in failures:
        sys.stderr.write(failure + '\n')
    if failures:
        return 1
    print('PASS: code page 54936 matches GB18030-2000 for the BMP, supplementary planes and invalid input')
    return 0


if __name__ == '__main__':
    sys.exit(main())
