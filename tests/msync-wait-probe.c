/* LGPL-2.1-or-later. msync wait probe for 0137-0143: leak and throughput.
 * leak <seconds>: four threads wait for any of a never-signalled event and a
 * semaphore two threads keep releasing. A wait that gives up during
 * registration (0138) or whose second object is already signalled (0137)
 * used to leave a server node on the never-signalled event; once the pool of
 * 0x80000 nodes runs out, wineserver prints "msync: warn: node memory pool
 * exhausted" on every registration and its memory grows. Read the wineserver
 * stderr and RSS; the probe itself only prints wait counts.
 * ring <handoffs>: eight threads pass an auto-reset event around.
 * multi <wakeups>: eight threads wait for any of two events, woken one by one.
 * ring and multi print elapsed milliseconds for comparison between runtimes.
 * Run every mode under one wineserver with WINEMSYNC=1 from its first command.
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror -o msync-wait-probe64.exe msync-wait-probe.c
 *   i686-w64-mingw32-gcc -O2 -Wall -Werror -o msync-wait-probe32.exe msync-wait-probe.c
 * It does not separate the effect of each patch, and does not cover mutex
 * abandonment, wait-all or alertable waits. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 8

static HANDLE never_event, busy_sem, stop_event;
static volatile LONG stop_flag;
static volatile LONG64 wait_count;

static DWORD WINAPI leak_waiter(void *arg)
{
    HANDLE objs[2] = { never_event, busy_sem };
    (void)arg;
    while (!stop_flag)
    {
        WaitForMultipleObjects(2, objs, FALSE, 50);
        InterlockedIncrement64(&wait_count);
    }
    return 0;
}

static DWORD WINAPI leak_producer(void *arg)
{
    (void)arg;
    while (!stop_flag)
    {
        ReleaseSemaphore(busy_sem, 1, NULL);
        SwitchToThread();
    }
    return 0;
}

static int run_leak(int seconds)
{
    HANDLE threads[6];
    int i;
    never_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    busy_sem = CreateSemaphoreW(NULL, 0, 0x7fffffff, NULL);
    for (i = 0; i < 4; i++) threads[i] = CreateThread(NULL, 0, leak_waiter, NULL, 0, NULL);
    for (; i < 6; i++) threads[i] = CreateThread(NULL, 0, leak_producer, NULL, 0, NULL);
    for (i = 0; i < seconds; i++)
    {
        Sleep(1000);
        printf("t=%d waits=%lld\n", i + 1, (long long)wait_count);
    }
    stop_flag = 1;
    WaitForMultipleObjects(6, threads, TRUE, INFINITE);
    printf("leak seconds=%d waits=%lld\n", seconds, (long long)wait_count);
    return 0;
}

static HANDLE ring_events[THREADS];
static LONG ring_total, ring_target;
static volatile LONG ring_done;

static DWORD WINAPI ring_thread(void *arg)
{
    int me = (int)(INT_PTR)arg;
    while (1)
    {
        WaitForSingleObject(ring_events[me], INFINITE);
        if (ring_done) { SetEvent(ring_events[(me + 1) % THREADS]); return 0; }
        if (InterlockedIncrement(&ring_total) >= ring_target) ring_done = 1;
        SetEvent(ring_events[(me + 1) % THREADS]);
    }
}

static int run_ring(int handoffs)
{
    HANDLE threads[THREADS];
    LARGE_INTEGER f, a, b;
    int i;
    ring_target = handoffs;
    for (i = 0; i < THREADS; i++) ring_events[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
    for (i = 0; i < THREADS; i++) threads[i] = CreateThread(NULL, 0, ring_thread, (void *)(INT_PTR)i, 0, NULL);
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&a);
    SetEvent(ring_events[0]);
    WaitForMultipleObjects(THREADS, threads, TRUE, INFINITE);
    QueryPerformanceCounter(&b);
    printf("ring handoffs=%d ms=%.1f\n", handoffs, (b.QuadPart - a.QuadPart) * 1000.0 / f.QuadPart);
    return 0;
}

static HANDLE multi_events[THREADS], multi_ack;

static DWORD WINAPI multi_thread(void *arg)
{
    HANDLE objs[2] = { stop_event, multi_events[(int)(INT_PTR)arg] };
    while (WaitForMultipleObjects(2, objs, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
        SetEvent(multi_ack);
    return 0;
}

static int run_multi(int wakeups)
{
    HANDLE threads[THREADS];
    LARGE_INTEGER f, a, b;
    int i;
    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    multi_ack = CreateEventW(NULL, FALSE, FALSE, NULL);
    for (i = 0; i < THREADS; i++) multi_events[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
    for (i = 0; i < THREADS; i++) threads[i] = CreateThread(NULL, 0, multi_thread, (void *)(INT_PTR)i, 0, NULL);
    Sleep(100);
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&a);
    for (i = 0; i < wakeups; i++)
    {
        SetEvent(multi_events[i % THREADS]);
        WaitForSingleObject(multi_ack, INFINITE);
    }
    QueryPerformanceCounter(&b);
    SetEvent(stop_event);
    WaitForMultipleObjects(THREADS, threads, TRUE, INFINITE);
    printf("multi wakeups=%d ms=%.1f\n", wakeups, (b.QuadPart - a.QuadPart) * 1000.0 / f.QuadPart);
    return 0;
}

int main(int argc, char **argv)
{
    int n;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 3 || (n = atoi(argv[2])) <= 0)
    {
        fprintf(stderr, "usage: msync-probe leak|ring|multi <count>\n");
        return 2;
    }
    if (!strcmp(argv[1], "leak")) return run_leak(n);
    if (!strcmp(argv[1], "ring")) return run_ring(n);
    if (!strcmp(argv[1], "multi")) return run_multi(n);
    fprintf(stderr, "unknown mode %s\n", argv[1]);
    return 2;
}
