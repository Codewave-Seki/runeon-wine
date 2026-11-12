/*
 * VirtualQuery() must only report MEM_FREE for ranges that can be allocated.
 * On macOS x86_64 the loader sits at 0x200000000 with Rosetta and host
 * allocations above it; this walks the regions from 0x150000000 and checks
 * that every free range accepts a reservation of its full size (capped) and
 * that the loader address is not reported free; at least one free range must
 * take a full RESERVE_CAP reservation. A MemoryRegionInformation query at the
 * loader with a buffer that ends before CommitSize must not write past it.
 * It does not cover WoW64 32-bit processes.
 *
 * Build: x86_64-w64-mingw32-gcc -O1 -o host-mapping-query-probe.exe host-mapping-query-probe.c -lntdll
 * Exit code: 0 pass, 1 fail.
 */
#include <windows.h>
#include <stdio.h>

#define WALK_START 0x150000000ull
#define WALK_END   0x400000000ull
#define LOADER     0x200000000ull
#define RESERVE_CAP 0x100000000ull
#define MEMORY_REGION_INFORMATION_CLASS 2
#define COMMIT_SIZE_OFFSET 24

NTSTATUS NTAPI NtQueryVirtualMemory( HANDLE, PVOID, int, PVOID, SIZE_T, SIZE_T * );

static const char *state_name( DWORD state )
{
    switch (state)
    {
    case MEM_FREE: return "FREE";
    case MEM_RESERVE: return "RESERVE";
    case MEM_COMMIT: return "COMMIT";
    }
    return "?";
}

int main( void )
{
    MEMORY_BASIC_INFORMATION mbi;
    ULONG_PTR addr = WALK_START;
    int failures = 0, free_ranges = 0, full_reserves = 0;
    unsigned char region[COMMIT_SIZE_OFFSET + 8];
    SIZE_T returned = 0;
    NTSTATUS status;
    unsigned int i;

    if (!VirtualQuery( (void *)LOADER, &mbi, sizeof(mbi) ))
    {
        printf( "FAIL query %#llx err %lu\n", LOADER, GetLastError() );
        return 1;
    }
    printf( "loader %#llx state=%s\n", LOADER, state_name( mbi.State ) );
    if (mbi.State == MEM_FREE)
    {
        printf( "FAIL loader address reported free\n" );
        failures++;
    }

    while (addr < WALK_END)
    {
        SIZE_T size;
        void *p;

        if (!VirtualQuery( (void *)addr, &mbi, sizeof(mbi) ) || !mbi.RegionSize)
        {
            printf( "FAIL query %#llx during walk err %lu\n", (unsigned long long)addr, GetLastError() );
            failures++;
            break;
        }
        printf( "%#llx-%#llx %s\n", (unsigned long long)(ULONG_PTR)mbi.BaseAddress,
                (unsigned long long)((ULONG_PTR)mbi.BaseAddress + mbi.RegionSize), state_name( mbi.State ) );
        if (mbi.State == MEM_FREE)
        {
            /* a reservation starts on a 64K boundary, so test from the first one in the range */
            ULONG_PTR start = ((ULONG_PTR)mbi.BaseAddress + 0xffff) & ~(ULONG_PTR)0xffff;
            ULONG_PTR end = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;

            free_ranges++;
            size = end > start ? end - start : 0;
            if (size > RESERVE_CAP) size = RESERVE_CAP;
            size &= ~(SIZE_T)0xffff;
            if (size)
            {
                p = VirtualAlloc( (void *)start, size, MEM_RESERVE, PAGE_NOACCESS );
                if (!p)
                {
                    printf( "FAIL reserve %#llx size %#llx err %lu\n",
                            (unsigned long long)start, (unsigned long long)size, GetLastError() );
                    failures++;
                }
                else
                {
                    printf( "  reserved %#llx size %#llx\n", (unsigned long long)start, (unsigned long long)size );
                    if (size == RESERVE_CAP) full_reserves++;
                    VirtualFree( p, 0, MEM_RELEASE );
                }
            }
        }
        addr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;
    }

    if (!full_reserves)
    {
        printf( "FAIL no free range took a %#llx reservation\n", RESERVE_CAP );
        failures++;
    }

    memset( region, 0xa5, sizeof(region) );
    status = NtQueryVirtualMemory( GetCurrentProcess(), (void *)LOADER, MEMORY_REGION_INFORMATION_CLASS,
                                   region, COMMIT_SIZE_OFFSET, &returned );
    printf( "region info len %u status %#lx\n", COMMIT_SIZE_OFFSET, (unsigned long)status );
    for (i = COMMIT_SIZE_OFFSET; i < sizeof(region); i++)
        if (region[i] != 0xa5)
        {
            printf( "FAIL region info wrote past a %u-byte buffer\n", COMMIT_SIZE_OFFSET );
            failures++;
            break;
        }

    printf( "%d free ranges, %d full reserves, %d failures\n", free_ranges, full_reserves, failures );
    return failures ? 1 : 0;
}
