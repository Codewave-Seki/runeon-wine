/* LGPL-2.1-or-later. Tests the loaded runtime, including both public exports. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef void *(WINAPI *get_value_fn)(DWORD);
static get_value_fn get_value;
static DWORD slot;
static void *parent_fiber;
static LONG failures, callbacks;
static int main_value, thread_value, fiber_value;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); InterlockedIncrement(&failures); } } while (0)

static void check_value(DWORD index, void *expected)
{
    void *actual;
    DWORD error;
    SetLastError(0xdeadbeef);
    actual = get_value(index);
    error = GetLastError();
    CHECK(actual == expected, "FLS value");
    CHECK(error == 0xdeadbeef, "last error preserved");
}

static void WINAPI callback(void *value)
{
    CHECK(value == &main_value || value == &thread_value || value == &fiber_value, "callback value");
    InterlockedIncrement(&callbacks);
}

static DWORD WINAPI thread_proc(void *unused)
{
    (void)unused;
    check_value(slot, NULL);
    CHECK(FlsSetValue(slot, &thread_value), "thread write");
    check_value(slot, &thread_value);
    return 0;
}

static void WINAPI fiber_proc(void *unused)
{
    (void)unused;
    check_value(slot, NULL);
    CHECK(FlsSetValue(slot, &fiber_value), "fiber write");
    check_value(slot, &fiber_value);
    SwitchToFiber(parent_fiber);
}

int main(int argc, char **argv)
{
    const char *modules[] = {"kernelbase.dll", "kernel32.dll"};
    unsigned int i;
    const char *name = "FlsGetValue2";
    if (argc == 2 && !strcmp(argv[1], "--legacy-getter")) name = "FlsGetValue";
    else if (argc != 1) return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    for (i = 0; i < sizeof(modules) / sizeof(modules[0]); ++i)
    {
        HMODULE module = GetModuleHandleA(modules[i]);
        HANDLE thread;
        void *fiber;
        DWORD wait;
        get_value = (get_value_fn)(void *)GetProcAddress(module, name);
        printf("module=%s export=%s\n", modules[i], get_value ? "present" : "missing");
        CHECK(get_value != NULL, "FlsGetValue2 export");
        if (!get_value) continue;
        callbacks = 0;
        slot = FlsAlloc(callback);
        CHECK(slot != FLS_OUT_OF_INDEXES, "allocate slot");
        if (slot == FLS_OUT_OF_INDEXES) continue;
        check_value(slot, NULL);
        CHECK(FlsSetValue(slot, &main_value), "main write");
        check_value(slot, &main_value);
        check_value(FLS_OUT_OF_INDEXES, NULL);
        check_value(0, NULL);
        SetLastError(0xdeadbeef);
        CHECK(FlsGetValue(slot) == &main_value, "legacy value unchanged");
        CHECK(GetLastError() == ERROR_SUCCESS, "legacy last error unchanged");
        thread = CreateThread(NULL, 0, thread_proc, NULL, 0, NULL);
        CHECK(thread != NULL, "create thread");
        if (thread)
        {
            wait = WaitForSingleObject(thread, 10000);
            CHECK(wait == WAIT_OBJECT_0, "thread completed");
            if (wait != WAIT_OBJECT_0) return 2;
            CloseHandle(thread);
            CHECK(callbacks == 1, "thread cleanup");
            check_value(slot, &main_value);
        }
        parent_fiber = ConvertThreadToFiber(NULL);
        CHECK(parent_fiber != NULL, "convert main fiber");
        if (parent_fiber)
        {
            fiber = CreateFiber(0, fiber_proc, NULL);
            CHECK(fiber != NULL, "create fiber");
            if (fiber)
            {
                SwitchToFiber(fiber);
                check_value(slot, &main_value);
                DeleteFiber(fiber);
                CHECK(callbacks == 2, "fiber cleanup");
            }
            CHECK(ConvertFiberToThread(), "restore thread");
        }
        CHECK(FlsFree(slot), "free slot");
        CHECK(callbacks == 3, "main cleanup");
    }
    printf("failures=%ld\n", failures);
    return failures ? 1 : 0;
}
