/* Copyright 2026 Runeon Contributors. LGPL-2.1-or-later.
 * Checks that the media engine does not hold a lock of its
 * own while it calls IMFMediaEngineNotify::EventNotify() (0133).
 *
 * The main thread takes an application lock and sets the source of an audio-only
 * engine (a one-second silent WAV written to the temporary directory). The first
 * event that arrives on another thread signals the main thread and then waits for
 * the application lock. The main thread then has a helper thread call
 * GetCurrentTime(). If the engine holds its lock around the callback, the helper
 * cannot return while the main thread holds the application lock: DEADLOCK, exit
 * 1. If the helper returns within 5 s: OK, exit 0. If no event arrives on another
 * thread within 15 s the result is INCONCLUSIVE, exit 2. The lock is released at
 * the end in every case. Build with -lole32 -lmfplat -lmfuuid -luuid. */
#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <mfapi.h>
#include <mfmediaengine.h>
#include <stdio.h>

static CRITICAL_SECTION app_lock;
static HANDLE callback_waiting, helper_done;
static DWORD main_thread;
static LONG first_foreign = 1;
static IMFMediaEngine *engine;

static HRESULT WINAPI notify_QueryInterface(IMFMediaEngineNotify *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFMediaEngineNotify) || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI notify_AddRef(IMFMediaEngineNotify *iface) { return 2; }
static ULONG WINAPI notify_Release(IMFMediaEngineNotify *iface) { return 1; }

static HRESULT WINAPI notify_EventNotify(IMFMediaEngineNotify *iface, DWORD event, DWORD_PTR param1, DWORD param2)
{
    if (GetCurrentThreadId() != main_thread && InterlockedExchange(&first_foreign, 0))
    {
        printf("event %lu on thread %lu waits for the application lock\n", event, GetCurrentThreadId());
        SetEvent(callback_waiting);
        EnterCriticalSection(&app_lock);
        LeaveCriticalSection(&app_lock);
    }
    return S_OK;
}

static IMFMediaEngineNotifyVtbl notify_vtbl =
{
    notify_QueryInterface, notify_AddRef, notify_Release, notify_EventNotify,
};
static IMFMediaEngineNotify notify = { &notify_vtbl };

static DWORD WINAPI helper(void *arg)
{
    double time = IMFMediaEngine_GetCurrentTime(engine);
    (void)arg;
    printf("helper: GetCurrentTime returned %f\n", time);
    SetEvent(helper_done);
    return 0;
}

static BOOL write_silent_wav(const WCHAR *path)
{
    static const DWORD rate = 8000, seconds = 1;
    DWORD data = rate * seconds * 2, written;
    BYTE header[44] = {'R','I','F','F'};
    BYTE *samples;
    HANDLE file;
    BOOL ok;

    *(DWORD *)(header + 4) = 36 + data;
    memcpy(header + 8, "WAVEfmt ", 8);
    *(DWORD *)(header + 16) = 16;
    *(WORD *)(header + 20) = 1;           /* PCM */
    *(WORD *)(header + 22) = 1;           /* mono */
    *(DWORD *)(header + 24) = rate;
    *(DWORD *)(header + 28) = rate * 2;
    *(WORD *)(header + 32) = 2;
    *(WORD *)(header + 34) = 16;
    memcpy(header + 36, "data", 4);
    *(DWORD *)(header + 40) = data;
    if (!(samples = calloc(1, data))) return FALSE;
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok = file != INVALID_HANDLE_VALUE && WriteFile(file, header, sizeof(header), &written, NULL)
         && WriteFile(file, samples, data, &written, NULL);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    free(samples);
    return ok;
}

int main(void)
{
    IMFMediaEngineClassFactory *factory = NULL;
    IMFAttributes *attributes = NULL;
    WCHAR path[MAX_PATH];
    BSTR url;
    HANDLE thread;
    HRESULT hr;
    int result;

    setvbuf(stdout, NULL, _IONBF, 0);
    InitializeCriticalSection(&app_lock);
    callback_waiting = CreateEventW(NULL, TRUE, FALSE, NULL);
    helper_done = CreateEventW(NULL, TRUE, FALSE, NULL);
    main_thread = GetCurrentThreadId();

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"runeon-mfengine-probe.wav");
    if (!write_silent_wav(path))
    {
        printf("INCONCLUSIVE: cannot write %ls\n", path);
        return 2;
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION, MFSTARTUP_FULL);
    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IMFMediaEngineClassFactory, (void **)&factory);
    if (SUCCEEDED(hr)) hr = MFCreateAttributes(&attributes, 1);
    if (SUCCEEDED(hr)) hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)&notify);
    if (SUCCEEDED(hr)) hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_AUDIOONLY,
                                                                      attributes, &engine);
    if (FAILED(hr))
    {
        printf("INCONCLUSIVE: cannot create the media engine, hr %#lx\n", hr);
        return 2;
    }

    EnterCriticalSection(&app_lock);
    url = SysAllocString(path);
    hr = IMFMediaEngine_SetSource(engine, url);
    printf("main holds the application lock; SetSource hr %#lx\n", hr);

    if (WaitForSingleObject(callback_waiting, 15000))
    {
        printf("INCONCLUSIVE: no event on another thread within 15 s\n");
        result = 2;
    }
    else
    {
        thread = CreateThread(NULL, 0, helper, NULL, 0, NULL);
        if (WaitForSingleObject(helper_done, 5000))
        {
            printf("DEADLOCK: GetCurrentTime waits while the callback waits for the application lock\n");
            result = 1;
        }
        else
        {
            printf("OK: an engine call does not wait for the application's callback\n");
            result = 0;
        }
        LeaveCriticalSection(&app_lock);
        WaitForSingleObject(thread, 10000);
        CloseHandle(thread);
        goto shutdown;
    }
    LeaveCriticalSection(&app_lock);

shutdown:
    IMFMediaEngine_Shutdown(engine);
    IMFMediaEngine_Release(engine);
    IMFAttributes_Release(attributes);
    IMFMediaEngineClassFactory_Release(factory);
    SysFreeString(url);
    MFShutdown();
    DeleteFileW(path);
    printf("exit %d\n", result);
    return result;
}
