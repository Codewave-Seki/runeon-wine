/* Exercise action-map app data through the real DirectInput API. */
#define COBJMACROS
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <stdio.h>
#include <string.h>
#ifndef DIVIRTUAL_DRIVING_RACE
#define DIVIRTUAL_DRIVING_RACE 0x01000000
#endif

static const GUID probe_map = {0x399e4d82,0xdc60,0x4795,{0x8a,0x1a,0x42,0xe0,0x29,0xa1,0xf5,0x64}};

static int check_map(IDirectInputDevice8W *mouse, IDirectInputDevice8W *keyboard, int reverse, int legacy)
{
    DIACTIONW actions[2] = {0};
    DIACTIONFORMATW format = {0};
    DIPROPPOINTER prop = {{sizeof(prop), sizeof(prop.diph), DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(3), DIPH_BYID}, 0};
    IDirectInputDevice8W *devices[] = {mouse, keyboard};
    const char *names[] = {"mouse", "keyboard"};
    ULONG_PTR values[] = {17, 29};
    HRESULT hr;
    unsigned int i, slot;
    int failed = 0;

    for (i = 0; i < 2; ++i)
    {
        slot = reverse ? 1 - i : i;
        actions[slot].uAppData = values[i];
        actions[slot].dwSemantic = i ? DIKEYBOARD_2 : DIMOUSE_BUTTON0;
        actions[slot].lptszActionName = i ? L"Keyboard key" : L"Mouse button";
        actions[slot].guidInstance = i ? GUID_SysKeyboard : GUID_SysMouse;
        actions[slot].dwObjID = prop.diph.dwObj;
        actions[slot].dwFlags = DIA_APPMAPPED | DIA_APPFIXED;
        actions[slot].dwHow = DIAH_DEFAULT;
    }
    format.dwSize = sizeof(format);
    format.dwActionSize = sizeof(actions[0]);
    format.dwNumActions = 2;
    format.dwDataSize = 2 * sizeof(DWORD);
    format.rgoAction = actions;
    format.guidActionMap = probe_map;
    format.dwGenre = DIVIRTUAL_DRIVING_RACE;
    format.dwBufferSize = 32;
    lstrcpyW(format.tszActionMap, L"Runeon GUID regression probe");

    for (i = 0; i < 2; ++i)
    {
        ULONG_PTR expected = legacy ? values[reverse ? 0 : 1] : values[i];
        hr = IDirectInputDevice8_SetActionMap(devices[i], &format, L"runeon-guid-probe", DIDSAM_DEFAULT);
        if (FAILED(hr))
        {
            printf("{\"device\":\"%s\",\"reverse\":%d,\"error\":\"SetActionMap\",\"hr\":\"%08lx\"}\n", names[i], reverse, (unsigned long)hr);
            ++failed;
            continue;
        }
        prop.uData = 0;
        hr = IDirectInputDevice8_GetProperty(devices[i], DIPROP_APPDATA, &prop.diph);
        printf("{\"device\":\"%s\",\"reverse\":%d,\"hr\":\"%08lx\",\"actual\":%llu,\"expected\":%llu}\n",
               names[i], reverse, (unsigned long)hr, (unsigned long long)prop.uData, (unsigned long long)expected);
        if (hr != DI_OK || prop.uData != expected) ++failed;
    }
    return failed;
}

int main(int argc, char **argv)
{
    IDirectInput8W *input = NULL;
    IDirectInputDevice8W *mouse = NULL, *keyboard = NULL;
    char module_path[MAX_PATH];
    HRESULT hr;
    int failures = 0;
    int legacy = argc == 2 && !strcmp(argv[1], "--expect-legacy");

    if (argc > 1 && !legacy) return 2;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return 2;
    hr = DirectInput8Create(GetModuleHandleW(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void **)&input, NULL);
    if (FAILED(hr)) { printf("DirectInput8Create failed: %08lx\n", (unsigned long)hr); failures = 2; goto done; }
    if (GetModuleFileNameA(GetModuleHandleW(L"dinput8.dll"), module_path, sizeof(module_path)))
        printf("dinput8_module=%s\n", module_path);
    hr = IDirectInput8_CreateDevice(input, &GUID_SysMouse, &mouse, NULL);
    if (FAILED(hr)) { printf("CreateDevice(mouse) failed: %08lx\n", (unsigned long)hr); failures = 2; goto done; }
    hr = IDirectInput8_CreateDevice(input, &GUID_SysKeyboard, &keyboard, NULL);
    if (FAILED(hr)) { printf("CreateDevice(keyboard) failed: %08lx\n", (unsigned long)hr); failures = 2; goto done; }
    failures += check_map(mouse, keyboard, 0, legacy);
    failures += check_map(mouse, keyboard, 1, legacy);
    printf("{\"mode\":\"%s\",\"checks\":4,\"failures\":%d}\n", legacy ? "legacy" : "fixed", failures);
done:
    if (keyboard) IDirectInputDevice8_Release(keyboard);
    if (mouse) IDirectInputDevice8_Release(mouse);
    if (input) IDirectInput8_Release(input);
    CoUninitialize();
    return failures ? 1 : 0;
}
