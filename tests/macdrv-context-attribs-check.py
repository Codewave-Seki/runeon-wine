#!/usr/bin/env python3
"""Compile the patched winemac macdrv_context_create() attribute checks.

CGL context creation is replaced by a stub, so this covers only the
WGL_ARB_create_context attribute handling: a 3.2 core request carrying
WGL_CONTEXT_OPENGL_NO_ERROR_ARB and no forward-compatible flag (as SDL 2.0.16
sends it) is accepted, each half is also checked on its own, and
compatibility profiles, 3.0/3.1, versions above the host maximum and unknown
attributes are still refused with the same errors. CX_FWD_COMPAT_GL_CTX is
cleared so the CrossOver hack cannot supply the flag, then checked set. It
does not run Wine, share contexts or create a real OpenGL context.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

PRELUDE = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
typedef int BOOL; typedef int GLint; typedef void *CGLContextObj;
#define TRUE 1
#define FALSE 0
#define TRACE(...)
#define WARN(...)
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_OPENGL_NO_ERROR_ARB 0x31B3
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x2
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x1
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x2
#define ERROR_INVALID_PARAMETER 87
#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096
struct macdrv_context { int format; GLint renderer_id; CGLContextObj cglcontext; };
static struct { int max_major, max_minor; } gl_info = { 4, 1 };
static unsigned int last_error;
static int created_major;
static void RtlSetLastWin32Error(unsigned int error) { last_error = error; }
static const char *debugstr_attrib(int attr, int value) { (void)attr; (void)value; return ""; }
static BOOL create_context(struct macdrv_context *context, CGLContextObj share, int major)
{
    (void)context; (void)share;
    created_major = major;
    return TRUE;
}
'''

MAIN = r'''
static BOOL create(const int *attribs)
{
    void *context = NULL;
    BOOL ret;
    last_error = 0;
    created_major = 0;
    ret = macdrv_context_create(1, NULL, attribs, &context);
    free(context);
    return ret;
}

int main(void)
{
    /* What SDL 2.0.16 sends for a 3.2 core request (Rune Factory 4 Special). */
    static const int sdl_core[] = { 0x2091, 3, 0x2092, 2, 0x9126, 1, 0x31B3, 0, 0 };
    static const int no_error_on[] = { 0x2091, 4, 0x2092, 1, 0x9126, 1, 0x31B3, 1, 0 };
    static const int forward[] = { 0x2091, 3, 0x2092, 3, 0x2094, 2, 0 };
    static const int compat[] = { 0x2091, 3, 0x2092, 2, 0x9126, 2, 0 };
    static const int too_new[] = { 0x2091, 4, 0x2092, 3, 0x9126, 1, 0 };
    static const int unknown[] = { 0x2091, 3, 0x2092, 2, 0x2095, 1, 0 };
    static const int legacy_forward[] = { 0x2091, 2, 0x2092, 1, 0x2094, 2, 0 };

    static const int core_only[] = { 0x2091, 3, 0x2092, 2, 0x9126, 1, 0 };
    static const int legacy_no_error[] = { 0x2091, 2, 0x2092, 1, 0x31B3, 1, 0 };
    static const int v30[] = { 0x2091, 3, 0x2092, 0, 0 };
    static const int v31[] = { 0x2091, 3, 0x2092, 1, 0x9126, 1, 0 };

    unsetenv("CX_FWD_COMPAT_GL_CTX");
    assert(create(sdl_core) && created_major == 3);
    /* Each half alone: no forward-compatible flag, and no-error on a legacy context. */
    assert(create(core_only) && created_major == 3);
    assert(create(legacy_no_error) && created_major == 2);
    assert(!create(v30) && last_error == ERROR_INVALID_VERSION_ARB);
    assert(!create(v31) && last_error == ERROR_INVALID_VERSION_ARB);
    assert(create(no_error_on) && created_major == 4);
    assert(create(forward) && created_major == 3);
    assert(!create(compat) && last_error == ERROR_INVALID_PROFILE_ARB);
    assert(!create(too_new) && last_error == ERROR_INVALID_VERSION_ARB);
    assert(!create(unknown) && last_error == ERROR_INVALID_PARAMETER);
    assert(!create(legacy_forward) && last_error == ERROR_INVALID_VERSION_ARB);
    assert(create(NULL) && created_major == 1);
    setenv("CX_FWD_COMPAT_GL_CTX", "1", 1);
    assert(create(sdl_core) && created_major == 3);
    puts("PASS");
    return 0;
}
'''


def extract(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise SystemExit(f'missing {signature!r}')
    end = source.find('\n}\n', start)
    if end < 0:
        raise SystemExit(f'unterminated {signature!r}')
    return source[start:end + 3]


def main() -> int:
    root = Path(sys.argv[1])
    source = (root / 'dlls/winemac.drv/opengl.c').read_text()
    function = extract(source, 'static BOOL macdrv_context_create(')
    if 'WGL_CONTEXT_OPENGL_NO_ERROR_ARB' not in function:
        raise SystemExit('patched attribute handling is missing')
    with tempfile.TemporaryDirectory(prefix='runeon-macdrv-context.') as temp:
        c_file = Path(temp) / 'check.c'
        binary = Path(temp) / 'check'
        c_file.write_text(PRELUDE + function + MAIN)
        subprocess.run(['cc', '-std=c11', '-D_POSIX_C_SOURCE=200112L', '-Wall', '-Werror', '-Wno-unused-function',
                        '-o', str(binary), str(c_file)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if result.returncode != 0 or result.stdout.strip() != 'PASS':
            sys.stderr.write(result.stdout + result.stderr)
            return 1
    print('PASS: macdrv accepts SDL-style 3.2 core requests and keeps its other attribute checks')
    return 0


if __name__ == '__main__':
    sys.exit(main())
