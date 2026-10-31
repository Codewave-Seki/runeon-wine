/* LGPL-2.1-or-later. Activation click probe for 0153.
 * macOS is reported to re-announce an already active application in
 * full-screen games, and winemac.drv answered every activation by re-reading
 * the display list on the thread that owns the desktop. The probe opens a window for the given
 * number of seconds and counts left clicks in it. Click inside the window
 * while it runs, without switching to another application. Run with
 * WINEDEBUG=+event and count "DISPLAYS_CHANGED" lines against the clicks.
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror -mwindows -o activation-click-probe64.exe activation-click-probe.c
 * Usage: activation-click-probe64.exe [seconds]   (default 30)
 * It does not cover full-screen windows or clicks after another application
 * was active. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static LONG clicks;

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_LBUTTONDOWN) clicks++;
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int show)
{
    WNDCLASSA cls = { 0 };
    HWND hwnd;
    MSG msg;
    DWORD end;
    int seconds = atoi(cmdline) > 0 ? atoi(cmdline) : 30;
    char text[64];

    cls.lpfnWndProc = wndproc;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszClassName = "activation_click_probe";
    RegisterClassA(&cls);
    hwnd = CreateWindowA(cls.lpszClassName, "Click here", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         100, 100, 640, 480, NULL, NULL, instance, NULL);
    end = GetTickCount() + seconds * 1000;
    while (GetTickCount() < end)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        snprintf(text, sizeof(text), "Click here: %ld clicks, %lu s left", clicks,
                 (end - GetTickCount()) / 1000);
        SetWindowTextA(hwnd, text);
        MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT);
    }
    DestroyWindow(hwnd);
    fprintf(stderr, "clicks: %ld\n", clicks);
    return 0;
}
