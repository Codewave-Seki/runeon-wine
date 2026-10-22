/*
 * Check that GetDeviceCaps(HORZRES/VERTRES) on the screen DC matches
 * GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN) for the process's DPI awareness.
 *
 * Covers patch 0134. With RetinaMode enabled, winemac used to double both
 * caps on top of a fallback that already reports pixels, so the caps were
 * twice the screen metrics. Run it once per awareness mode, with RetinaMode
 * on and off; it only reads display state and changes nothing.
 *
 * It does not cover printer or memory DCs, per-monitor DCs on a second
 * display, or what a program does with the values.
 *
 * Build: x86_64-w64-mingw32-gcc -O1 -o retina-horzres-probe.exe
 *        retina-horzres-probe.c -luser32 -lgdi32   (i686-w64-mingw32-gcc for x86)
 * Usage: wine retina-horzres-probe.exe [unaware|system|permonitor]
 * Exit:  0 when the caps match the metrics, 1 when they differ, 2 on setup failure.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef BOOL (WINAPI *set_context_fn)(DPI_AWARENESS_CONTEXT);

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "unaware";
    set_context_fn set_context = (set_context_fn)GetProcAddress(GetModuleHandleA("user32.dll"),
                                                                "SetProcessDpiAwarenessContext");
    DPI_AWARENESS_CONTEXT context = NULL;
    int horz, vert, cx, cy, desk_horz, desk_vert;
    HDC hdc;

    if (!strcmp(mode, "system")) context = DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
    else if (!strcmp(mode, "permonitor")) context = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
    else if (strcmp(mode, "unaware"))
    {
        fprintf(stderr, "unknown mode %s\n", mode);
        return 2;
    }
    if (context && (!set_context || !set_context(context)))
    {
        fprintf(stderr, "SetProcessDpiAwarenessContext(%s) failed\n", mode);
        return 2;
    }

    if (!(hdc = GetDC(NULL)))
    {
        fprintf(stderr, "GetDC(NULL) failed\n");
        return 2;
    }
    horz = GetDeviceCaps(hdc, HORZRES);
    vert = GetDeviceCaps(hdc, VERTRES);
    desk_horz = GetDeviceCaps(hdc, DESKTOPHORZRES);
    desk_vert = GetDeviceCaps(hdc, DESKTOPVERTRES);
    ReleaseDC(NULL, hdc);
    cx = GetSystemMetrics(SM_CXSCREEN);
    cy = GetSystemMetrics(SM_CYSCREEN);

    printf("mode=%s HORZRES=%d VERTRES=%d SM_CXSCREEN=%d SM_CYSCREEN=%d DESKTOPHORZRES=%d DESKTOPVERTRES=%d\n",
           mode, horz, vert, cx, cy, desk_horz, desk_vert);
    if (horz != cx || vert != cy)
    {
        printf("FAIL: HORZRES/VERTRES differ from SM_CXSCREEN/SM_CYSCREEN\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
