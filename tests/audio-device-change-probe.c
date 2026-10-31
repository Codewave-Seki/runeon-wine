/* LGPL-2.1-or-later. Audio device change probe for 0154.
 * Wine listed the audio devices once, when mmdevapi loaded, so a program
 * never heard about headphones connected or removed while it ran, and kept
 * playing to a device that was gone. The probe registers an
 * IMMNotificationClient, plays silence on the default render device, and for
 * the given number of seconds prints every notification and every change of
 * the stream's GetCurrentPadding result. Connect or remove headphones, or
 * change the Mac's output device, while it runs.
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror -o audio-device-change-probe64.exe audio-device-change-probe.c -lole32 -luuid
 * Usage: audio-device-change-probe64.exe [seconds]   (default 60)
 * Prints "notifications: N" at the end. It does not reopen the default device
 * itself, and does not cover capture streams. */
#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <stdlib.h>

static LONG notifications;

static HRESULT WINAPI client_QueryInterface(IMMNotificationClient *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMMNotificationClient))
    {
        *out = iface;
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI client_AddRef(IMMNotificationClient *iface) { return 2; }
static ULONG WINAPI client_Release(IMMNotificationClient *iface) { return 1; }

static HRESULT WINAPI client_OnDeviceStateChanged(IMMNotificationClient *iface, const WCHAR *id, DWORD state)
{
    InterlockedIncrement(&notifications);
    printf("OnDeviceStateChanged %ls state %#lx\n", id, state);
    return S_OK;
}

static HRESULT WINAPI client_OnDeviceAdded(IMMNotificationClient *iface, const WCHAR *id)
{
    InterlockedIncrement(&notifications);
    printf("OnDeviceAdded %ls\n", id);
    return S_OK;
}

static HRESULT WINAPI client_OnDeviceRemoved(IMMNotificationClient *iface, const WCHAR *id)
{
    InterlockedIncrement(&notifications);
    printf("OnDeviceRemoved %ls\n", id);
    return S_OK;
}

static HRESULT WINAPI client_OnDefaultDeviceChanged(IMMNotificationClient *iface, EDataFlow flow, ERole role,
                                                    const WCHAR *id)
{
    InterlockedIncrement(&notifications);
    printf("OnDefaultDeviceChanged flow %d role %d %ls\n", flow, role, id ? id : L"(none)");
    return S_OK;
}

static HRESULT WINAPI client_OnPropertyValueChanged(IMMNotificationClient *iface, const WCHAR *id,
                                                    const PROPERTYKEY key)
{
    return S_OK;
}

static IMMNotificationClientVtbl client_vtbl =
{
    client_QueryInterface, client_AddRef, client_Release, client_OnDeviceStateChanged,
    client_OnDeviceAdded, client_OnDeviceRemoved, client_OnDefaultDeviceChanged,
    client_OnPropertyValueChanged,
};

int main(int argc, char **argv)
{
    IMMNotificationClient client = { &client_vtbl };
    IMMDeviceEnumerator *enumerator;
    IAudioClient *audio = NULL;
    IMMDevice *device;
    WAVEFORMATEX *format;
    HRESULT hr, last = S_OK;
    UINT32 padding;
    int seconds = argc > 1 ? atoi(argv[1]) : 60, i;

    setvbuf(stdout, NULL, _IONBF, 0);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator,
                          (void **)&enumerator);
    if (FAILED(hr)) return 2;
    IMMDeviceEnumerator_RegisterEndpointNotificationCallback(enumerator, &client);

    if (SUCCEEDED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device)) &&
        SUCCEEDED(IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void **)&audio)) &&
        SUCCEEDED(IAudioClient_GetMixFormat(audio, &format)) &&
        SUCCEEDED(IAudioClient_Initialize(audio, AUDCLNT_SHAREMODE_SHARED, 0, 2000000, 0, format, NULL)))
        IAudioClient_Start(audio);
    printf("listening for %d seconds\n", seconds);

    for (i = 0; i < seconds * 10; i++)
    {
        if (audio)
        {
            hr = IAudioClient_GetCurrentPadding(audio, &padding);
            if (hr != last) printf("GetCurrentPadding now returns %#lx\n", hr);
            last = hr;
        }
        Sleep(100);
    }

    IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(enumerator, &client);
    printf("notifications: %ld\n", notifications);
    return 0;
}
