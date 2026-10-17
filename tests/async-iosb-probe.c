/* Probe for asynchronous I/O completion into an IO status block that is or is
 * no longer writable (patch 0123).
 *
 *   async-iosb-probe.exe              run every case in a child process
 *   async-iosb-probe.exe CASE         run one case
 *
 * CASE is PLACEMENT-DELIVERY-NOTIFY:
 *   PLACEMENT  normal, freed, decommit, readonly, guard, cross (Information on
 *              a no-access page after Status), watch (write-watched pages)
 *   DELIVERY   wait: the reader blocks in a wait, so Wine runs the completion
 *              in the syscall (the writer allows 300 ms to enter the wait, so
 *              a very slow run may deliver it by signal instead); spin: the
 *              reader spins in user mode until the I/O's own event has fired,
 *              so the completion was delivered by signal and ran on the
 *              signal stack, and a spin that times out fails the case
 *   NOTIFY     event or port (I/O completion port)
 *
 * The reader issues an overlapped read on a named pipe; a second thread then
 * changes the pages holding the OVERLAPPED and writes the payload. The data
 * buffer and the event handle always stay valid. A case prints one result
 * line and exits with CASE_PASSED when every check holds. Without the patch,
 * a status block that is not writable makes the wait fail in the wait case
 * and aborts the reader in the spin case; the parent reports every child
 * that does not exit with CASE_PASSED.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define DATA_SIZE 64
#define PORT_KEY ((ULONG_PTR)0x5eed)
#define PENDING ((ULONG_PTR)0x103)
#define CHILD_TIMEOUT_MS 20000
/* a reader aborted by Wine leaves the writer to end the process with 0 */
#define CASE_PASSED 0x600d

static const char *placements[] = { "normal", "freed", "decommit", "readonly", "guard", "cross", "watch" };
static const char *deliveries[] = { "wait", "spin" };
static const char *notifies[] = { "event", "port" };

static char payload[DATA_SIZE];
static char buffer[DATA_SIZE];
static char placement[16], delivery[8], notify[8];
static HANDLE server, client, event, port;
static OVERLAPPED *ov;
static char *pages;
static SIZE_T page_size;
static volatile LONG reader_ready;
/* set by the writer once the I/O's own event fired: 1, or -1 on a timeout */
static volatile LONG completion;
static int failures;

static void check( int ok, const char *what )
{
    if (ok) return;
    printf( "  check failed: %s\n", what );
    failures++;
}

static DWORD WINAPI writer( void *arg )
{
    DWORD written, old;
    BOOL ok = TRUE;
    (void)arg;

    while (!reader_ready) Sleep( 1 );
    Sleep( 300 );  /* let a waiting reader enter its wait */

    /* a page change that did not happen would make the case pass for nothing */
    if (!strcmp( placement, "freed" )) ok = VirtualFree( pages, 0, MEM_RELEASE );
    else if (!strcmp( placement, "decommit" )) ok = VirtualFree( pages, 2 * page_size, MEM_DECOMMIT );
    else if (!strcmp( placement, "readonly" )) ok = VirtualProtect( pages, 2 * page_size, PAGE_READONLY, &old );
    else if (!strcmp( placement, "guard" )) ok = VirtualProtect( pages, 2 * page_size, PAGE_READWRITE | PAGE_GUARD, &old );
    else if (!strcmp( placement, "cross" )) ok = VirtualProtect( pages + page_size, page_size, PAGE_NOACCESS, &old );
    else if (!strcmp( placement, "watch" )) ok = !ResetWriteWatch( pages, 2 * page_size );
    if (!ok)
    {
        printf( "  writer: changing the status block pages failed %lu\n", GetLastError() );
        ExitProcess( 4 );
    }

    if (!WriteFile( client, payload, DATA_SIZE, &written, NULL ) || written != DATA_SIZE)
        printf( "  writer: WriteFile failed %lu\n", GetLastError() );
    /* the event is a separate handle, so it shows completion without the status block */
    InterlockedExchange( (LONG *)&completion, WaitForSingleObject( event, 5000 ) == WAIT_OBJECT_0 ? 1 : -1 );
    return 0;
}

static int run_case( const char *name )
{
    char pipe_name[64];
    BOOL iosb_readable = TRUE;
    DWORD bytes = 0, wait = 0, error = 0;
    ULONG_PTR key = 0;
    OVERLAPPED *packet = NULL, *issued;
    OVERLAPPED connect = { 0 };
    SYSTEM_INFO info;
    HANDLE thread;
    int i;

    if (sscanf( name, "%15[^-]-%7[^-]-%7s", placement, delivery, notify ) != 3) return 1;
    for (i = 0; i < DATA_SIZE; i++) payload[i] = (char)('A' + i % 26);

    GetSystemInfo( &info );
    page_size = info.dwPageSize;
    pages = VirtualAlloc( NULL, 2 * page_size,
                          MEM_RESERVE | MEM_COMMIT | (!strcmp( placement, "watch" ) ? MEM_WRITE_WATCH : 0),
                          PAGE_READWRITE );
    if (!pages) return 1;
    /* cross: Status in the last slot of the first page, Information on the next */
    ov = (OVERLAPPED *)(!strcmp( placement, "cross" ) ? pages + page_size - sizeof(ULONG_PTR) : pages + 64);
    issued = ov;

    sprintf( pipe_name, "\\\\.\\pipe\\runeon-iosb-%lu", GetCurrentProcessId() );
    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_WAIT,
                               1, 4096, 4096, 0, NULL );
    if (server == INVALID_HANDLE_VALUE) return 1;
    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (client == INVALID_HANDLE_VALUE) return 1;
    connect.hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
    if (!ConnectNamedPipe( server, &connect ) && GetLastError() != ERROR_PIPE_CONNECTED
        && GetLastError() != ERROR_IO_PENDING) return 1;
    if (!strcmp( notify, "port" ) && !(port = CreateIoCompletionPort( server, NULL, PORT_KEY, 0 ))) return 1;

    event = CreateEventA( NULL, TRUE, FALSE, NULL );
    memset( ov, 0, sizeof(*ov) );
    ov->hEvent = event;
    if (ReadFile( server, buffer, DATA_SIZE, NULL, ov ) || GetLastError() != ERROR_IO_PENDING)
    {
        printf( "%s: read did not pend (%lu)\n", name, GetLastError() );
        return 1;
    }

    thread = CreateThread( NULL, 0, writer, NULL, 0, NULL );
    if (!strcmp( delivery, "spin" ))
    {
        DWORD start = GetTickCount();
        BOOL seen = FALSE;
        InterlockedExchange( (LONG *)&reader_ready, 1 );
        /* user-mode spin, no syscalls, until the completion is known to have
         * arrived: it can only have been delivered to this thread by signal */
        while (!completion && GetTickCount() - start < 10000)
        {
            if (!seen && !strcmp( placement, "normal" ) && __atomic_load_n( &ov->Internal, __ATOMIC_ACQUIRE ) != PENDING)
            {
                /* smoke check only: x86 stores are not reordered anyway */
                check( ov->InternalHigh == DATA_SIZE, "Information visible with Status" );
                check( !memcmp( buffer, payload, DATA_SIZE ), "data visible with Status" );
                seen = TRUE;
            }
        }
        check( completion == 1, "completion arrived while spinning" );
    }
    else InterlockedExchange( (LONG *)&reader_ready, 1 );

    if (!strcmp( notify, "port" ))
    {
        BOOL ok = GetQueuedCompletionStatus( port, &bytes, &key, &packet, 5000 );
        error = ok ? 0 : GetLastError();
        check( ok, "completion port packet" );
        check( bytes == DATA_SIZE, "packet byte count" );
        check( key == PORT_KEY, "packet key" );
        check( packet == issued, "packet OVERLAPPED pointer" );
    }
    else
    {
        wait = WaitForSingleObject( event, 5000 );
        error = wait == WAIT_OBJECT_0 ? 0 : GetLastError();
        check( wait == WAIT_OBJECT_0, "event signaled" );
    }
    WaitForSingleObject( thread, 5000 );
    check( !memcmp( buffer, payload, DATA_SIZE ), "data" );

    if (!strcmp( placement, "freed" ) || !strcmp( placement, "decommit" )) iosb_readable = FALSE;
    if (!strcmp( placement, "guard" ))
    {
        MEMORY_BASIC_INFORMATION mbi;
        /* reading would consume the guard; skipping the write leaves it armed,
         * which is this patch's choice, not measured against Windows */
        iosb_readable = FALSE;
        check( VirtualQuery( ov, &mbi, sizeof(mbi) ) && (mbi.Protect & PAGE_GUARD), "guard page untouched" );
    }
    if (iosb_readable)
    {
        if (!strcmp( placement, "normal" ) || !strcmp( placement, "watch" ))
        {
            check( ov->Internal == 0, "Status written" );
            check( ov->InternalHigh == DATA_SIZE, "Information written" );
        }
        else
            check( ov->Internal == PENDING, "Status not published without Information" );
        if (!strcmp( placement, "readonly" )) check( ov->InternalHigh == 0, "read-only Information unchanged" );
    }
    if (!strcmp( placement, "watch" ))
    {
        void *written[4];
        ULONG_PTR count = 4;
        ULONG granularity;
        check( !GetWriteWatch( 0, pages, 2 * page_size, written, &count, &granularity ) && count >= 1
               && written[0] == pages, "write watch recorded the status block page" );
    }

    printf( "%s: %s (wait %lu, error %lu, bytes %lu)\n", name, failures ? "FAIL" : "ok", wait, error, bytes );
    return failures ? 2 : CASE_PASSED;
}

static int run_all( const char *self )
{
    int total = 0, failed = 0;
    unsigned int p, d, n;

    for (p = 0; p < sizeof(placements) / sizeof(*placements); p++)
    for (d = 0; d < sizeof(deliveries) / sizeof(*deliveries); d++)
    for (n = 0; n < sizeof(notifies) / sizeof(*notifies); n++)
    {
        char cmd[MAX_PATH + 64];
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        DWORD code = 0;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        sprintf( cmd, "\"%s\" %s-%s-%s", self, placements[p], deliveries[d], notifies[n] );
        total++;
        if (!CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi ))
        {
            printf( "%s-%s-%s: cannot start (%lu)\n", placements[p], deliveries[d], notifies[n], GetLastError() );
            failed++;
            continue;
        }
        if (WaitForSingleObject( pi.hProcess, CHILD_TIMEOUT_MS ) != WAIT_OBJECT_0)
        {
            TerminateProcess( pi.hProcess, 3 );
            printf( "%s-%s-%s: TIMEOUT (child did not finish)\n", placements[p], deliveries[d], notifies[n] );
            failed++;
        }
        else if (!GetExitCodeProcess( pi.hProcess, &code ) || code != CASE_PASSED)
        {
            printf( "%s-%s-%s: exit code %#lx\n", placements[p], deliveries[d], notifies[n], code );
            failed++;
        }
        fflush( stdout );
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
    }
    printf( "%d of %d cases passed\n", total - failed, total );
    return failed ? 1 : 0;
}

int main( int argc, char **argv )
{
    char self[MAX_PATH];

    setvbuf( stdout, NULL, _IONBF, 0 );
    if (argc > 1) return run_case( argv[1] );
    GetModuleFileNameA( NULL, self, sizeof(self) );
    return run_all( self );
}
