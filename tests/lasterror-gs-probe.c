/* LGPL-2.1-or-later. Compares GetLastError(), TEB.LastErrorValue and a raw
 * 64-bit gs:[0x68] read, which is how Mono x64 JIT code fetches the last error
 * inline. On Windows x64 %gs points to the TEB, so all three agree; on macOS
 * gs:[0x68] is a host TSD slot that only agrees if Wine mirrors the error.
 *
 * Covers, 1000 times each on the main thread and a second thread: explicit
 * SetLastError values (0 and large ones), RestoreLastError,
 * RtlSetLastWin32ErrorAndNtStatusFromNtStatus, three failing file APIs, a
 * failing user32 call whose error is set on the Unix side, and
 * GetTimeZoneInformation, which calls host gmtime(), with an error code in the
 * slot. Then: 16 threads setting distinct values at once; SetLastError and
 * failing calls inside a window procedure reached by SendMessage; threads
 * ending by return, ExitThread and TerminateThread; and a process exit with an
 * error code in the main thread's slot. Exits 0 only if every sample agrees
 * and every thread ended as expected. Not covered: i386 (fs:[0x34]) and
 * threads created by host libraries.
 *
 * x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter
 *   -O2 tests/lasterror-gs-probe.c -o lasterror-gs-probe.exe -luser32 */
#include <windows.h>
#include <stdio.h>

#define LOOPS 1000

static LONG failures;

static ULONG64 read_gs68(void)
{
    ULONG64 value;
    __asm__ volatile("movq %%gs:0x68, %0" : "=r"(value));
    return value;
}

static DWORD teb_last_error(void)
{
    /* NtCurrentTeb() reads gs:0x30 (TEB.Self); LastErrorValue is at 0x68 on x64. */
    return *(volatile DWORD *)((BYTE *)NtCurrentTeb() + 0x68);
}

struct sample { ULONG64 gs; DWORD teb, api; };

static struct sample take(void)
{
    struct sample s;
    /* gs first: nothing between the failing call and this read touches the TEB. */
    s.gs = read_gs68();
    s.teb = teb_last_error();
    s.api = GetLastError();
    return s;
}

static BOOL agrees(struct sample s)
{
    /* The whole 64-bit slot must hold the zero-extended error. */
    return s.gs == s.api && s.teb == s.api;
}

static struct sample fail_attributes(void)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    GetFileAttributesExW(L"C:\\windows\\t1078-missing.file", GetFileExInfoStandard, &data);
    return take();
}

static struct sample fail_find_first(void)
{
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileExW(L"C:\\windows\\t1078-missing.file", FindExInfoBasic, &data,
                                   FindExSearchNameMatch, NULL, 0);
    struct sample s = take();
    if (find != INVALID_HANDLE_VALUE) FindClose(find);
    return s;
}

static struct sample fail_find_next(void)
{
    WIN32_FIND_DATAW data;
    struct sample s = {0, 0, 0xffffffff};
    HANDLE find = FindFirstFileW(L"C:\\windows\\*", &data);
    if (find == INVALID_HANDLE_VALUE) return s;
    while (FindNextFileW(find, &data)) {}
    s = take();
    FindClose(find);
    return s;
}

static struct sample fail_window_long(void)
{
    /* win32u sets ERROR_INVALID_WINDOW_HANDLE on the Unix side. */
    GetWindowLongW((HWND)(ULONG_PTR)0xdead0, GWL_STYLE);
    return take();
}

static struct sample set_values(void)
{
    static const DWORD values[] = {0, 0x1234, 2, 0xdeadbeef, 0xffffffff, 0x80000000};
    static LONG next;
    SetLastError(values[(InterlockedIncrement(&next) & 0x7fffffff) % ARRAYSIZE(values)]);
    return take();
}

static void (WINAPI *pRestoreLastError)(DWORD);

static struct sample restore_error(void)
{
    pRestoreLastError(0x4567);
    return take();
}

static void (WINAPI *pRtlSetLastWin32ErrorAndNtStatusFromNtStatus)(LONG);

static struct sample set_from_status(void)
{
    pRtlSetLastWin32ErrorAndNtStatusFromNtStatus(0xc0000034 /* STATUS_OBJECT_NAME_NOT_FOUND */);
    return take();
}

static struct sample time_zone(void)
{
    TIME_ZONE_INFORMATION tzi;
    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    /* An error code, not a buffer pointer, is in the gmtime() slot when ntdll calls gmtime(). */
    SetLastError(2);
    GetTimeZoneInformation(&tzi);
    GetDynamicTimeZoneInformation(&dtzi);
    return take();
}

static void print_sample(const char *thread, const char *label, struct sample s)
{
    if (!agrees(s)) InterlockedIncrement(&failures);
    printf("%-6s %-26s gs=%p teb=0x%08lx api=0x%08lx %s\n", thread, label,
           (void *)(ULONG_PTR)s.gs, s.teb, s.api, agrees(s) ? "match" : "MISMATCH");
}

static void run_loop(const char *thread, const char *label, struct sample (*fn)(void))
{
    /* ponytail: remember only the first few distinct "other" values. */
    ULONG64 others[4] = {0};
    unsigned int match = 0, zero = 0, other = 0, teb_mismatch = 0, n_others = 0, i, j;
    for (i = 0; i < LOOPS; ++i)
    {
        struct sample s = fn();
        if (s.teb != s.api) ++teb_mismatch;
        if (s.gs == s.api) ++match;
        else if (!s.gs) ++zero;
        else
        {
            ++other;
            for (j = 0; j < n_others && others[j] != s.gs; ++j) {}
            if (j == n_others && n_others < ARRAYSIZE(others)) others[n_others++] = s.gs;
        }
    }
    if (match != LOOPS || teb_mismatch) InterlockedIncrement(&failures);
    printf("%-6s %-26s loops=%u gs==api=%u gs==0=%u gs_other=%u teb!=api=%u others:",
           thread, label, LOOPS, match, zero, other, teb_mismatch);
    for (j = 0; j < n_others; ++j) printf(" %p", (void *)(ULONG_PTR)others[j]);
    printf("\n");
}

static DWORD WINAPI run_cases(void *arg)
{
    const char *thread = arg;
    unsigned int i;
    for (i = 0; i < 6; ++i) print_sample(thread, "SetLastError", set_values());
    print_sample(thread, "RestoreLastError", restore_error());
    print_sample(thread, "RtlSetLast...FromNtStatus", set_from_status());
    print_sample(thread, "GetFileAttributesExW", fail_attributes());
    print_sample(thread, "FindFirstFileExW", fail_find_first());
    print_sample(thread, "FindNextFileW end", fail_find_next());
    print_sample(thread, "GetWindowLongW bad hwnd", fail_window_long());
    print_sample(thread, "GetTimeZoneInformation", time_zone());
    run_loop(thread, "SetLastError", set_values);
    run_loop(thread, "RestoreLastError", restore_error);
    run_loop(thread, "RtlSetLast...FromNtStatus", set_from_status);
    run_loop(thread, "GetFileAttributesExW", fail_attributes);
    run_loop(thread, "FindFirstFileExW", fail_find_first);
    run_loop(thread, "FindNextFileW end", fail_find_next);
    run_loop(thread, "GetWindowLongW bad hwnd", fail_window_long);
    run_loop(thread, "GetTimeZoneInformation", time_zone);
    return 0;
}

#define CONCURRENT 16
#define CONCURRENT_LOOPS 20000

static HANDLE start_event;
static LONG concurrent_bad[CONCURRENT];
static LONG concurrent_done;

static DWORD WINAPI concurrent_thread(void *arg)
{
    unsigned int index = (ULONG_PTR)arg, i;
    WaitForSingleObject(start_event, INFINITE);
    for (i = 0; i < CONCURRENT_LOOPS; ++i)
    {
        DWORD value = 0x80000000 | (index << 16) | (i & 0xffff);
        struct sample s;
        SetLastError(value);
        if (!(i % 64)) SwitchToThread();
        s = take();
        if (s.gs != value || s.teb != value || s.api != value) ++concurrent_bad[index];
    }
    /* Counted after the loop, so a thread that never ran cannot pass as zero mismatches. */
    InterlockedIncrement(&concurrent_done);
    return 0;
}

static void run_concurrent(void)
{
    HANDLE threads[CONCURRENT];
    unsigned int i, started = 0, bad = 0;
    DWORD wait = WAIT_FAILED;
    BOOL ok;
    start_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    while (start_event && started < CONCURRENT
           && (threads[started] = CreateThread(NULL, 0, concurrent_thread, (void *)(ULONG_PTR)started, 0, NULL)))
        ++started;
    if (start_event && SetEvent(start_event) && started)
        wait = WaitForMultipleObjects(started, threads, TRUE, INFINITE);
    for (i = 0; i < started; ++i)
    {
        bad += concurrent_bad[i];
        CloseHandle(threads[i]);
    }
    if (start_event) CloseHandle(start_event);
    ok = started == CONCURRENT && wait == WAIT_OBJECT_0 && concurrent_done == CONCURRENT && !bad;
    if (!ok) InterlockedIncrement(&failures);
    printf("concurrent threads=%u started=%u done=%ld wait=%lu loops=%u own-value mismatches=%u %s\n",
           CONCURRENT, started, concurrent_done, wait, CONCURRENT_LOOPS, bad, ok ? "match" : "MISMATCH");
}

static struct sample in_proc[3];

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg != WM_APP) return DefWindowProcW(hwnd, msg, wparam, lparam);
    SetLastError(0x1abc);
    in_proc[0] = take();
    in_proc[1] = fail_attributes();
    in_proc[2] = fail_window_long();
    SetLastError(0x2bcd);
    return 0;
}

static void run_window(void)
{
    WNDCLASSW cls = {0};
    struct sample after;
    HWND hwnd;
    cls.lpfnWndProc = window_proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.lpszClassName = L"t1078_lasterror";
    RegisterClassW(&cls);
    hwnd = CreateWindowW(cls.lpszClassName, L"", WS_OVERLAPPED, 0, 0, 10, 10, NULL, NULL, cls.hInstance, NULL);
    if (!hwnd)
    {
        InterlockedIncrement(&failures);
        printf("FAIL: CreateWindowW %lu\n", GetLastError());
        return;
    }
    SetLastError(0x77);
    SendMessageW(hwnd, WM_APP, 0, 0);
    after = take();
    print_sample("window", "SetLastError in proc", in_proc[0]);
    print_sample("window", "GetFileAttributesExW proc", in_proc[1]);
    print_sample("window", "GetWindowLongW in proc", in_proc[2]);
    print_sample("window", "after SendMessageW", after);
    DestroyWindow(hwnd);
}

static DWORD WINAPI exit_by_return(void *arg)
{
    SetLastError(0xdeadbeef);
    return 0;
}

static DWORD WINAPI exit_by_exitthread(void *arg)
{
    SetLastError(0xfeedface);
    ExitThread(0);
}

static DWORD WINAPI exit_by_terminate(void *arg)
{
    unsigned int i = 0;
    for (;;)
    {
        SetLastError(0xcafe0000 | (++i & 0xffff));
        Sleep(1);
    }
    return 0;
}

static void run_exits(void)
{
    static LPTHREAD_START_ROUTINE procs[] = {exit_by_return, exit_by_exitthread, exit_by_terminate};
    static const char *names[] = {"return", "ExitThread", "TerminateThread"};
    unsigned int kind, i;
    for (kind = 0; kind < ARRAYSIZE(procs); ++kind)
    {
        unsigned int ok = 0;
        for (i = 0; i < 50; ++i)
        {
            HANDLE thread = CreateThread(NULL, 0, procs[kind], NULL, 0, NULL);
            DWORD code = 0;
            if (!thread) continue;
            if (kind == 2)
            {
                Sleep(20);
                TerminateThread(thread, 7);
            }
            if (WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0 && GetExitCodeThread(thread, &code)
                && code == (kind == 2 ? 7u : 0u))
                ++ok;
            CloseHandle(thread);
        }
        if (ok != 50) InterlockedIncrement(&failures);
        printf("exit   %-26s threads=50 ended=%u %s\n", names[kind], ok, ok == 50 ? "ok" : "FAIL");
    }
}

int main(void)
{
    ULONG64 gs30;
    HANDLE thread;
    setvbuf(stdout, NULL, _IONBF, 0);
    pRestoreLastError = (void *)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "RestoreLastError");
    pRtlSetLastWin32ErrorAndNtStatusFromNtStatus = (void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),
                                                                         "RtlSetLastWin32ErrorAndNtStatusFromNtStatus");
    __asm__ volatile("movq %%gs:0x30, %0" : "=r"(gs30));
    printf("gs:0x30=%p NtCurrentTeb=%p gs:0x68=%p GetLastError=0x%08lx\n",
           (void *)(ULONG_PTR)gs30, (void *)NtCurrentTeb(), (void *)(ULONG_PTR)read_gs68(), GetLastError());
    run_cases("main");
    thread = CreateThread(NULL, 0, run_cases, "thread", 0, NULL);
    if (!thread || WaitForSingleObject(thread, 120000) != WAIT_OBJECT_0)
    {
        printf("FAIL: second thread\n");
        return 2;
    }
    CloseHandle(thread);
    run_concurrent();
    run_window();
    run_exits();
    printf("process still running; failures=%ld\n", failures);
    /* Leave an error code in the main thread's slot for the process exit. */
    SetLastError(0x1234);
    return failures ? 1 : 0;
}
