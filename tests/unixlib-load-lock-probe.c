/* LGPL-2.1-or-later. Unix library load lock probe for 0152.
 * The first call into a builtin DLL's Unix library dlopen()s it, and ntdll
 * used to do that while holding virtual_mutex, which every VirtualAlloc,
 * VirtualProtect and page fault in the process also takes. The probe loads
 * builtin DLLs that have Unix libraries while a second thread times
 * VirtualAlloc/VirtualFree pairs, and prints each load's duration and the
 * slowest pair while idle and during the loads. A slowest pair close to the
 * slowest load means the loads held the lock.
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror -o unixlib-load-lock-probe64.exe unixlib-load-lock-probe.c
 * It does not cover libraries loaded by the Unix side itself, or 32-bit
 * processes. */
#include <windows.h>
#include <stdio.h>

static volatile LONG stop;
static double worst_ms;
static LARGE_INTEGER freq;

static double now_ms(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)freq.QuadPart;
}

static DWORD WINAPI alloc_loop(void *arg)
{
    (void)arg;
    while (!stop)
    {
        double t0 = now_ms(), dt;
        void *p = VirtualAlloc(NULL, 65536, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        VirtualFree(p, 0, MEM_RELEASE);
        dt = now_ms() - t0;
        if (dt > worst_ms) worst_ms = dt;
    }
    return 0;
}

int main(void)
{
    static const char *dlls[] = { "bcrypt.dll", "secur32.dll", "dnsapi.dll", "netapi32.dll", "winspool.drv" };
    double slowest_load = 0, idle_worst;
    HANDLE thread;
    unsigned int i;

    QueryPerformanceFrequency(&freq);
    thread = CreateThread(NULL, 0, alloc_loop, NULL, 0, NULL);
    Sleep(200);
    worst_ms = 0;
    Sleep(1000);
    idle_worst = worst_ms;
    worst_ms = 0;
    for (i = 0; i < ARRAYSIZE(dlls); i++)
    {
        double t0 = now_ms(), dt;
        HMODULE module = LoadLibraryA(dlls[i]);
        dt = now_ms() - t0;
        printf("%s: %s in %.1f ms\n", dlls[i], module ? "loaded" : "failed", dt);
        if (dt > slowest_load) slowest_load = dt;
    }
    stop = 1;
    WaitForSingleObject(thread, INFINITE);
    printf("slowest load: %.1f ms, slowest VirtualAlloc/VirtualFree idle: %.1f ms, during loads: %.1f ms\n",
           slowest_load, idle_worst, worst_ms);
    return 0;
}
