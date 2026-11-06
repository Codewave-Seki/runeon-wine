/* LGPL-2.1-or-later. Checks the concurrent_vector internals of concrt140 and msvcp120 (0166).
 * Layout check: after reserving 16 four-byte elements in an empty vector, the
 * allocator must be asked for 16 elements (concrt140 asked for 16 * 4) and
 * segments 1-3, which live inside segment 0, must start 2, 4 and 8 elements in
 * (concrt140 placed them 8 times further, past the allocation).
 * Race check: 8 threads push_back 2000 elements each while the allocator sleeps,
 * so other threads see a segment that is still being allocated. Every returned
 * address must lie inside an allocated segment, and every element must read back
 * the value its thread wrote. Without the fix _Internal_capacity() counted that
 * segment as allocated and push_back() returned the allocation marker plus an
 * offset, near address 1.
 * x86_64 only (the exports use the win64 names).
 *   x86_64-w64-mingw32-gcc -O1 -Wall -Werror -o concurrent-vector-probe.exe concurrent-vector-probe.c
 * Run it with the builtin DLLs; it must print PASS. It does not cover i386,
 * the other concurrent_vector functions, or msvcp100/msvcp110. */
#include <windows.h>
#include <stdio.h>

typedef struct vector_base
{
    void *(__cdecl *allocator)(struct vector_base *, size_t);
    void *storage[3];
    size_t first_block;
    size_t early_size;
    void **segment;
} vector_base;

typedef void *(__cdecl *push_back_func)(vector_base *, size_t, size_t *);
typedef void (__cdecl *reserve_func)(vector_base *, size_t, size_t, size_t);

#define THREADS 8
#define PER_THREAD 2000
#define MAX_ALLOCS 64

static int failures;
static size_t last_count;
static struct { char *base; size_t bytes; } allocs[MAX_ALLOCS];
static LONG alloc_count;
static push_back_func push_back;
static vector_base vec;
static size_t elem_size;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

static void * __cdecl allocator(vector_base *v, size_t count)
{
    LONG i = InterlockedIncrement(&alloc_count) - 1;
    char *p;

    (void)v;

    if (i >= MAX_ALLOCS) return NULL;
    last_count = count;
    Sleep(2);  /* widen the window in which the segment is still being allocated */
    p = calloc(count, elem_size);
    allocs[i].bytes = count * elem_size;
    allocs[i].base = p;
    return p;
}

static BOOL inside_allocation(const char *p)
{
    LONG i, n = alloc_count;

    for (i = 0; i < n && i < MAX_ALLOCS; i++)
        if (allocs[i].base && p >= allocs[i].base && p + elem_size <= allocs[i].base + allocs[i].bytes)
            return TRUE;
    return FALSE;
}

static void reset(size_t size)
{
    memset(&vec, 0, sizeof(vec));
    memset(allocs, 0, sizeof(allocs));
    alloc_count = 0;
    vec.allocator = allocator;
    vec.segment = vec.storage;
    elem_size = size;
}

static void check_layout(HMODULE dll, const char *name)
{
    reserve_func reserve = (void *)GetProcAddress(dll,
            "?_Internal_reserve@_Concurrent_vector_base_v4@details@Concurrency@@IEAAX_K00@Z");
    char *seg0;

    if (!reserve) { printf("FAIL: %s has no _Internal_reserve\n", name); ++failures; return; }
    reset(4);
    reserve(&vec, 16, 4, ~(size_t)0 / 4);
    seg0 = vec.segment[0];
    CHECK(last_count == 16, "allocator asked for the element count");
    CHECK(vec.segment[1] == seg0 + 4 * 2, "segment 1 inside segment 0");
    CHECK(vec.segment[2] == seg0 + 4 * 4, "segment 2 inside segment 0");
    CHECK(vec.segment[3] == seg0 + 4 * 8, "segment 3 inside segment 0");
}

static DWORD WINAPI pusher(void *arg)
{
    ULONG_PTR id = (ULONG_PTR)arg;
    size_t idx;
    int i;

    for (i = 0; i < PER_THREAD; i++)
    {
        size_t *p = push_back(&vec, elem_size, &idx);
        if (!inside_allocation((char *)p))
        {
            printf("FAIL: push_back returned %p for index %lu\n", p, (unsigned long)idx);
            InterlockedIncrement((LONG *)&failures);
            return 1;
        }
        *p = (id << 32) | idx;
    }
    return 0;
}

static void check_race(HMODULE dll, const char *name)
{
    HANDLE threads[THREADS];
    size_t idx, seg, base;
    int before = failures;
    ULONG_PTR i;

    push_back = (void *)GetProcAddress(dll,
            "?_Internal_push_back@_Concurrent_vector_base_v4@details@Concurrency@@IEAAPEAX_KAEA_K@Z");
    if (!push_back) { printf("FAIL: %s has no _Internal_push_back\n", name); ++failures; return; }
    reset(sizeof(size_t));
    for (i = 0; i < THREADS; i++)
        threads[i] = CreateThread(NULL, 0, pusher, (void *)i, 0, NULL);
    WaitForMultipleObjects(THREADS, threads, TRUE, 60000);
    for (i = 0; i < THREADS; i++) CloseHandle(threads[i]);
    if (failures != before) return;

    CHECK(vec.early_size == THREADS * PER_THREAD, "element count");
    for (idx = 0; idx < vec.early_size; idx++)
    {
        size_t value;
        for (seg = 0; ((size_t)1 << (seg + 1)) <= (idx | 1); seg++);
        base = seg ? (size_t)1 << seg : 0;
        value = ((size_t *)vec.segment[seg])[idx - base];
        if ((value & 0xffffffff) != idx)
        {
            printf("FAIL: element %lu reads %llx\n", (unsigned long)idx, (unsigned long long)value);
            ++failures;
            break;
        }
    }
}

int main(void)
{
    static const char *names[] = { "concrt140.dll", "msvcp120.dll" };
    unsigned int i;

    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    {
        HMODULE dll = LoadLibraryA(names[i]);
        int before = failures;

        if (!dll) { printf("FAIL: cannot load %s\n", names[i]); ++failures; continue; }
        check_layout(dll, names[i]);
        check_race(dll, names[i]);
        printf("%s: %s\n", names[i], failures == before ? "ok" : "failed");
    }
    printf("%s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
