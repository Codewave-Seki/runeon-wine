/* LGPL-2.1-or-later. msync alert fd probe for 0150.
 * Under WINEMSYNC a thread's in-process alert "fd" is an index into the
 * server's shared object table, not a descriptor, but thread exit used to
 * close() it, closing whatever file the process had under that number.
 * The probe opens many files and reads one byte from each so the client
 * caches their Unix fds, lets short-lived threads make an alertable wait and
 * exit, then reads every file again, with a pause before and after the
 * threads. A read that fails is a descriptor some exiting thread closed.
 * Run with WINEMSYNC=1 from the first command of the wineserver, and once
 * without it as a control.
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror -o msync-alert-fd-probe64.exe msync-alert-fd-probe.c
 *   i686-w64-mingw32-gcc -O2 -Wall -Werror -o msync-alert-fd-probe32.exe msync-alert-fd-probe.c
 * Prints "failed reads: N" and exits 1 when N > 0. It does not cover the
 * esync or server-side wait paths, or files opened after the threads exit. */
#include <windows.h>
#include <stdio.h>

#define FILES 6000
#define THREADS 400

static HANDLE never_event;

static DWORD WINAPI alertable_waiter(void *arg)
{
    (void)arg;
    WaitForSingleObjectEx(never_event, 1, TRUE);
    return 0;
}

static BOOL read_one(HANDLE file)
{
    char c;
    DWORD got = 0;

    if (SetFilePointer(file, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) return FALSE;
    return ReadFile(file, &c, 1, &got, NULL) && got == 1;
}

int main(void)
{
    static HANDLE files[FILES];
    char dir[MAX_PATH], path[MAX_PATH + 32];
    DWORD written, failed = 0, opened = 0, i;

    never_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    GetTempPathA(sizeof(dir), dir);
    for (i = 0; i < FILES; i++)
    {
        snprintf(path, sizeof(path), "%smsync-alert-fd-%lu.tmp", dir, i);
        files[i] = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
        if (files[i] == INVALID_HANDLE_VALUE) break;
        WriteFile(files[i], "x", 1, &written, NULL);
        read_one(files[i]);
        opened++;
    }

    /* Without these pauses the failure did not show in local runs. */
    Sleep(8000);
    for (i = 0; i < THREADS; i++)
    {
        HANDLE thread = CreateThread(NULL, 0, alertable_waiter, NULL, 0, NULL);
        if (!thread) break;
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }

    Sleep(8000);
    for (i = 0; i < opened; i++)
    {
        if (!read_one(files[i])) failed++;
        CloseHandle(files[i]);
    }
    printf("opened files: %lu, threads: %d, failed reads: %lu\n", opened, THREADS, failed);
    return failed ? 1 : 0;
}
