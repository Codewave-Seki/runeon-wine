/*
 * Cross-process child window surface flush under the USER lock.
 *
 * A window whose parent belongs to another process gets CrossOver's shared-memory
 * surface. Reparenting a window into it makes set_window_pos() flush that surface while
 * the USER lock is held; unpatched, the flush sends a message and the process aborts in
 * user_check_not_lock(). With the fix the flush is deferred and the message loop sends it.
 *
 * The parent process starts the child and samples the pixel under the child window.
 * The child paints red and lets it flush, then paints blue and reparents a window into
 * itself before pumping messages again, and never paints after that. Blue reaching the
 * parent therefore proves the deferred flush was delivered without new drawing.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -o shm-surface-lock-probe.exe shm-surface-lock-probe.c -lgdi32 -luser32
 *
 * Exit codes: 0 blue delivered, 2 child died, 3 child alive but blue never arrived,
 * 4 setup failure.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define RED  RGB(255, 0, 0)
#define BLUE RGB(0, 0, 255)

static LRESULT CALLBACK child_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if (msg == WM_PAINT)
    {
        /* Validate without drawing: any later colour change must come from the deferred flush. */
        PAINTSTRUCT ps;
        BeginPaint( hwnd, &ps );
        EndPaint( hwnd, &ps );
        return 0;
    }
    if (msg == WM_ERASEBKGND) return 1;
    return DefWindowProcA( hwnd, msg, wp, lp );
}

static void pump( DWORD ms )
{
    DWORD end = GetTickCount() + ms;
    MSG msg;
    while ((int)(end - GetTickCount()) > 0)
    {
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageA( &msg );
        }
        MsgWaitForMultipleObjects( 0, NULL, FALSE, 20, QS_ALLINPUT );
    }
}

static void fill( HWND hwnd, COLORREF color )
{
    HDC hdc = GetDC( hwnd );
    HBRUSH brush = CreateSolidBrush( color );
    RECT rc;
    GetClientRect( hwnd, &rc );
    FillRect( hdc, &rc, brush );
    DeleteObject( brush );
    ReleaseDC( hwnd, hdc );
}

static int run_child( HWND parent )
{
    WNDCLASSA cls = { 0 };
    HWND host, inner;

    cls.lpfnWndProc = child_proc;
    cls.hInstance = GetModuleHandleA( NULL );
    cls.lpszClassName = "shm_lock_child";
    RegisterClassA( &cls );

    host = CreateWindowExA( 0, cls.lpszClassName, NULL, WS_CHILD | WS_VISIBLE,
                            10, 10, 120, 120, parent, 0, cls.hInstance, NULL );
    inner = CreateWindowExA( 0, cls.lpszClassName, NULL, WS_POPUP | WS_VISIBLE,
                             0, 0, 40, 40, 0, 0, cls.hInstance, NULL );
    if (!host || !inner) return 4;

    fill( host, RED );
    pump( 1500 );

    /* No message pumping between the draw and the reparent: the blue bits are still dirty
     * when set_window_pos() flushes the shared surface with the USER lock held. */
    fill( host, BLUE );
    SetParent( inner, host );

    pump( 6000 );
    DestroyWindow( inner );
    DestroyWindow( host );
    return 0;
}

int main( int argc, char **argv )
{
    WNDCLASSA cls = { 0 };
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    char cmd[MAX_PATH * 2], self[MAX_PATH];
    BOOL saw_red = FALSE;
    DWORD code = STILL_ACTIVE, start;
    HWND parent;

    si.cb = sizeof(si);
    if (argc == 3 && !strcmp( argv[1], "child" ))
        return run_child( (HWND)(ULONG_PTR)strtoull( argv[2], NULL, 16 ) );

    cls.lpfnWndProc = DefWindowProcA;
    cls.hInstance = GetModuleHandleA( NULL );
    cls.hbrBackground = GetStockObject( WHITE_BRUSH );
    cls.lpszClassName = "shm_lock_parent";
    RegisterClassA( &cls );
    parent = CreateWindowExA( 0, cls.lpszClassName, "shm lock probe", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 300, 300, 0, 0, cls.hInstance, NULL );
    if (!parent) return 4;
    pump( 500 );

    GetModuleFileNameA( NULL, self, sizeof(self) );
    snprintf( cmd, sizeof(cmd), "\"%s\" child %p", self, parent );
    if (!CreateProcessA( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi )) return 4;

    start = GetTickCount();
    while (GetTickCount() - start < 12000)
    {
        HDC hdc;
        COLORREF px;

        pump( 100 );
        hdc = GetDC( parent );
        px = GetPixel( hdc, 70, 70 );
        ReleaseDC( parent, hdc );
        if (px == RED && !saw_red) { saw_red = TRUE; printf( "red seen\n" ); }
        if (px == BLUE)
        {
            DWORD wait_start = GetTickCount();
            printf( "blue seen after %lu ms\n", GetTickCount() - start );
            /* Keep pumping: destroying the child's cross-process window needs this thread. */
            while (GetTickCount() - wait_start < 15000 &&
                   MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, 100, QS_ALLINPUT ) != WAIT_OBJECT_0)
                pump( 50 );
            GetExitCodeProcess( pi.hProcess, &code );
            printf( "child exit %lu\n", code );
            return code == 0 ? 0 : 2;
        }
        GetExitCodeProcess( pi.hProcess, &code );
        if (code != STILL_ACTIVE)
        {
            printf( "child exited with %#lx before blue arrived (red seen: %d)\n", code, saw_red );
            return code == 0 ? 3 : 2;
        }
    }
    printf( "timeout, red seen: %d\n", saw_red );
    TerminateProcess( pi.hProcess, 1 );
    return 3;
}
