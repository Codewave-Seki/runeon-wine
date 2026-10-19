/* LGPL-2.1-or-later. Checks per-thread CPU times (0131).
 * A worker spins for about 800 ms while the main thread sleeps. GetThreadTimes()
 * must give the worker at least 500 ms of CPU time and the main thread less than
 * 300 ms. The old macOS code answers zero for the worker and the whole process's
 * time for the main thread. */
#include <windows.h>
#include <stdio.h>

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

static ULONGLONG cpu_ms(HANDLE thread)
{
    FILETIME create, exit_time, kernel, user;
    if (!GetThreadTimes(thread, &create, &exit_time, &kernel, &user)) return ~0ull;
    return ((((ULONGLONG)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime)
          + (((ULONGLONG)user.dwHighDateTime << 32) | user.dwLowDateTime)) / 10000;
}

static volatile LONG stop;

static DWORD WINAPI spin(void *arg)
{
    volatile ULONGLONG x = 0;
    (void)arg;
    while (!stop) x++;
    return 0;
}

int main(void)
{
    HANDLE worker;
    ULONGLONG worker_ms, main_ms;

    setvbuf(stdout, NULL, _IONBF, 0);
    worker = CreateThread(NULL, 0, spin, NULL, 0, NULL);
    CHECK(worker != NULL, "create worker");
    if (!worker) return 1;
    Sleep(800);
    worker_ms = cpu_ms(worker);
    main_ms = cpu_ms(GetCurrentThread());
    stop = 1;
    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);
    printf("worker=%llu ms main=%llu ms\n", worker_ms, main_ms);
    CHECK(worker_ms != ~0ull && main_ms != ~0ull, "GetThreadTimes succeeds");
    CHECK(worker_ms >= 500, "a spinning thread has its own CPU time");
    CHECK(main_ms < 300, "a sleeping thread does not get the process's CPU time");
    printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
    return failures ? 1 : 0;
}
