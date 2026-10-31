/* LGPL-2.1-or-later. Registry save stall probe for 0151.
 * wineserver writes dirty registry branches to disk every 30 seconds, and
 * used to do it inside its request loop, so every client request waited for
 * the write. The probe grows HKCU by about 10 MB, keeps it dirty with one
 * value write per second, and times a server round trip (SetEvent without
 * msync) for 75 seconds, so at least two periodic saves fall inside.
 * Prints the slowest round trip and how many took over 20 ms, then deletes
 * its key. Run without WINEMSYNC (with it, SetEvent never reaches the server).
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror -o registry-save-stall-probe64.exe registry-save-stall-probe.c
 * It measures one client's view; it does not cover the flush at wineserver
 * exit, or save failures. */
#include <windows.h>
#include <stdio.h>

#define VALUES 20000
#define SECONDS 75

static double now_ms(LARGE_INTEGER freq)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)freq.QuadPart;
}

int main(void)
{
    static BYTE data[512];
    LARGE_INTEGER freq;
    HKEY key;
    HANDLE event;
    char name[32];
    double start, last_dirty, worst = 0;
    DWORD i, slow = 0, calls = 0;

    QueryPerformanceFrequency(&freq);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\RegistrySaveStallProbe", 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &key, NULL))
        return 2;
    memset(data, 'r', sizeof(data));
    for (i = 0; i < VALUES; i++)
    {
        snprintf(name, sizeof(name), "v%lu", i);
        RegSetValueExA(key, name, 0, REG_BINARY, data, sizeof(data));
    }

    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    start = last_dirty = now_ms(freq);
    while (now_ms(freq) - start < SECONDS * 1000.0)
    {
        double t0 = now_ms(freq), dt;

        SetEvent(event);
        dt = now_ms(freq) - t0;
        calls++;
        if (dt > worst) worst = dt;
        if (dt > 20.0)
        {
            slow++;
            printf("stall %.1f ms at %.1f s\n", dt, (t0 - start) / 1000.0);
        }
        if (t0 - last_dirty > 1000.0)
        {
            RegSetValueExA(key, "dirty", 0, REG_BINARY, data, 4);
            last_dirty = t0;
        }
    }

    RegCloseKey(key);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\RegistrySaveStallProbe");
    printf("calls: %lu, worst: %.1f ms, over 20 ms: %lu\n", calls, worst, slow);
    return slow ? 1 : 0;
}
