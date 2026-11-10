/*
 * Check that 64-bit system calls read only the low byte of a BOOLEAN argument.
 *
 * Covers patch 0173. Windows and the x64 calling convention leave the bits
 * above a BOOLEAN undefined, but clang compiles the Unix side assuming the
 * caller extended it to 32 bits, so a caller that sets only the low byte can
 * be read as TRUE. Each call below passes a value whose low byte is 0 and
 * whose other bits are not, through a ULONG prototype:
 *   - NtQueryDefaultLocale(user) in a register (the WineHQ test);
 *   - NtQueryDirectoryObject(restart), the fifth argument (on the stack on
 *     the Windows side, in r8 on the Unix side): after one entry has been
 *     read, a "FALSE" restart must return the next entry, not the first one
 *     again (with the defect, a loop over the directory never ends).
 * It changes the user locale only when it equals the system locale, and then
 * restores it. It does not reach the trampoline path for a BOOLEAN passed as
 * the seventh or a later argument.
 *
 * It does not cover the other wrapped system calls, the 32-bit (WoW64) path,
 * which truncates arguments in C, or callers inside Wine.
 *
 * Build: x86_64-w64-mingw32-gcc -O1 -Wall -Werror -o syscall-boolean-args-probe.exe
 *        syscall-boolean-args-probe.c
 * Usage: wine syscall-boolean-args-probe.exe
 * Exit:  0 when every call reads only the low byte, 1 when one does not, 2 on setup failure.
 */

#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <string.h>

#ifndef DIRECTORY_QUERY
#define DIRECTORY_QUERY 0x0001
#endif

typedef NTSTATUS (WINAPI *query_locale_fn)(ULONG user, LCID *lcid);
typedef NTSTATUS (WINAPI *set_locale_fn)(BOOLEAN user, LCID lcid);
typedef NTSTATUS (WINAPI *open_dir_fn)(HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr);
typedef NTSTATUS (WINAPI *query_dir_fn)(HANDLE handle, void *buffer, ULONG size, ULONG single_entry,
                                        ULONG restart, ULONG *context, ULONG *ret_size);

typedef struct
{
    UNICODE_STRING name;
    UNICODE_STRING type;
} dir_entry;

static int failures;

static int check_locale(HMODULE ntdll)
{
    query_locale_fn query = (query_locale_fn)GetProcAddress(ntdll, "NtQueryDefaultLocale");
    set_locale_fn set = (set_locale_fn)GetProcAddress(ntdll, "NtSetDefaultLocale");
    LCID system_lcid, user_lcid, lcid, restore = 0;
    ULONG i;

    if (!query || !set) return 0;
    query(0, &system_lcid);
    query(1, &user_lcid);
    if (system_lcid == user_lcid)
    {
        restore = user_lcid;
        /* Two installed locales, so the switch cannot fail on a bogus LCID. */
        user_lcid = (system_lcid == 0x0409) ? 0x0407 : 0x0409;
        if (set(TRUE, user_lcid)) return 0;
    }
    for (i = 1; i < 0x10000; i <<= 1)
    {
        LCID expect = (i & 0xff) ? user_lcid : system_lcid;
        query(i, &lcid);
        if (lcid != expect)
        {
            printf("FAIL: NtQueryDefaultLocale(%#lx) returned %04lx, expected %04lx\n",
                   (unsigned long)i, (unsigned long)lcid, (unsigned long)expect);
            failures++;
        }
    }
    if (restore) set(TRUE, restore);
    return 1;
}

static int check_directory(HMODULE ntdll)
{
    open_dir_fn open_dir = (open_dir_fn)GetProcAddress(ntdll, "NtOpenDirectoryObject");
    query_dir_fn query = (query_dir_fn)GetProcAddress(ntdll, "NtQueryDirectoryObject");
    WCHAR first[64], second[64];
    union { dir_entry entry; char bytes[1024]; } buffer;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name = { 2, 4, (WCHAR *)L"\\" };
    ULONG context = 0, size;
    HANDLE dir;
    NTSTATUS status;

    if (!open_dir || !query) return 0;
    InitializeObjectAttributes(&attr, &name, 0, NULL, NULL);
    if (open_dir(&dir, DIRECTORY_QUERY, &attr)) return 0;

    status = query(dir, &buffer, sizeof(buffer), TRUE, TRUE, &context, &size);
    if (status) { CloseHandle(dir); return 0; }
    lstrcpynW(first, buffer.entry.name.Buffer, ARRAYSIZE(first));

    status = query(dir, &buffer, sizeof(buffer), TRUE, 0x100, &context, &size);
    if (status) { CloseHandle(dir); return 0; }
    lstrcpynW(second, buffer.entry.name.Buffer, ARRAYSIZE(second));
    CloseHandle(dir);

    printf("directory entries: %ls, %ls\n", first, second);
    if (!wcscmp(first, second))
    {
        printf("FAIL: NtQueryDirectoryObject(restart 0x100) restarted the enumeration\n");
        failures++;
    }
    return 1;
}

int main(void)
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

    setvbuf(stdout, NULL, _IONBF, 0);
    if (!ntdll || !check_locale(ntdll) || !check_directory(ntdll)) return 2;
    printf("%s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
