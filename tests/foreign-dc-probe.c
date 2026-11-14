/*
 * Drawing through a DC of a window whose top-level window belongs to another process.
 *
 * Three processes, as with a Chromium GPU process drawing into a WebView2 browser window
 * hosted inside an application window:
 *   root     owns the top-level window and samples its pixels;
 *   browser  owns a child window parented to the root window and paints it orange;
 *   painter  calls GetDC() on the child window, draws blue and calls ReleaseDC().
 * Unpatched, the painter's DC has no window surface and its output never reaches the root
 * window. Patch 0179 delivers it with RUNEON_FOREIGN_WINDOW_SURFACE=1 in every process.
 *
 * Modes:
 *   fill      the painter fills the whole child client area; every sample must turn blue.
 *   preserve  the painter fills a 3-pixel wide rectangle at x=10..12 and two rectangles at
 *             x=41..43 and x=53..55 in one DC session. The drawn pixels must turn blue, and
 *             the neighbours (x=8, 9, 13..15, the rows above and below) and the gap between
 *             the two rectangles (x=44..52) must keep the browser's orange. The write-back is
 *             widened to 8-pixel alignment, but what it carries there is the seeded copy of the
 *             window, so no white and no stale pixels may appear.
 *   crossframe one painter process, two DC sessions. Session 1 fills (3,5)-(13,11) blue; the
 *             browser then repaints its whole window magenta; session 2 fills (50,30)-(61,37)
 *             blue. Afterwards only the second rectangle may be blue: the first rectangle, the
 *             8-pixel aligned neighbours of both and the rest must stay magenta, so neither the
 *             old blue nor the old orange may be written back from a stale copy.
 *   painteronly  fill with the opt-in in the painter only: run without it, and the root sets
 *             it for the painter alone. The root, which owns the top window, does not seed, so
 *             the result must be the unpatched one (exit 1).
 *   bench     a 1280x720 child; the painter times 120 GetDC -> BitBlt (full client) ->
 *             ReleaseDC iterations and prints the average. No pixel verdict (exit 0 or 4).
 *
 * Phases are signalled with named events; processes that own windows pump messages while
 * they wait. In crossframe the magenta repaint is the owner control and must also reach the
 * root window before session 2 (a miss there counts as exit 1). Otherwise, after the verdict, the browser repaints its own window magenta as a positive
 * control of the cross-process child surface path. The control is reported separately and
 * never turns a failed verdict into a pass.
 *
 * Covers only GDI FillRect() through GetDC() on a client area; window DCs, BitBlt() sources
 * and concurrent drawing by other processes are not covered.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -o foreign-dc-probe.exe foreign-dc-probe.c -lgdi32 -luser32
 * Run:   wine foreign-dc-probe.exe fill|preserve|crossframe|painteronly|bench
 *
 * Exit codes: 0 pass, 1 foreign drawing missing or wrong (the defect),
 * 2 verdict passed but the owner control failed, 4 setup failure.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORANGE  RGB(255, 128, 0)
#define BLUE    RGB(0, 0, 255)
#define MAGENTA RGB(255, 0, 255)

#define CHILD_X 20
#define CHILD_Y 20
#define BENCH_W 1280
#define BENCH_H 720
#define BENCH_N 120

/* ponytail: globals, set from the mode in every process; only bench changes them */
static int CHILD_W = 120, CHILD_H = 80;

#define CHILD_CLASS "foreign_dc_probe_child"

enum { EV_READY, EV_PAINTED, EV_CONTROL, EV_CONTROL_DONE, EV_QUIT, EV_PAINT2, EV_PAINTED2, EV_COUNT };
static const char *event_names[EV_COUNT] = { "ready", "painted", "control", "control_done", "quit",
                                             "paint2", "painted2" };
static HANDLE events[EV_COUNT];

struct sample
{
    int x, y;          /* child client coordinates */
    COLORREF expect;
};

static const struct sample fill_samples[] =
{
    { 0, 0, BLUE }, { 60, 40, BLUE }, { 119, 0, BLUE }, { 0, 79, BLUE }, { 119, 79, BLUE },
};

static const RECT preserve_rects[] = { { 10, 10, 13, 30 }, { 41, 10, 44, 30 }, { 53, 10, 56, 30 } };

static const struct sample preserve_samples[] =
{
    /* drawn */
    { 10, 20, BLUE }, { 11, 20, BLUE }, { 12, 20, BLUE },
    { 41, 20, BLUE }, { 43, 20, BLUE }, { 53, 20, BLUE }, { 55, 20, BLUE },
    /* neighbours inside the 8-pixel aligned span of the first rectangle */
    { 8, 20, ORANGE }, { 9, 20, ORANGE }, { 13, 20, ORANGE }, { 14, 20, ORANGE }, { 15, 20, ORANGE },
    { 11, 9, ORANGE }, { 11, 30, ORANGE },
    /* gap between the two separated rectangles */
    { 44, 20, ORANGE }, { 48, 20, ORANGE }, { 52, 20, ORANGE }, { 48, 15, ORANGE },
    /* between the first and second rectangle, and far away */
    { 30, 20, ORANGE }, { 0, 0, ORANGE }, { 119, 79, ORANGE },
};

static const RECT cross_rect1 = { 3, 5, 13, 11 }, cross_rect2 = { 50, 30, 61, 37 };

static const struct sample cross_stage1_samples[] =
{
    { 3, 5, BLUE }, { 12, 10, BLUE }, { 2, 8, ORANGE }, { 13, 8, ORANGE }, { 8, 4, ORANGE },
};

static const struct sample cross_repaint_samples[] =
{
    { 3, 5, MAGENTA }, { 8, 8, MAGENTA }, { 12, 10, MAGENTA }, { 0, 0, MAGENTA },
    { 55, 33, MAGENTA }, { 119, 79, MAGENTA },
};

static const struct sample cross_samples[] =
{
    /* second rectangle */
    { 50, 30, BLUE }, { 60, 36, BLUE }, { 55, 33, BLUE },
    /* its 8-pixel aligned neighbours: the old orange must not come back */
    { 48, 33, MAGENTA }, { 49, 33, MAGENTA }, { 61, 33, MAGENTA }, { 63, 33, MAGENTA },
    { 55, 29, MAGENTA }, { 55, 37, MAGENTA }, { 55, 24, MAGENTA }, { 55, 39, MAGENTA },
    /* first rectangle and its aligned span: the old blue must not come back */
    { 3, 5, MAGENTA }, { 8, 8, MAGENTA }, { 12, 10, MAGENTA }, { 0, 0, MAGENTA }, { 15, 15, MAGENTA },
    /* far away */
    { 30, 60, MAGENTA }, { 119, 79, MAGENTA },
};

static BOOL open_events( DWORD root_pid, BOOL create )
{
    char name[64];
    int i;

    for (i = 0; i < EV_COUNT; i++)
    {
        snprintf( name, sizeof(name), "foreign_dc_probe_%lu_%s", root_pid, event_names[i] );
        events[i] = create ? CreateEventA( NULL, TRUE, FALSE, name )
                           : OpenEventA( EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, name );
        if (!events[i]) return FALSE;
    }
    return TRUE;
}

static void pump_pending(void)
{
    MSG msg;

    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
}

/* Wait for one of the handles while pumping messages; returns its index, or -1 on timeout. */
static int wait_pumping( DWORD count, const HANDLE *handles, DWORD ms )
{
    DWORD end = GetTickCount() + ms, ret;

    for (;;)
    {
        int remaining = (int)(end - GetTickCount());

        pump_pending();
        if (remaining <= 0) return -1;
        ret = MsgWaitForMultipleObjects( count, handles, FALSE, remaining, QS_ALLINPUT );
        if (ret < WAIT_OBJECT_0 + count) return ret - WAIT_OBJECT_0;
        if (ret == WAIT_TIMEOUT) return -1;
    }
}

static void fill_rect( HDC hdc, const RECT *rect, COLORREF color )
{
    HBRUSH brush = CreateSolidBrush( color );
    FillRect( hdc, rect, brush );
    DeleteObject( brush );
}

static void fill_window( HWND hwnd, COLORREF color )
{
    HDC hdc = GetDC( hwnd );
    RECT rc;

    GetClientRect( hwnd, &rc );
    fill_rect( hdc, &rc, color );
    ReleaseDC( hwnd, hdc );
}

static COLORREF child_color = ORANGE;

static LRESULT CALLBACK child_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint( hwnd, &ps );
        fill_rect( hdc, &ps.rcPaint, child_color );
        EndPaint( hwnd, &ps );
        return 0;
    }
    if (msg == WM_ERASEBKGND) return 1;
    return DefWindowProcA( hwnd, msg, wp, lp );
}

static int run_browser( HWND root )
{
    WNDCLASSA cls = { 0 };
    HANDLE wait[2];
    HWND child;

    cls.lpfnWndProc = child_proc;
    cls.hInstance = GetModuleHandleA( NULL );
    cls.lpszClassName = CHILD_CLASS;
    if (!RegisterClassA( &cls )) return 4;

    child = CreateWindowExA( 0, CHILD_CLASS, NULL, WS_CHILD | WS_VISIBLE,
                             CHILD_X, CHILD_Y, CHILD_W, CHILD_H, root, 0, cls.hInstance, NULL );
    if (!child) return 4;
    fill_window( child, ORANGE );
    SetEvent( events[EV_READY] );

    wait[0] = events[EV_CONTROL];
    wait[1] = events[EV_QUIT];
    for (;;)
    {
        int ret = wait_pumping( 2, wait, 120000 );

        if (ret == 0)
        {
            ResetEvent( events[EV_CONTROL] );
            child_color = MAGENTA;
            fill_window( child, MAGENTA );
            SetEvent( events[EV_CONTROL_DONE] );
            continue;
        }
        DestroyWindow( child );
        return ret == 1 ? 0 : 4;
    }
}

static int run_painter( const char *mode, HWND root )
{
    HWND child = FindWindowExA( root, 0, CHILD_CLASS, NULL );
    RECT clip = { 0 }, rc;
    int kind;
    HDC hdc;
    unsigned int i;

    if (!child) return 4;
    if (!strcmp( mode, "crossframe" ))
    {
        hdc = GetDC( child );
        if (!hdc) return 4;
        fill_rect( hdc, &cross_rect1, BLUE );
        ReleaseDC( child, hdc );
        SetEvent( events[EV_PAINTED] );
        if (WaitForSingleObject( events[EV_PAINT2], 60000 )) return 4;
        hdc = GetDC( child );
        if (!hdc) return 4;
        fill_rect( hdc, &cross_rect2, BLUE );
        ReleaseDC( child, hdc );
        SetEvent( events[EV_PAINTED2] );
        return 0;
    }
    if (!strcmp( mode, "bench" ))
    {
        HDC mem = CreateCompatibleDC( NULL );
        BITMAPINFO bi = { 0 };
        void *bits;
        HBITMAP bmp;
        LARGE_INTEGER freq, t0, t1;
        double first = 0, total = 0, worst = 0;

        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = BENCH_W;
        bi.bmiHeader.biHeight = -BENCH_H;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bmp = CreateDIBSection( NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0 );

        if (!mem || !bmp) return 4;
        memset( bits, 0x80, BENCH_W * BENCH_H * 4 );
        SelectObject( mem, bmp );
        QueryPerformanceFrequency( &freq );
        for (i = 0; i < BENCH_N; i++)
        {
            double ms;

            QueryPerformanceCounter( &t0 );
            hdc = GetDC( child );
            BitBlt( hdc, 0, 0, BENCH_W, BENCH_H, mem, 0, 0, SRCCOPY );
            ReleaseDC( child, hdc );
            QueryPerformanceCounter( &t1 );
            ms = (t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
            if (!i) first = ms;
            else { total += ms; if (ms > worst) worst = ms; }
        }
        printf( "bench: %dx%d GetDC->BitBlt->ReleaseDC first %.3f ms, avg %.3f ms, max %.3f ms over %d\n",
                BENCH_W, BENCH_H, first, total / (BENCH_N - 1), worst, BENCH_N - 1 );
        fflush( stdout );
        SetEvent( events[EV_PAINTED] );
        return 0;
    }
    hdc = GetDC( child );
    if (!hdc) return 4;
    kind = GetClipBox( hdc, &clip );
    /* Draw everything in one DC session, so that one flush covers all rectangles. */
    if (!strcmp( mode, "preserve" ))
        for (i = 0; i < ARRAYSIZE(preserve_rects); i++) fill_rect( hdc, &preserve_rects[i], BLUE );
    else
    {
        GetClientRect( child, &rc );
        fill_rect( hdc, &rc, BLUE );
    }
    GdiFlush();
    printf( "painter: dc %p clip %d (%ld,%ld)-(%ld,%ld), own pixel at (11,20) %08lx\n", hdc, kind,
            clip.left, clip.top, clip.right, clip.bottom, GetPixel( hdc, 11, 20 ) );
    fflush( stdout );
    ReleaseDC( child, hdc );
    SetEvent( events[EV_PAINTED] );
    return 0;
}

static COLORREF root_pixel( HWND root, int x, int y )
{
    /* No DCX_CLIPCHILDREN: the root window has WS_CLIPCHILDREN, but the samples are under
     * the child window. */
    HDC hdc = GetDCEx( root, 0, DCX_CACHE );
    COLORREF px = GetPixel( hdc, CHILD_X + x, CHILD_Y + y );

    ReleaseDC( root, hdc );
    return px;
}

/* Poll the samples until they all match or the deadline passes; returns the mismatches. */
static unsigned int check_samples( HWND root, const struct sample *samples, unsigned int count,
                                   DWORD ms, BOOL report )
{
    DWORD end = GetTickCount() + ms;
    unsigned int i, bad;

    for (;;)
    {
        for (i = bad = 0; i < count; i++)
            if (root_pixel( root, samples[i].x, samples[i].y ) != samples[i].expect) bad++;
        if (!bad || (int)(end - GetTickCount()) <= 0) break;
        wait_pumping( 0, NULL, 50 );
    }
    if (report)
        for (i = 0; i < count; i++)
        {
            COLORREF px = root_pixel( root, samples[i].x, samples[i].y );
            printf( "  (%3d,%3d) expect %06lx got %06lx%s\n", samples[i].x, samples[i].y,
                    samples[i].expect, px, px == samples[i].expect ? "" : "  MISMATCH" );
        }
    return bad;
}

static BOOL spawn( const char *args, PROCESS_INFORMATION *pi )
{
    STARTUPINFOA si = { 0 };
    char self[MAX_PATH], cmd[MAX_PATH * 2];

    si.cb = sizeof(si);
    GetModuleFileNameA( NULL, self, sizeof(self) );
    snprintf( cmd, sizeof(cmd), "\"%s\" %s", self, args );
    return CreateProcessA( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, pi );
}

int main( int argc, char **argv )
{
    const struct sample orange_samples[] =
    {
        { 0, 0, ORANGE }, { CHILD_W / 2, CHILD_H / 2, ORANGE }, { CHILD_W - 1, CHILD_H - 1, ORANGE },
    };
    const struct sample magenta_samples[] =
    {
        { 0, 0, MAGENTA }, { CHILD_W / 2, CHILD_H / 2, MAGENTA }, { CHILD_W - 1, CHILD_H - 1, MAGENTA },
    };
    PROCESS_INFORMATION browser = { 0 }, painter = { 0 };
    const struct sample *samples;
    unsigned int count, bad;
    const char *mode = "fill";
    char args[128];
    WNDCLASSA cls = { 0 };
    HANDLE wait[2];
    HWND root;
    int ret = 4, control_ok;
    BOOL painter_only = FALSE, started;

    if (argc >= 2 && !strcmp( argv[argc - 1], "bench" )) { CHILD_W = BENCH_W; CHILD_H = BENCH_H; }
    if (argc == 5 && !strcmp( argv[1], "browser" ))
    {
        if (!open_events( strtoul( argv[3], NULL, 10 ), FALSE )) return 4;
        return run_browser( (HWND)(ULONG_PTR)strtoull( argv[2], NULL, 16 ) );
    }
    if (argc == 5 && !strcmp( argv[1], "painter" ))
    {
        if (!open_events( strtoul( argv[4], NULL, 10 ), FALSE )) return 4;
        return run_painter( argv[2], (HWND)(ULONG_PTR)strtoull( argv[3], NULL, 16 ) );
    }
    if (argc == 2) mode = argv[1];
    if (!strcmp( mode, "painteronly" ))
    {
        if (getenv( "RUNEON_FOREIGN_WINDOW_SURFACE" ))
        {
            printf( "setup: run painteronly without RUNEON_FOREIGN_WINDOW_SURFACE\n" );
            return 4;
        }
        painter_only = TRUE;
        mode = "fill";
    }
    if (strcmp( mode, "fill" ) && strcmp( mode, "preserve" ) && strcmp( mode, "crossframe" ) &&
        strcmp( mode, "bench" ))
    {
        printf( "usage: %s [fill|preserve|crossframe|painteronly|bench]\n", argv[0] );
        return 4;
    }
    samples = !strcmp( mode, "fill" ) ? fill_samples : !strcmp( mode, "preserve" ) ? preserve_samples : cross_samples;
    count = !strcmp( mode, "fill" ) ? ARRAYSIZE(fill_samples)
          : !strcmp( mode, "preserve" ) ? ARRAYSIZE(preserve_samples) : ARRAYSIZE(cross_samples);

    if (!open_events( GetCurrentProcessId(), TRUE )) return 4;
    cls.lpfnWndProc = DefWindowProcA;
    cls.hInstance = GetModuleHandleA( NULL );
    cls.hbrBackground = GetStockObject( GRAY_BRUSH );
    cls.lpszClassName = "foreign_dc_probe_root";
    RegisterClassA( &cls );
    root = CreateWindowExA( 0, cls.lpszClassName, "foreign DC probe",
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE,
                            100, 100, CHILD_W + 180, CHILD_H + 160, 0, 0, cls.hInstance, NULL );
    if (!root) return 4;
    UpdateWindow( root );

    snprintf( args, sizeof(args), "browser %p %lu %s", root, GetCurrentProcessId(), mode );
    if (!spawn( args, &browser )) return 4;
    wait[0] = events[EV_READY];
    wait[1] = browser.hProcess;
    if (wait_pumping( 2, wait, 15000 ) != 0)
    {
        printf( "setup: browser did not create its window\n" );
        goto done;
    }
    if (check_samples( root, orange_samples, ARRAYSIZE(orange_samples), 5000, FALSE ))
    {
        printf( "setup: the browser's orange never reached the root window\n" );
        check_samples( root, orange_samples, ARRAYSIZE(orange_samples), 0, TRUE );
        goto done;
    }

    snprintf( args, sizeof(args), "painter %s %p %lu", mode, root, GetCurrentProcessId() );
    if (painter_only) SetEnvironmentVariableA( "RUNEON_FOREIGN_WINDOW_SURFACE", "1" );
    started = spawn( args, &painter );
    if (painter_only)
    {
        SetEnvironmentVariableA( "RUNEON_FOREIGN_WINDOW_SURFACE", NULL );
        printf( "painteronly: the opt-in is set for the painter only\n" );
    }
    if (!started) goto done;
    wait[0] = events[EV_PAINTED];
    wait[1] = painter.hProcess;
    if (wait_pumping( 2, wait, 60000 ) != 0)
    {
        printf( "setup: painter did not finish drawing\n" );
        goto done;
    }
    if (!strcmp( mode, "bench" ))
    {
        ret = 0;
        goto done;
    }
    if (!strcmp( mode, "crossframe" ))
    {
        bad = check_samples( root, cross_stage1_samples, ARRAYSIZE(cross_stage1_samples), 3000, FALSE );
        printf( "crossframe session 1: %u of %u samples wrong\n", bad, (unsigned int)ARRAYSIZE(cross_stage1_samples) );
        check_samples( root, cross_stage1_samples, ARRAYSIZE(cross_stage1_samples), 0, TRUE );
        ret = 1;
        if (bad) goto done;
        SetEvent( events[EV_CONTROL] );
        wait[0] = events[EV_CONTROL_DONE];
        wait[1] = browser.hProcess;
        if (wait_pumping( 2, wait, 15000 ) != 0) goto done;
        bad = check_samples( root, cross_repaint_samples, ARRAYSIZE(cross_repaint_samples), 5000, FALSE );
        printf( "crossframe owner repaint: %u of %u samples wrong\n", bad, (unsigned int)ARRAYSIZE(cross_repaint_samples) );
        check_samples( root, cross_repaint_samples, ARRAYSIZE(cross_repaint_samples), 0, TRUE );
        if (bad) goto done;
        SetEvent( events[EV_PAINT2] );
        wait[0] = events[EV_PAINTED2];
        wait[1] = painter.hProcess;
        if (wait_pumping( 2, wait, 15000 ) != 0) { ret = 4; goto done; }
        bad = check_samples( root, samples, count, 3000, FALSE );
        printf( "verdict (%s): %u of %u samples wrong\n", mode, bad, count );
        check_samples( root, samples, count, 0, TRUE );
        ret = bad ? 1 : 0;
        printf( "result: %s\n", ret ? "FAIL (foreign drawing)" : "PASS" );
        goto done;
    }

    bad = check_samples( root, samples, count, 3000, FALSE );
    printf( "verdict (%s): %u of %u samples wrong\n", mode, bad, count );
    check_samples( root, samples, count, 0, TRUE );

    SetEvent( events[EV_CONTROL] );
    wait[0] = events[EV_CONTROL_DONE];
    wait[1] = browser.hProcess;
    control_ok = wait_pumping( 2, wait, 15000 ) == 0 &&
                 !check_samples( root, magenta_samples, ARRAYSIZE(magenta_samples), 5000, FALSE );
    printf( "owner control: %s\n", control_ok ? "pass" : "FAIL" );

    ret = bad ? 1 : control_ok ? 0 : 2;
    printf( "result: %s\n", ret == 0 ? "PASS" : ret == 1 ? "FAIL (foreign drawing)" : "FAIL (owner control)" );

done:
    SetEvent( events[EV_QUIT] );
    if (browser.hProcess)
    {
        /* Keep pumping: destroying the browser's cross-process child needs this thread. */
        wait[0] = browser.hProcess;
        if (wait_pumping( 1, wait, 15000 ) != 0) TerminateProcess( browser.hProcess, 4 );
        CloseHandle( browser.hThread );
        CloseHandle( browser.hProcess );
    }
    if (painter.hProcess)
    {
        wait[0] = painter.hProcess;
        if (wait_pumping( 1, wait, 5000 ) != 0) TerminateProcess( painter.hProcess, 4 );
        CloseHandle( painter.hThread );
        CloseHandle( painter.hProcess );
    }
    DestroyWindow( root );
    fflush( stdout );
    return ret;
}
