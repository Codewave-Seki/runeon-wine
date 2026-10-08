#!/usr/bin/env python3
"""Compile the patched x64 call_seh_handlers() with a leaf-only unwinder stub.

The stub can move Rsp once, as an unwinder that misreads a frame might, and
then unwinds every frame as a leaf, reading the return address at Rsp the way
RtlVirtualUnwind2() does. The thread stack ends at an inaccessible page, so a
read past StackBase crashes the check. It covers an aligned walk that ends at
StackBase and a stack pointer moved exactly to StackBase, across it or above
it. It does not run Wine, real unwind data, language handlers, the TEB frame
list, fibers or guard-page stack growth, and is_valid_frame() below is a copy
of the ntdll_misc.h helper.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

PRELUDE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
typedef int BOOL; typedef unsigned long DWORD; typedef uint64_t ULONG64;
typedef uintptr_t ULONG_PTR; typedef long NTSTATUS; typedef unsigned long ULONG;
#define TRUE 1
#define FALSE 0
#define STATUS_SUCCESS 0
#define STATUS_UNHANDLED_EXCEPTION ((NTSTATUS)0xc0000144)
#define STATUS_NONCONTINUABLE_EXCEPTION ((NTSTATUS)0xc0000025)
#define STATUS_INVALID_DISPOSITION ((NTSTATUS)0xc0000026)
#define EXCEPTION_NONCONTINUABLE 0x01
#define EXCEPTION_STACK_INVALID 0x08
#define EXCEPTION_NESTED_CALL 0x10
#define UNW_FLAG_NHANDLER 0
#define UNW_FLAG_EHANDLER 1
#define TRACE(...)
#define ERR(...)
enum { ExceptionContinueExecution, ExceptionContinueSearch, ExceptionNestedException, ExceptionCollidedUnwind };
typedef DWORD (*PEXCEPTION_ROUTINE)(void);
typedef struct EXCEPTION_REGISTRATION_RECORD { struct EXCEPTION_REGISTRATION_RECORD *Prev; PEXCEPTION_ROUTINE Handler; } EXCEPTION_REGISTRATION_RECORD;
typedef struct { DWORD ExceptionFlags; } EXCEPTION_RECORD;
typedef struct { DWORD ContextFlags; ULONG64 Rip, Rsp; } CONTEXT;
typedef struct { int unused; } UNWIND_HISTORY_TABLE;
typedef struct { ULONG64 ControlPc, ImageBase, EstablisherFrame, TargetIp; void *FunctionEntry;
                 CONTEXT *ContextRecord; PEXCEPTION_ROUTINE LanguageHandler; void *HandlerData;
                 UNWIND_HISTORY_TABLE *HistoryTable; } DISPATCHER_CONTEXT;
typedef struct { struct { EXCEPTION_REGISTRATION_RECORD *ExceptionList; void *StackBase, *StackLimit; } Tib; } TEB;
static TEB teb;
static TEB *NtCurrentTeb(void) { return &teb; }
/* Copy of is_valid_frame() from dlls/ntdll/ntdll_misc.h. */
static BOOL is_valid_frame( ULONG_PTR frame )
{
    if (frame & (sizeof(void*) - 1)) return FALSE;
    return ((void *)frame >= NtCurrentTeb()->Tib.StackLimit &&
            (void *)frame <= NtCurrentTeb()->Tib.StackBase);
}
static int unwinds;
static ULONG64 jump_rsp;
/* The first frame may move Rsp anywhere; every later frame is a leaf, read
 * the same way RtlVirtualUnwind2() does. */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    (void)type;
    unwinds++;
    dispatch->EstablisherFrame = context->Rsp;
    dispatch->LanguageHandler = NULL;
    if (jump_rsp)
    {
        context->Rsp = jump_rsp;
        jump_rsp = 0;
        return STATUS_SUCCESS;
    }
    context->Rip = *(ULONG64 *)context->Rsp;
    context->Rsp += sizeof(ULONG64);
    return STATUS_SUCCESS;
}
static DWORD call_seh_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame, CONTEXT *context,
                               void *dispatch, PEXCEPTION_ROUTINE handler )
{ (void)rec; (void)frame; (void)context; (void)dispatch; (void)handler; return ExceptionContinueSearch; }
static void RtlVirtualUnwind( ULONG type, ULONG64 base, ULONG64 pc, void *function, CONTEXT *context,
                              void *data, ULONG_PTR *frame, void *ctx )
{ (void)type; (void)base; (void)pc; (void)function; (void)context; (void)data; (void)frame; (void)ctx; }
'''

MAIN = r'''
static NTSTATUS run( ULONG64 rsp, ULONG64 jump, DWORD *flags )
{
    EXCEPTION_RECORD rec = { 0 };
    CONTEXT context = { 0 };
    NTSTATUS status;
    context.Rsp = rsp;
    jump_rsp = jump;
    unwinds = 0;
    status = call_seh_handlers( &rec, &context );
    *flags = rec.ExceptionFlags;
    return status;
}

int main(void)
{
    long page = sysconf( _SC_PAGESIZE );
    char *map = mmap( NULL, 2 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0 );
    char *base;
    ULONG64 top;
    DWORD flags;
    assert( map != MAP_FAILED );
    base = map + page;
    assert( !mprotect( base, page, PROT_NONE ) );  /* nothing is readable above StackBase */
    memset( map, 0, page );
    teb.Tib.StackLimit = map;
    teb.Tib.StackBase = base;
    teb.Tib.ExceptionList = (EXCEPTION_REGISTRATION_RECORD *)~(ULONG_PTR)0;
    top = (ULONG64)base;

    /* An aligned leaf walk ends at StackBase, as before. */
    assert( run( top - 4 * sizeof(ULONG64), 0, &flags ) == STATUS_UNHANDLED_EXCEPTION );
    assert( unwinds == 4 && !(flags & EXCEPTION_STACK_INVALID) );

    /* A frame that unwinds exactly to StackBase also ends the walk, as before. */
    assert( run( top - 64, top, &flags ) == STATUS_UNHANDLED_EXCEPTION );
    assert( unwinds == 1 && !(flags & EXCEPTION_STACK_INVALID) );

    /* A frame that leaves less than one slot below StackBase: the next leaf
     * read would cross it. */
    assert( run( top - 64, top - 4, &flags ) == STATUS_UNHANDLED_EXCEPTION );
    assert( unwinds == 1 && (flags & EXCEPTION_STACK_INVALID) );

    /* A frame that unwinds above StackBase. */
    assert( run( top - 64, top + sizeof(ULONG64), &flags ) == STATUS_UNHANDLED_EXCEPTION );
    assert( unwinds == 1 && (flags & EXCEPTION_STACK_INVALID) );

    puts( "PASS" );
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
    source = (root / 'dlls/ntdll/signal_x86_64.c').read_text()
    if 'is_valid_unwind_rsp' not in source:
        raise SystemExit('patched stack pointer check is missing')
    helper = extract(source, 'static inline BOOL is_valid_unwind_rsp(')
    dispatch = extract(source, 'NTSTATUS call_seh_handlers(')
    dispatch = re.sub(r'\bNTSTATUS call_seh_handlers\(', 'static NTSTATUS call_seh_handlers(', dispatch, count=1)
    with tempfile.TemporaryDirectory(prefix='runeon-seh-bounds.') as temp:
        c_file = Path(temp) / 'check.c'
        binary = Path(temp) / 'check'
        c_file.write_text(PRELUDE + helper + dispatch + MAIN)
        subprocess.run(['cc', '-std=c11', '-D_DEFAULT_SOURCE', '-D_DARWIN_C_SOURCE', '-Wall', '-Werror',
                        '-Wno-unused-function', '-o', str(binary), str(c_file)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if result.returncode != 0 or result.stdout.strip() != 'PASS':
            sys.stderr.write(result.stdout + result.stderr)
            return 1
    print('PASS: SEH dispatch stops at the thread stack bounds without reading past StackBase')
    return 0


if __name__ == '__main__':
    sys.exit(main())
