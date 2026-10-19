/* LGPL-2.1-or-later. Starts a shared-mode WASAPI render stream on the default
 * device with the default period for one second of silence (0129). Run it with
 * WINEDEBUG=trace+coreaudio: the patched driver logs "I/O buffer <old> -> <new>
 * frames at <rate> Hz" when it lowers the device pull below half the period. This
 * probe only creates the stream; it does not measure dropouts. */
#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>

int main(void)
{
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioClient *client = NULL;
    WAVEFORMATEX *format = NULL;
    REFERENCE_TIME period = 0;
    HRESULT hr;

    setvbuf(stdout, NULL, _IONBF, 0);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator,
                          (void **)&enumerator);
    if (SUCCEEDED(hr)) hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device);
    if (SUCCEEDED(hr)) hr = IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void **)&client);
    if (SUCCEEDED(hr)) hr = IAudioClient_GetMixFormat(client, &format);
    if (SUCCEEDED(hr)) hr = IAudioClient_GetDevicePeriod(client, &period, NULL);
    if (SUCCEEDED(hr)) hr = IAudioClient_Initialize(client, AUDCLNT_SHAREMODE_SHARED, 0, period * 2, 0, format, NULL);
    if (SUCCEEDED(hr)) hr = IAudioClient_Start(client);
    printf("rate=%lu channels=%u period=%lld hr=%#lx\n", format ? format->nSamplesPerSec : 0,
           format ? format->nChannels : 0, (long long)period, hr);
    if (SUCCEEDED(hr))
    {
        Sleep(1000);
        IAudioClient_Stop(client);
    }
    if (client) IAudioClient_Release(client);
    if (device) IMMDevice_Release(device);
    if (enumerator) IMMDeviceEnumerator_Release(enumerator);
    CoTaskMemFree(format);
    printf(SUCCEEDED(hr) ? "RESULT: STARTED\n" : "RESULT: FAIL\n");
    return SUCCEEDED(hr) ? 0 : 1;
}
