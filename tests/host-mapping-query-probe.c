/*
 * VirtualQuery() must only report MEM_FREE for ranges that can be allocated.
 * On macOS x86_64 the loader sits at 0x200000000 with Rosetta and host
 * allocations above it; this walks the regions from 0x150000000 and checks
 * that every free range accepts a reservation of its full size (capped) and
 * that the loader address is not reported free. It does not cover WoW64
 * 32-bit processes or the MemoryRegionInformation query.
 *
 * Build: x86_64-w64-mingw32-gcc -O1 -o host-mapping-query-probe.exe host-mapping-query-probe.c
 * Exit code: 0 pass, 1 fail.
 */
#include <windows.h>
#include <stdio.h>

#define WALK_START 0x150000000ull
#define WALK_END   0x400000000ull
#define LOADER     0x200000000ull
#define RESERVE_CAP 0x100000000ull

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
    int failures = 0, free_ranges = 0;

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

        if (!VirtualQuery( (void *)addr, &mbi, sizeof(mbi) ) || !mbi.RegionSize) break;
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
                    VirtualFree( p, 0, MEM_RELEASE );
                }
            }
        }
        addr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;
    }

    printf( "%d free ranges, %d failures\n", free_ranges, failures );
    return failures ? 1 : 0;
}
