/* Copyright 2026 Runeon Contributors. LGPL-2.1-or-later.
 * Checks the order of media engine events raised inside a callback (0133).
 *
 * A paused audio-only engine with a loaded one-second silent WAV raises SEEKING,
 * SEEKED and TIMEUPDATE in one SetCurrentTime() call. The SEEKING callback calls
 * SetMuted(TRUE), which raises VOLUMECHANGE. Delivered as each event is raised,
 * the main thread sees SEEKING, VOLUMECHANGE, SEEKED, TIMEUPDATE, and all of them
 * before SetCurrentTime() returns. Exit 0 on that order, 1 on another, 2 when the
 * media does not load within 15 s. Build with -lole32 -loleaut32 -lmfplat -lmfuuid -luuid. */
#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <mfapi.h>
#include <mfmediaengine.h>
#include <stdio.h>

static IMFMediaEngine *engine;
static HANDLE can_play;
static DWORD main_thread;
static volatile LONG recording;
static DWORD order[16];
static LONG count;

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
    if (event == MF_MEDIA_ENGINE_EVENT_CANPLAY) SetEvent(can_play);
    if (!recording || GetCurrentThreadId() != main_thread) return S_OK;
    if (count < (LONG)ARRAYSIZE(order)) order[count++] = event;
    if (event == MF_MEDIA_ENGINE_EVENT_SEEKING) IMFMediaEngine_SetMuted(engine, TRUE);
    return S_OK;
}

static IMFMediaEngineNotifyVtbl notify_vtbl =
{
    notify_QueryInterface, notify_AddRef, notify_Release, notify_EventNotify,
};
static IMFMediaEngineNotify notify = { &notify_vtbl };

static BOOL write_silent_wav(const WCHAR *path)
{
    static const DWORD rate = 8000;
    DWORD data = rate * 2, written;
    BYTE header[44] = {'R','I','F','F'};
    BYTE *samples;
    HANDLE file;
    BOOL ok;

    *(DWORD *)(header + 4) = 36 + data;
    memcpy(header + 8, "WAVEfmt ", 8);
    *(DWORD *)(header + 16) = 16;
    *(WORD *)(header + 20) = 1;
    *(WORD *)(header + 22) = 1;
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
    static const DWORD expected[] = {MF_MEDIA_ENGINE_EVENT_SEEKING, MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE,
                                     MF_MEDIA_ENGINE_EVENT_SEEKED, MF_MEDIA_ENGINE_EVENT_TIMEUPDATE};
    IMFMediaEngineClassFactory *factory = NULL;
    IMFAttributes *attributes = NULL;
    WCHAR path[MAX_PATH];
    int result = 0;
    LONG i, seen;
    BSTR url;
    HRESULT hr;

    setvbuf(stdout, NULL, _IONBF, 0);
    can_play = CreateEventW(NULL, TRUE, FALSE, NULL);
    main_thread = GetCurrentThreadId();
    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"runeon-mfengine-order.wav");
    if (!write_silent_wav(path)) return 2;

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
    url = SysAllocString(path);
    IMFMediaEngine_SetSource(engine, url);
    if (WaitForSingleObject(can_play, 15000))
    {
        printf("INCONCLUSIVE: the media did not load within 15 s\n");
        result = 2;
        goto done;
    }

    recording = 1;
    hr = IMFMediaEngine_SetCurrentTime(engine, 0.5);
    seen = count;
    recording = 0;
    printf("SetCurrentTime hr %#lx; events during the call:", hr);
    for (i = 0; i < seen; i++) printf(" %lu", order[i]);
    printf("\n");
    if (seen != ARRAYSIZE(expected)) result = 1;
    for (i = 0; !result && i < seen; i++) if (order[i] != expected[i]) result = 1;
    printf(result ? "WRONG ORDER: expected 16 (seeking) 22 (volumechange) 17 (seeked) 18 (timeupdate)\n"
                  : "OK: events raised inside the callback come before the rest of the call's events\n");

done:
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
