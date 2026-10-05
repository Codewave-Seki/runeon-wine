#!/usr/bin/env python3
"""Compile the actual Steam selector with deterministic OS stubs.

Covers opt-in, parent/grant/context validation, layout precedence, command and
length preservation, and failure fallback. It does not run Wine or the helper.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

PRELUDE = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
typedef wchar_t WCHAR; typedef int BOOL; typedef unsigned DWORD;
typedef int NTSTATUS; typedef void *HANDLE;
typedef struct { size_t Length, MaximumLength; WCHAR *Buffer; } UNICODE_STRING;
typedef struct { UNICODE_STRING ImagePathName, CommandLine; void *Environment; struct { UNICODE_STRING DosPath; } CurrentDirectory; } RTL_USER_PROCESS_PARAMETERS;
typedef struct { RTL_USER_PROCESS_PARAMETERS *ProcessParameters; } PEB;
typedef struct { PEB *Peb; } TEB;
typedef struct { int Machine, ImageCharacteristics; } SECTION_IMAGE_INFORMATION;
#define TRACE(...)
#define TRUE 1
#define FALSE 0
#define STATUS_SUCCESS 0
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define GENERIC_READ 1
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 1
#define STANDARD_RIGHTS_REQUIRED 1
#define SECTION_QUERY 2
#define PAGE_READONLY 1
#define SEC_IMAGE 1
#define SectionImageInformation 1
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_DLL 0x2000
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define INVALID_FILE_ATTRIBUTES (~0u)
#define FILE_ATTRIBUTE_DIRECTORY 16
static RTL_USER_PROCESS_PARAMETERS parent;
static PEB peb = { &parent }; static TEB teb = { &peb };
static TEB *NtCurrentTeb(void) { return &teb; }
static int nwjs, guard, layout, fail, grant_ok = 1, app_ok = 1;
static int wcsnicmp(const WCHAR *a, const WCHAR *b, size_t n) {
    while (n--) { int x = towlower(*a++), y = towlower(*b++); if (x != y) return x-y; if (!x) return 0; }
    return 0;
}
static int wcsicmp(const WCHAR *a, const WCHAR *b) { return wcsnicmp(a,b,wcslen(a)+1); }
static void RtlInitUnicodeString(UNICODE_STRING *s, const WCHAR *v) {
    s->Buffer=(WCHAR *)v; s->Length=wcslen(v)*sizeof(WCHAR); s->MaximumLength=s->Length+sizeof(WCHAR);
}
static int RtlQueryEnvironmentVariable_U(void *e, UNICODE_STRING *key, UNICODE_STRING *out) {
    WCHAR text[65]; const WCHAR *value = text;
    if (!wcscmp(key->Buffer,L"RUNEON_NWJS_AUTO_V1")) value = nwjs ? L"1" : L"0";
    else if (!wcscmp(key->Buffer,L"RUNEON_D3D12ON7_GUARD_V1")) value = guard ? L"1" : L"0";
    else if (!wcscmp(key->Buffer,L"SteamAppId")) value = app_ok ? L"123" : L"0";
    else { for (int i=0;i<64;i++) text[i]=grant_ok ? L'a' : L'!'; text[64]=0; }
    size_t bytes=(wcslen(value)+1)*sizeof(WCHAR); if(bytes>out->MaximumLength) return 1;
    memcpy(out->Buffer,value,bytes); out->Length=bytes-sizeof(WCHAR); return 0;
}
static HANDLE CreateFileW(const WCHAR *p, int a,int b,void*c,int d,int e,int f) { return fail==1 ? INVALID_HANDLE_VALUE : (HANDLE)1; }
static int CloseHandle(HANDLE h) { return 1; }
static int NtCreateSection(HANDLE *s,int a,void*b,void*c,int d,int e,HANDLE f) { *s=(HANDLE)2; return fail==2; }
static int NtQuerySection(HANDLE h,int a,SECTION_IMAGE_INFORMATION *i,size_t n,void*b) {
    i->Machine=fail==3 ? 0x14c : IMAGE_FILE_MACHINE_AMD64; i->ImageCharacteristics=fail==4 ? IMAGE_FILE_DLL : 0; return 0;
}
static DWORD GetFileAttributesW(const WCHAR *p) {
    if(wcsstr(p,L"runeon-input-source-launch.exe")) return fail==5 ? INVALID_FILE_ATTRIBUTES : 0;
    if(wcsstr(p,L"12on7\\d3d12.dll")) return layout&1 ? 0 : INVALID_FILE_ATTRIBUTES;
    return layout&2 ? 0 : INVALID_FILE_ATTRIBUTES;
}
static void *GetProcessHeap(void) { return NULL; }
static void *HeapAlloc(void *h,int f,size_t n) { return fail==6 ? NULL : malloc(n); }
'''
TEST = r'''
#define CREATE_UNICODE_ENVIRONMENT 0x400
#define EXTENDED_STARTUPINFO_PRESENT 0x80000
struct startup { DWORD dwFlags; unsigned short cbReserved2; void *lpTitle; };
static int gate(HANDLE token, BOOL inherit, void *process_attr, void *thread_attr,
                DWORD flags, struct startup *startup_info) {
    GATE_EXPRESSION
    return allow_guard;
}
static void check(RTL_USER_PROCESS_PARAMETERS *p, int allow, const WCHAR *mode) {
    WCHAR *out=(void *)1; BOOL prepare=TRUE;
    assert(!runeon_nwjs_launch_command(p,allow,&out,&prepare));
    if(!mode) assert(!out && !prepare);
    else { assert(prepare == !wcscmp(mode,L"d3d12on7-prepare")); assert(out && wcsstr(out,mode)); size_t n=wcslen(out), m=wcslen(p->CommandLine.Buffer);
        assert(n>=m && !wcscmp(out+n-m,p->CommandLine.Buffer)); free(out); }
}
int main(void) {
    struct startup startup = {0};
    assert(gate(0,0,0,0,0,&startup)); assert(gate(0,0,0,0,CREATE_UNICODE_ENVIRONMENT,&startup));
    assert(gate(0,1,(void *)1,(void *)1,4,&startup));
    assert(!gate((HANDLE)1,0,0,0,0,&startup));
    assert(!gate(0,0,0,0,EXTENDED_STARTUPINFO_PRESENT,&startup));
    startup.dwFlags=1; startup.cbReserved2=4; startup.lpTitle=(void *)1;
    assert(gate(0,1,(void *)1,(void *)1,4,&startup));
    RTL_USER_PROCESS_PARAMETERS p = {0};
    RtlInitUnicodeString(&parent.ImagePathName,L"C:\\Steam\\steam.exe");
    RtlInitUnicodeString(&p.ImagePathName,L"C:\\Games\\Example\\game.exe");
    RtlInitUnicodeString(&p.CommandLine,L"\"C:\\Games\\Example\\game.exe\" -x \"two words\"");
    layout=1; check(&p,1,NULL); guard=1; check(&p,1,L"d3d12on7-prepare"); check(&p,0,NULL);
    layout=0; check(&p,1,NULL); layout=1;
    grant_ok=0; check(&p,1,NULL); grant_ok=1; app_ok=0; check(&p,1,NULL); app_ok=1;
    RtlInitUnicodeString(&parent.ImagePathName,L"C:\\not-steam.exe"); check(&p,1,NULL);
    RtlInitUnicodeString(&parent.ImagePathName,L"C:\\Steam\\STEAM.EXE"); check(&p,1,L"d3d12on7-prepare");
    for(fail=1;fail<=6;fail++) check(&p,1,NULL); fail=0;
    nwjs=1; layout=3; check(&p,1,L"nwjs-auto"); guard=0; check(&p,1,L"nwjs-auto");
    layout=1; check(&p,1,NULL); guard=1; check(&p,1,L"d3d12on7-prepare");
    RtlInitUnicodeString(&p.CommandLine,L"\"C:\\other.exe\""); check(&p,1,NULL);
    RtlInitUnicodeString(&p.CommandLine,L"\"C:\\Games\\Example\\game.exe\"suffix"); check(&p,1,NULL);
    RtlInitUnicodeString(&p.CommandLine,L"\"C:\\Games\\Example\\game.exe\"\n"); check(&p,1,NULL);
    WCHAR huge[32768]; wcscpy(huge,L"\"C:\\Games\\Example\\game.exe\" ");
    size_t start=wcslen(huge); for(size_t i=start;i<32766;i++) huge[i]='a'; huge[32766]=0;
    RtlInitUnicodeString(&p.CommandLine,huge); check(&p,1,NULL);
    puts("Steam layout selector: selector and actual creation gate passed; no runtime execution");
}
'''
PREPARE_PRELUDE = r'''
typedef unsigned long long ULONGLONG;
typedef struct { HANDLE Process, Thread; } RTL_USER_PROCESS_INFORMATION;
typedef struct { unsigned cb; } STARTUPINFOW;
#define CREATE_NO_WINDOW 0x8000000
#define CREATE_UNICODE_ENVIRONMENT 0x400
#define WAIT_OBJECT_0 0
#define WAIT_ABANDONED 128
#define INFINITE (~0u)
static int prepared, resumed, waited, lock_held, prepare_failure;
static WCHAR lock_name[80];
static const WCHAR *expected_cwd = L"C:\\Games\\Example";
static WCHAR RtlDowncaseUnicodeChar(WCHAR c) { return towlower(c); }
static RTL_USER_PROCESS_PARAMETERS helper_params;
static HANDLE CreateMutexW(void *a,BOOL b,const WCHAR *name) { assert(wcsstr(name,L"RuneonD3D12On7GuardV1-")); wcscpy(lock_name,name); return prepare_failure==1 ? NULL : (HANDLE)3; }
static DWORD WaitForSingleObject(HANDLE h,DWORD ms) { assert(ms==INFINITE); lock_held=1; return WAIT_OBJECT_0; }
static RTL_USER_PROCESS_PARAMETERS *create_process_params(const WCHAR *exe,const WCHAR *command,const WCHAR *cwd,void *env,DWORD flags,STARTUPINFOW *startup) {
    assert(lock_held && !wcscmp(exe,runeon_nwjs_wrapperW));
    assert(wcsstr(command,L"d3d12on7-prepare") && !wcscmp(cwd,expected_cwd));
    assert(env==(void *)99 && flags==(CREATE_UNICODE_ENVIRONMENT|CREATE_NO_WINDOW));
    assert(startup->cb==sizeof(*startup));
    return prepare_failure==2 ? NULL : &helper_params;
}
static NTSTATUS create_nt_process(HANDLE token,HANDLE debug,void *psa,void *tsa,DWORD flags,RTL_USER_PROCESS_PARAMETERS *p,RTL_USER_PROCESS_INFORMATION *out,HANDLE parent,unsigned short machine,void *handles,void *jobs) {
    assert(lock_held && !token && !debug && !psa && !tsa && !flags && p==&helper_params && !parent && !machine && !handles && !jobs);
    if(prepare_failure==3) return -1;
    ++prepared; out->Process=(HANDLE)4; out->Thread=(HANDLE)5; return 0;
}
static void RtlDestroyProcessParameters(RTL_USER_PROCESS_PARAMETERS *p) { assert(p==&helper_params); }
static NTSTATUS NtResumeThread(HANDLE h,void *n) { assert(h==(HANDLE)5 && lock_held); ++resumed; return prepare_failure==4 || prepare_failure==5 ? -2 : 0; }
static NTSTATUS NtWaitForSingleObject(HANDLE h,BOOL alertable,void *timeout) { assert(h==(HANDLE)4 && !alertable && !timeout && lock_held); ++waited; return prepare_failure>=6 && (waited==1 || prepare_failure==8) ? -3 : 0; }
static NTSTATUS NtTerminateProcess(HANDLE h,NTSTATUS s) { assert(h==(HANDLE)4); return prepare_failure==5 || prepare_failure==7 ? -4 : 0; }
static void NtClose(HANDLE h) { assert(h==(HANDLE)4 || h==(HANDLE)5); }
'''
PREPARE_TEST = r'''
static void check_preparation(void) {
    RTL_USER_PROCESS_PARAMETERS p={0};
    RtlInitUnicodeString(&p.ImagePathName,L"C:\\Games\\Example\\game.exe");
    RtlInitUnicodeString(&p.CurrentDirectory.DosPath,L"C:\\Games\\Example");
    p.Environment=(void *)99;
    RTL_USER_PROCESS_PARAMETERS before=p;
    WCHAR command[]=L"helper --runtime d3d12on7-prepare -- original";
    for(prepare_failure=0;prepare_failure<9;prepare_failure++) {
        prepared=resumed=waited=lock_held=0;
        HANDLE mutex;
        NTSTATUS status=runeon_d3d12on7_prepare(&p,command,&mutex);
        assert((status!=0)==(prepare_failure==5 || prepare_failure==7 || prepare_failure==8));
        assert(!memcmp(&p,&before,sizeof(p)));
        assert((mutex!=NULL)==(prepare_failure!=1));
        if(!prepare_failure || prepare_failure>=4) assert(prepared==1 && resumed==1);
        else assert(!prepared && !resumed && !waited);
        /* The caller retains the mutex through original creation. */
        assert(lock_held==(prepare_failure!=1));
    }
    prepare_failure=0;
    HANDLE mutex;
    WCHAR without_slash[80];
    assert(!runeon_d3d12on7_prepare(&p,command,&mutex)); wcscpy(without_slash,lock_name);
    expected_cwd=L"C:\\OtherWorkingDirectory\\";
    RtlInitUnicodeString(&p.CurrentDirectory.DosPath,expected_cwd);
    assert(!runeon_d3d12on7_prepare(&p,command,&mutex) && !wcscmp(without_slash,lock_name));
    RtlInitUnicodeString(&p.ImagePathName,L"c:\\games\\example\\other.exe");
    assert(!runeon_d3d12on7_prepare(&p,command,&mutex) && !wcscmp(without_slash,lock_name));
    RtlInitUnicodeString(&p.ImagePathName,L"C:\\Games\\Different\\game.exe");
    assert(!runeon_d3d12on7_prepare(&p,command,&mutex) && wcscmp(without_slash,lock_name));
}
'''

def main():
    source = (Path(sys.argv[1])/'dlls/kernelbase/process.c').read_text()
    begin=source.index('static const WCHAR runeon_nwjs_wrapperW')
    end=source.index('\n\nstatic BOOL is_steamwebhelper_command',begin)
    function=source[begin:end]
    # POSIX wprintf uses %ls where Windows uses %s for wide strings.
    function=function.replace('L"\\"%s\\" --runtime %s -- %s"','L"\\"%ls\\" --runtime %ls -- %ls"')
    with tempfile.TemporaryDirectory(prefix='wine-launch-check-') as directory:
        c=Path(directory)/'check.c'; binary=Path(directory)/'check'
        gate_begin=source.index('BOOL allow_guard =')
        gate=source[gate_begin:source.index(';',gate_begin)+1]
        prep_start=source.index('static NTSTATUS runeon_d3d12on7_prepare(')
        prep_end=source.index('\n/**********************************************************************',prep_start)
        preparation=source[prep_start:prep_end]
        test=TEST.replace('GATE_EXPRESSION',gate).replace('    struct startup startup = {0};','    check_preparation();\n    struct startup startup = {0};')
        c.write_text(PRELUDE+function+PREPARE_PRELUDE+preparation+PREPARE_TEST+test)
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-parameter',str(c),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
if __name__ == '__main__': main()
