/* LGPL-2.1-or-later. Checks that the commit charge follows committed memory (0124).
 * PagefileUsage (GetProcessMemoryInfo) and PrivateUsage (ProcessVmCounters, the
 * only place Wine fills it) must stay below 4 GB in a small 64-bit process and
 * grow by at least 200 MB when 256 MB is committed and touched. After the memory
 * is released, PeakPagefileUsage must keep that growth while PagefileUsage falls
 * back. The old macOS counters report address space, which is tens of GB and does
 * not grow, with a zero peak. */
#include <windows.h>
#include <psapi.h>
#include <winternl.h>
#include <stdio.h>

#define COMMIT_SIZE (256u << 20)
#define MIN_GROWTH  (200u << 20)

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

/* VM_COUNTERS_EX as ProcessVmCounters returns it. */
struct vm_counters_ex
{
    SIZE_T PeakVirtualSize, VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage, QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage;
    SIZE_T PrivateUsage;
};

static BOOL read_counters(PROCESS_MEMORY_COUNTERS_EX *pmc)
{
    struct vm_counters_ex vmc;
    memset(pmc, 0, sizeof(*pmc));
    if (!K32GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)pmc, sizeof(*pmc))) return FALSE;
    if (NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)3 /* ProcessVmCounters */,
                                  &vmc, sizeof(vmc), NULL)) return FALSE;
    pmc->PrivateUsage = vmc.PrivateUsage;
    return TRUE;
}

static void print_counters(const char *label, const PROCESS_MEMORY_COUNTERS_EX *pmc)
{
    printf("%s: pagefile=%llu MB private=%llu MB working=%llu MB\n", label,
           (unsigned long long)pmc->PagefileUsage >> 20, (unsigned long long)pmc->PrivateUsage >> 20,
           (unsigned long long)pmc->WorkingSetSize >> 20);
}

int main(void)
{
    PROCESS_MEMORY_COUNTERS_EX before, after, released;
    SYSTEM_INFO si;
    unsigned char *mem;
    SIZE_T i;

    setvbuf(stdout, NULL, _IONBF, 0);
    GetSystemInfo(&si);
    CHECK(read_counters(&before), "GetProcessMemoryInfo before");
    print_counters("before", &before);
#ifdef _WIN64
    CHECK(before.PagefileUsage < ((SIZE_T)4 << 30), "PagefileUsage of a small process below 4 GB");
    CHECK(before.PrivateUsage < ((SIZE_T)4 << 30), "PrivateUsage of a small process below 4 GB");
#endif

    mem = VirtualAlloc(NULL, COMMIT_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(mem != NULL, "commit 256 MB");
    if (!mem) return 1;
    for (i = 0; i < COMMIT_SIZE; i += si.dwPageSize) mem[i] = (unsigned char)i | 1;

    CHECK(read_counters(&after), "GetProcessMemoryInfo after");
    print_counters("after", &after);
    CHECK(after.PagefileUsage >= before.PagefileUsage + MIN_GROWTH, "PagefileUsage grows with committed memory");
    CHECK(after.PrivateUsage >= before.PrivateUsage + MIN_GROWTH, "PrivateUsage grows with committed memory");
    VirtualFree(mem, 0, MEM_RELEASE);
    CHECK(read_counters(&released), "GetProcessMemoryInfo after release");
    print_counters("released", &released);
    CHECK(released.PagefileUsage + MIN_GROWTH <= after.PagefileUsage, "PagefileUsage falls after the release");
    CHECK(released.PeakPagefileUsage >= before.PagefileUsage + MIN_GROWTH, "the peak keeps the committed memory");
    printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
    return failures ? 1 : 0;
}
