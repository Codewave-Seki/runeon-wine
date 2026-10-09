#!/usr/bin/env python3
"""Compile the patched winemac.drv two-dimensional BPTC decoder and check it.

decode_bptc_2d() is extracted from dlls/winemac.drv/opengl.c and built with
the system compiler under AddressSanitizer against the tree's own
dlls/winemac.drv/opengl_bcdec.h (shift checks are off: the bundled decoder
shifts signed values, which is outside this check). For each size, random BC7 blocks are decoded
by the function and, as the expected result, decoded block by block into a
padded image that is then cropped. Sizes cover whole blocks, partial edge
blocks and the 1x1, 2x2 and 3x3 mipmap tails. Invalid input (NULL data,
non-positive or oversized dimensions, a negative or short image size) must
return NULL.

Not covered: the GL wrappers, the unpack-state save and restore, the pixel
unpack buffer check, the RUNEON_GL_BPTC_DECODE gate, and running inside Wine
on macOS; a complete build and an application run are still required.
"""
from pathlib import Path
import os
import random
import subprocess
import sys
import tempfile

PRELUDE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char BYTE;
typedef int GLsizei;
#define BCDEC_IMPLEMENTATION
#define BCDEC_STATIC
#include "opengl_bcdec.h"
'''

MAIN = r'''
int main(int argc, char **argv)
{
    int width = atoi(argv[1]), height = atoi(argv[2]), size = atoi(argv[3]);
    long length;
    BYTE *data, *pixels;
    FILE *f = fopen(argv[4], "rb");

    if (!f) return 2;
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(length ? length : 1);
    if (fread(data, 1, length, f) != (size_t)length) return 2;
    fclose(f);
    pixels = decode_bptc_2d(!strcmp(argv[5], "null") ? NULL : data, width, height, size);
    if (!pixels)
    {
        puts("null");
        return 0;
    }
    fwrite(pixels, 1, (size_t)width * height * 4, stdout);
    free(pixels);
    free(data);
    return 0;
}
'''

REFERENCE = r'''
static int reference(const BYTE *data, int width, int height, BYTE *out)
{
    int blocks_x = (width + 3) / 4, blocks_y = (height + 3) / 4, x, y, row;
    BYTE *padded = malloc((size_t)blocks_x * 4 * blocks_y * 4 * 4);

    for (y = 0; y < blocks_y; y++)
        for (x = 0; x < blocks_x; x++)
            bcdec_bc7(data + (y * blocks_x + x) * 16, padded + ((size_t)y * 4 * blocks_x * 4 + x * 4) * 4, blocks_x * 4 * 4);
    for (row = 0; row < height; row++)
        memcpy(out + (size_t)row * width * 4, padded + (size_t)row * blocks_x * 4 * 4, (size_t)width * 4);
    free(padded);
    return 0;
}
'''

REFERENCE_MAIN = r'''
int main(int argc, char **argv)
{
    int width = atoi(argv[1]), height = atoi(argv[2]);
    long length;
    BYTE *data, *out;
    FILE *f = fopen(argv[3], "rb");

    if (!f) return 2;
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(length);
    if (fread(data, 1, length, f) != (size_t)length) return 2;
    fclose(f);
    out = malloc((size_t)width * height * 4);
    reference(data, width, height, out);
    fwrite(out, 1, (size_t)width * height * 4, stdout);
    return 0;
}
'''


def extract(source: str) -> str:
    start = source.find('static BYTE *decode_bptc_2d(')
    if start < 0:
        raise SystemExit('decode_bptc_2d() is missing from winemac.drv/opengl.c')
    end = source.find('\n}\n', start)
    return source[start:end + 3]


def build(tmp: Path, name: str, code: str, include: Path) -> Path:
    c_file = tmp / f'{name}.c'
    binary = tmp / name
    c_file.write_text(code)
    subprocess.run(['cc', '-std=c11', '-O1', '-g', '-Wall', '-Werror', '-Wno-unused-function',
                    '-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-sanitize-recover=all',
                    '-I', str(include), '-o', str(binary), str(c_file)], check=True)
    return binary


def run(binary: Path, args) -> bytes:
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    result = subprocess.run([str(binary), *map(str, args)], capture_output=True, env=env)
    if result.returncode != 0:
        raise SystemExit(f'{binary.name} {args} exited {result.returncode}: {result.stderr.decode()[:400]}')
    return result.stdout


def main() -> int:
    root = Path(sys.argv[1])
    driver = root / 'dlls/winemac.drv'
    decoder = extract((driver / 'opengl.c').read_text())
    failures = []
    rng = random.Random(0x8E8C)
    with tempfile.TemporaryDirectory(prefix='runeon-bptc.') as tmp_name:
        tmp = Path(tmp_name)
        check = build(tmp, 'check', PRELUDE + decoder + MAIN, driver)
        reference = build(tmp, 'reference', PRELUDE + REFERENCE + REFERENCE_MAIN, driver)
        blocks = tmp / 'blocks.bin'

        for width, height in [(4, 4), (8, 4), (1, 1), (2, 2), (3, 3), (5, 7), (6, 6), (17, 9), (64, 33), (128, 512)]:
            size = ((width + 3) // 4) * ((height + 3) // 4) * 16
            blocks.write_bytes(bytes(rng.randrange(256) for _ in range(size)))
            got = run(check, [width, height, size, blocks, 'data'])
            want = run(reference, [width, height, blocks])
            if got != want:
                failures.append(f'{width}x{height} differs from the cropped block decode')

        blocks.write_bytes(bytes(64))
        for width, height, size, mode in [(4, 4, 16, 'null'), (0, 4, 16, 'data'), (4, 0, 16, 'data'),
                                          (-4, 4, 16, 'data'), (16385, 1, 65536, 'data'), (4, 4, -1, 'data'),
                                          (8, 8, 63, 'data'), (5, 5, 48, 'data')]:
            if run(check, [width, height, size, blocks, mode]) != b'null\n':
                failures.append(f'{width}x{height} size {size} {mode} was accepted')

    for failure in failures:
        sys.stderr.write(failure + '\n')
    if failures:
        return 1
    print('PASS: two-dimensional BC7 decode matches the cropped block decode and rejects invalid input')
    return 0


if __name__ == '__main__':
    sys.exit(main())
