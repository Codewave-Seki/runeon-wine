/* LGPL-2.1-or-later. Checks distant relative timer due times (0209).
 * Windows takes any negative due time as relative, so -MAXLONGLONG means
 * about 29,000 years. A waitable timer set to -MAXLONGLONG, to
 * QueryPerformanceCounter() minus MAXLONGLONG (what the threadpool timer
 * thread passes when no relative threadpool timer is queued) or to -2^62 must
 * stay unsignalled for 100 ms; the unpatched server signals the first two at
 * once. A 10 ms relative timer must still fire. Does not cover absolute or
 * periodic timers, or the threadpool timer thread itself. */
#include <windows.h>
#include <stdio.h>

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

static DWORD wait_after_set(HANDLE timer, LONGLONG due, DWORD ms)
{
    LARGE_INTEGER when;
    DWORD ret;

    when.QuadPart = due;
    if (!SetWaitableTimer(timer, &when, 0, NULL, NULL, FALSE)) return WAIT_FAILED;
    ret = WaitForSingleObject(timer, ms);
    CancelWaitableTimer(timer);
    return ret;
}

int main(void)
{
    HANDLE timer;
    LARGE_INTEGER qpc;
    DWORD ret;

    setvbuf(stdout, NULL, _IONBF, 0);
    timer = CreateWaitableTimerW(NULL, FALSE, NULL);
    CHECK(timer != NULL, "create timer");
    if (!timer) return 1;
    QueryPerformanceCounter(&qpc);

    ret = wait_after_set(timer, -MAXLONGLONG, 100);
    printf("-MAXLONGLONG: %lu\n", ret);
    CHECK(ret == WAIT_TIMEOUT, "-MAXLONGLONG does not fire");
    ret = wait_after_set(timer, qpc.QuadPart - MAXLONGLONG, 100);
    printf("QPC - MAXLONGLONG: %lu\n", ret);
    CHECK(ret == WAIT_TIMEOUT, "QPC - MAXLONGLONG does not fire");
    ret = wait_after_set(timer, -((LONGLONG)1 << 62), 100);
    printf("-2^62: %lu\n", ret);
    CHECK(ret == WAIT_TIMEOUT, "-2^62 does not fire");
    ret = wait_after_set(timer, -100000, 1000);
    printf("-10 ms: %lu\n", ret);
    CHECK(ret == WAIT_OBJECT_0, "a 10 ms relative timer fires");

    CloseHandle(timer);
    printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
    return failures ? 1 : 0;
}
