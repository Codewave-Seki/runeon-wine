/*
 * Showing a cursor set by a window hosted in a window of another process.
 *
 * Two processes, as with a WebView2 browser window hosted inside an application window:
 *   root   owns the top-level window; the server asks it to show the cursor, because it owns
 *          the top-level window under the pointer;
 *   child  owns a child window parented to the root window (so both threads share one input
 *          state), creates the cursors and sets them while the pointer is over its window.
 * Unpatched, the root process cannot read the child's cursors and the shown cursor does not
 * change. With RUNEON_FOREIGN_WINDOW_SURFACE=1, patch 0180 shows the arrow and patch 0181 a
 * copy of the cursor's image.
 *
 * The child drives five phases, each signalled with named events; the root pumps messages
 * for 500 ms, reports the cursor its input state holds and acknowledges:
 *   1 custom    cursor A, 16x16 32 bpp with alpha, red/blue halves, rows 0-1 transparent,
 *               hotspot (10,5)
 *   2 hand      LoadCursor(NULL, IDC_HAND)
 *   3 hidden    SetCursor(NULL)
 *   4 custom    cursor A again
 *   5 recreate  DestroyCursor(A), then cursor B, 16x16 green/yellow, hotspot (3,12), which
 *               may reuse A's handle slot with a new generation
 *   6 large     a 300x300 cursor, larger than a snapshot carries, so it always gets the arrow
 *   7 hand      the hand again
 *   8 large     the 300x300 cursor again: it gave no snapshot before, so it is not pulled again
 * Processes that own windows pump messages while they wait: the child must answer the root's
 * snapshot request.
 *
 * The cursor macOS shows cannot be read from inside Wine, so the verdict on the image is the
 * trace of the root process. Run with WINEDEBUG=+pid,+cursor and stdout and stderr together;
 * the phase lines printed here carry the handles to look for. With 0181 and the opt-in,
 * per phase in the root process (pid printed as "root pid"):
 *   1 "foreign cursor <A>: snapshot 16x16 hotspot (10,5) flags 0x3 applied as <copy>",
 *     then macdrv "setting cursor with cursor_name (null) cursor_frames <non-null>"
 *   2 "foreign cursor <hand>: snapshot ... applied as <copy>" with the child's
 *     "snapshot of <hand>: ... module ...user32.dll... res 32649", macdrv mapping
 *     "user32.dll,32649" to pointingHandCursor, and "releasing foreign cursor copy" of phase 1
 *   3 "hcursor 00000000", macdrv "cursor_name (null) cursor_frames 0x0" (hidden), and
 *     "releasing foreign cursor copy" of phase 2
 *   4 a new "foreign cursor <A>: snapshot 16x16 hotspot (10,5) ... applied as" line
 *   5 "foreign cursor <B>: snapshot 16x16 hotspot (3,12) ... applied as" and
 *     "releasing foreign cursor copy" of phase 4
 *   6 "foreign cursor <large>: showing the arrow", with the child's "no snapshot of <large>"
 *   7 "foreign cursor <hand>: snapshot ... applied as <copy>"
 *   8 "foreign cursor <large>: no snapshot before, not pulling again", then the arrow again
 * The child process logs a matching "snapshot of <handle>" line for phases 1, 2, 4 and 5.
 *
 * Mechanical verdict on the trace (counts of fixed strings in the whole log):
 *   with the opt-in:    "snapshot 16x16 hotspot (10,5)" 2, "snapshot 16x16 hotspot (3,12)" 1,
 *                       "showing the arrow" 2, "not pulling again" 1, "no snapshot of" 1,
 *                       "superseded" 0, "GetIconInfoExW failed" 0;
 *   without the opt-in: "GetIconInfoExW failed" 7 (every phase but 3), "foreign cursor " 0.
 * With 0180 alone, or when no snapshot arrives, each foreign cursor logs "foreign cursor <handle>: showing
 * the arrow" and macdrv maps "user32.dll,32512" to arrowCursor; unpatched or without the
 * opt-in, the root logs "icon handle <handle> from other process" and "GetIconInfoExW failed".
 * No phase may show "superseded" in this probe: nothing replaces a cursor while it is pulled.
 *
 * SetCursorPos() moves the real pointer onto the child window; do not move the mouse while
 * the probe runs. A phase whose pointer is not over the child window is reported.
 *
 * Covers static cursors set with SetCursor() from the thread that created them; animated
 * cursors, cursors created by a thread without a message loop and DPI scaling are not covered.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -o foreign-cursor-probe.exe foreign-cursor-probe.c -lgdi32 -luser32
 *
 * Exit codes: 0 every phase acknowledged with the expected input state, 1 the root's input
 * state did not match a phase, 4 setup failure. Exit code 0 says nothing about the image.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHILD_X 40
#define CHILD_Y 40
#define CHILD_W 200
#define CHILD_H 120

#define CHILD_CLASS "foreign_cursor_probe_child"

enum { EV_READY, EV_PHASE, EV_ACK, EV_COUNT };
static const char *event_names[EV_COUNT] = { "ready", "phase", "ack" };
static HANDLE events[EV_COUNT];

static BOOL open_events( DWORD root_pid, BOOL create )
{
    char name[64];
    int i;

    for (i = 0; i < EV_COUNT; i++)
    {
        snprintf( name, sizeof(name), "foreign_cursor_probe_%lu_%s", root_pid, event_names[i] );
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

/* size x size, 32 bpp with alpha: left and right halves in two colours, rows 0-1 transparent. */
static HCURSOR make_cursor( int size, DWORD left, DWORD right, int hot_x, int hot_y )
{
    BYTE *mask_bits;
    BITMAPINFO bmi = { 0 };
    ICONINFO info = { 0 };
    HBITMAP color, mask;
    HCURSOR cursor;
    DWORD *bits;
    int x, y;

    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    if (!(color = CreateDIBSection( NULL, &bmi, DIB_RGB_COLORS, (void **)&bits, NULL, 0 ))) return 0;
    for (y = 0; y < size; y++)
        for (x = 0; x < size; x++)
            bits[y * size + x] = y < 2 ? 0 : x < size / 2 ? left : right;
    /* CreateBitmap() rows are WORD aligned */
    mask_bits = calloc( (size + 15) / 16 * 2, size );
    mask = mask_bits ? CreateBitmap( size, size, 1, 1, mask_bits ) : 0;
    free( mask_bits );
    if (!mask)
    {
        DeleteObject( color );
        return 0;
    }
    info.fIcon = FALSE;
    info.xHotspot = hot_x;
    info.yHotspot = hot_y;
    info.hbmMask = mask;
    info.hbmColor = color;
    cursor = CreateIconIndirect( &info );
    DeleteObject( mask );
    DeleteObject( color );
    return cursor;
}

static HCURSOR child_cursor;

static LRESULT CALLBACK child_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    /* keep the phase's cursor while the pointer is over the window */
    if (msg == WM_SETCURSOR && LOWORD(lp) == HTCLIENT)
    {
        SetCursor( child_cursor );
        return TRUE;
    }
    return DefWindowProcA( hwnd, msg, wp, lp );
}

/* Child side of one phase: set the cursor, signal, wait for the root's acknowledgement. */
static BOOL run_phase( HWND child, int phase, const char *name, HCURSOR cursor )
{
    HANDLE ack = events[EV_ACK];
    POINT pt;

    child_cursor = cursor;
    SetCursor( cursor );
    GetCursorPos( &pt );
    printf( "child: phase %d (%s): cursor %p, GetCursor() %p%s\n", phase, name, cursor, GetCursor(),
            WindowFromPoint( pt ) == child ? "" : ", POINTER NOT OVER THE CHILD WINDOW" );
    fflush( stdout );
    SetEvent( events[EV_PHASE] );
    if (wait_pumping( 1, &ack, 15000 ) != 0) return FALSE;
    ResetEvent( events[EV_ACK] );
    return TRUE;
}

static int run_child( HWND root )
{
    WNDCLASSA cls = { 0 };
    HCURSOR a, b, hand, large;
    HWND child;
    RECT rc;
    int ret = 4;

    cls.lpfnWndProc = child_proc;
    cls.hInstance = GetModuleHandleA( NULL );
    cls.hbrBackground = GetStockObject( WHITE_BRUSH );
    cls.lpszClassName = CHILD_CLASS;
    if (!RegisterClassA( &cls )) return 4;
    child = CreateWindowExA( 0, CHILD_CLASS, NULL, WS_CHILD | WS_VISIBLE,
                             CHILD_X, CHILD_Y, CHILD_W, CHILD_H, root, 0, cls.hInstance, NULL );
    if (!child) return 4;
    a = make_cursor( 16, 0xffff0000, 0xff0000ff, 10, 5 );
    hand = LoadCursorA( NULL, (LPCSTR)IDC_HAND );
    if (!a || !hand) goto done;

    GetWindowRect( child, &rc );
    SetCursorPos( (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 );
    printf( "child: pid %04lx tid %04lx window %p cursor A %p hand %p\n", GetCurrentProcessId(),
            GetCurrentThreadId(), child, a, hand );
    fflush( stdout );
    SetEvent( events[EV_READY] );
    wait_pumping( 0, NULL, 300 );

    if (!run_phase( child, 1, "custom A", a )) goto done;
    if (!run_phase( child, 2, "hand", hand )) goto done;
    if (!run_phase( child, 3, "hidden", NULL )) goto done;
    if (!run_phase( child, 4, "custom A again", a )) goto done;
    DestroyCursor( a );
    if (!(b = make_cursor( 16, 0xff00ff00, 0xffffff00, 3, 12 ))) goto done;
    printf( "child: destroyed A %p, created B %p (%s handle slot)\n", a, b,
            LOWORD(b) == LOWORD(a) ? "same" : "different" );
    if (!run_phase( child, 5, "recreated B", b )) goto done;
    /* larger than any snapshot carries: always the arrow fallback */
    if (!(large = make_cursor( 300, 0xff00ffff, 0xffff00ff, 0, 0 ))) goto done;
    if (!run_phase( child, 6, "300x300", large )) goto done;
    if (!run_phase( child, 7, "hand again", hand )) goto done;
    if (!run_phase( child, 8, "300x300 again", large )) goto done;
    child_cursor = NULL;
    SetCursor( NULL );
    DestroyCursor( b );
    DestroyCursor( large );
    ret = 0;

done:
    DestroyWindow( child );
    return ret;
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
    PROCESS_INFORMATION child = { 0 };
    const char *env = getenv( "RUNEON_FOREIGN_WINDOW_SURFACE" );
    WNDCLASSA cls = { 0 };
    HANDLE wait[2];
    HCURSOR prev = (HCURSOR)1;
    DWORD code = 4;
    char args[128];
    HWND root;
    int phase = 0, mismatches = 0, ret = 4;

    if (argc == 4 && !strcmp( argv[1], "child" ))
    {
        if (!open_events( strtoul( argv[3], NULL, 10 ), FALSE )) return 4;
        return run_child( (HWND)(ULONG_PTR)strtoull( argv[2], NULL, 16 ) );
    }

    printf( "root pid %04lx tid %04lx, RUNEON_FOREIGN_WINDOW_SURFACE=%s\n",
            GetCurrentProcessId(), GetCurrentThreadId(), env ? env : "(unset)" );
    if (!open_events( GetCurrentProcessId(), TRUE )) return 4;
    cls.lpfnWndProc = DefWindowProcA;
    cls.hInstance = GetModuleHandleA( NULL );
    cls.hCursor = LoadCursorA( NULL, (LPCSTR)IDC_ARROW );
    cls.hbrBackground = GetStockObject( GRAY_BRUSH );
    cls.lpszClassName = "foreign_cursor_probe_root";
    RegisterClassA( &cls );
    root = CreateWindowExA( 0, cls.lpszClassName, "foreign cursor probe",
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE,
                            100, 100, 320, 240, 0, 0, cls.hInstance, NULL );
    if (!root) return 4;
    UpdateWindow( root );
    SetForegroundWindow( root );

    snprintf( args, sizeof(args), "child %p %lu", root, GetCurrentProcessId() );
    if (!spawn( args, &child )) return 4;
    wait[0] = events[EV_READY];
    wait[1] = child.hProcess;
    if (wait_pumping( 2, wait, 15000 ) != 0)
    {
        printf( "setup: child did not create its window\n" );
        goto done;
    }

    wait[0] = events[EV_PHASE];
    while (wait_pumping( 2, wait, 30000 ) == 0)
    {
        HCURSOR cursor;

        ResetEvent( events[EV_PHASE] );
        phase++;
        /* let the server's cursor notification reach this thread and be handled */
        wait_pumping( 0, NULL, 500 );
        /* the threads share one input state, so this is the cursor the child set */
        cursor = GetCursor();
        printf( "root: phase %d settled, input cursor %p\n", phase, cursor );
        /* phase 3 hides the cursor; every other phase sets a cursor different from the last */
        if ((phase == 3) != !cursor || cursor == prev)
        {
            printf( "root: phase %d input cursor UNEXPECTED\n", phase );
            mismatches++;
        }
        prev = cursor;
        fflush( stdout );
        SetEvent( events[EV_ACK] );
    }

    wait[0] = child.hProcess;
    if (wait_pumping( 1, wait, 15000 ) != 0) printf( "setup: child did not exit\n" );
    else GetExitCodeProcess( child.hProcess, &code );
    if (code || phase != 8) printf( "setup: child exit code %lu after %d phases\n", code, phase );
    else ret = mismatches ? 1 : 0;
    printf( "result: %s\n", ret == 0 ? "PASS (input state; check the trace for the image)"
                            : ret == 1 ? "FAIL (input state)" : "FAIL (setup)" );

done:
    if (child.hProcess)
    {
        /* Keep pumping: destroying the child's cross-process window needs this thread. */
        wait[0] = child.hProcess;
        if (wait_pumping( 1, wait, 15000 ) != 0) TerminateProcess( child.hProcess, 4 );
        CloseHandle( child.hThread );
        CloseHandle( child.hProcess );
    }
    DestroyWindow( root );
    fflush( stdout );
    return ret;
}
