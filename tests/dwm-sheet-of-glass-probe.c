/*
 * Show two borderless topmost D3D11 windows whose content has known alpha,
 * for checking DwmExtendFrameIntoClientArea() "sheet of glass" composition.
 *
 * Both windows present the same 400x400 image in four quadrants:
 *   top-left     alpha 0     (black, fully transparent under glass)
 *   top-right    alpha 0.5   (red, premultiplied: 0.5, 0, 0, 0.5)
 *   bottom-left  alpha 1     (green)
 *   bottom-right alpha 1     (black, must stay black)
 * The left window calls DwmExtendFrameIntoClientArea() with all margins -1;
 * the right window never does and is the control. After the glass phase the
 * left window resets its margins to 0 and must turn opaque again.
 *
 * The probe only presents frames and prints window rectangles and phase
 * markers; checking what the desktop shows is up to the caller (screenshot).
 * It does not cover partial margins, GDI-drawn content, i386, or other
 * swap effects than flip-discard.
 *
 * Build: x86_64-w64-mingw32-gcc -O1 -o dwm-sheet-of-glass-probe.exe
 *        dwm-sheet-of-glass-probe.c -ld3d11 -ldxgi -ldwmapi -luser32 -ldxguid
 * Usage: wine dwm-sheet-of-glass-probe.exe [early|late|clickthrough|reparent] [seconds per phase]
 *        early: glass before the swap chain exists; late (default): after.
 *        clickthrough: with glass on, the left window switches between
 *        WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST and WS_EX_TOPMOST,
 *        setting each value repeatedly as overlays do every frame. It prints
 *        what WindowFromPoint() returns at the window centre; whether the
 *        Cocoa window ignores the mouse is up to the caller (+macdrv trace or
 *        a native hit test at the printed centre).
 *        reparent: a 200x200 child window showing only the alpha-0 quadrant
 *        starts over the control window's green quadrant (must look black), is
 *        moved with SetParent() over the glass window's alpha-0 quadrant (must
 *        show the desktop), then back over the control's green quadrant (must
 *        look black again; green would mean its layer stayed non-opaque).
 * Exit:  0 when every call succeeded, 2 on setup failure.
 */

#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 400

struct target
{
    HWND hwnd;
    UINT size;
    IDXGISwapChain *swapchain;
    ID3D11Texture2D *backbuffer;
};

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_ERASEBKGND) return 1; /* leave the GDI surface untouched */
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static HRESULT glass(HWND hwnd, int on)
{
    MARGINS margins = {on ? -1 : 0, on ? -1 : 0, on ? -1 : 0, on ? -1 : 0};
    HRESULT hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    printf("DwmExtendFrameIntoClientArea(%p, %d) hr %#lx\n", hwnd, on ? -1 : 0, hr);
    return hr;
}

static int create_swapchain(ID3D11Device *device, struct target *t)
{
    DXGI_SWAP_CHAIN_DESC desc = {0};
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    desc.BufferDesc.Width = t->size;
    desc.BufferDesc.Height = t->size;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.OutputWindow = t->hwnd;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &desc, &t->swapchain);
    IDXGIFactory_Release(factory);
    IDXGIAdapter_Release(adapter);
    IDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        printf("CreateSwapChain hr %#lx\n", hr);
        return 0;
    }
    IDXGISwapChain_GetBuffer(t->swapchain, 0, &IID_ID3D11Texture2D, (void **)&t->backbuffer);
    return 1;
}

static void set_ex_style(HWND hwnd, LONG ex_style, int repeat)
{
    POINT centre = {100 + SIZE / 2, 100 + SIZE / 2};

    for (int i = 0; i < repeat; i++) SetWindowLongW(hwnd, GWL_EXSTYLE, ex_style);
    printf("ex_style %#lx set %d times, WindowFromPoint(%ld,%ld) %s\n", GetWindowLongW(hwnd, GWL_EXSTYLE),
           repeat, centre.x, centre.y, WindowFromPoint(centre) == hwnd ? "window" : "not window");
}

static void present(ID3D11DeviceContext *context, ID3D11Texture2D *image, struct target *t, DWORD ms)
{
    DWORD end = GetTickCount() + ms;
    D3D11_BOX box = {0, 0, 0, SIZE, SIZE, 1};
    MSG msg;

    while ((LONG)(end - GetTickCount()) > 0)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
        for (int i = 0; i < 3 && t[i].swapchain; i++)
        {
            /* the top-left corner of the image: the whole of it for the top-level
               windows, only the alpha-0 quadrant for the smaller child */
            box.right = box.bottom = t[i].size;
            ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)t[i].backbuffer, 0, 0, 0, 0,
                                                      (ID3D11Resource *)image, 0, &box);
            IDXGISwapChain_Present(t[i].swapchain, 1, 0);
        }
    }
}

static void move_child(HWND child, HWND parent, int y, const char *name)
{
    SetParent(child, parent);
    SetWindowPos(child, HWND_TOP, 0, y, SIZE / 2, SIZE / 2, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    printf("phase child in %s start\n", name);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    static UINT32 pixels[SIZE * SIZE];
    int early = argc > 1 && !strcmp(argv[1], "early");
    int clickthrough = argc > 1 && !strcmp(argv[1], "clickthrough");
    int reparent = argc > 1 && !strcmp(argv[1], "reparent");
    DWORD phase_ms = (argc > 2 ? atoi(argv[2]) : 10) * 1000;
    D3D11_SUBRESOURCE_DATA data = {pixels, SIZE * 4, 0};
    D3D11_TEXTURE2D_DESC desc = {0};
    struct target t[3] = {{0}};
    ID3D11DeviceContext *context;
    ID3D11Texture2D *image;
    ID3D11Device *device;
    WNDCLASSW wc = {0};
    HRESULT hr;
    int ok = 1;

    SetProcessDPIAware();
    for (int y = 0; y < SIZE; y++)
        for (int x = 0; x < SIZE; x++)
        {
            int right = x >= SIZE / 2, bottom = y >= SIZE / 2;
            /* B8G8R8A8: 0xAARRGGBB */
            pixels[y * SIZE + x] = !bottom ? (right ? 0x80800000 : 0x00000000)
                                           : (right ? 0xff000000 : 0xff00ff00);
        }

    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"dwm_glass_probe";
    wc.hCursor = LoadCursorW(NULL, (const WCHAR *)IDC_ARROW);
    RegisterClassW(&wc);
    for (int i = 0; i < 2; i++)
    {
        RECT rect;
        t[i].size = SIZE;
        t[i].hwnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, i ? L"control" : L"glass", WS_POPUP,
                                    100 + i * 500, 100, SIZE, SIZE, NULL, NULL, wc.hInstance, NULL);
        if (!t[i].hwnd) return 2;
        GetWindowRect(t[i].hwnd, &rect);
        printf("window %s %p rect %ld,%ld-%ld,%ld\n", i ? "control" : "glass", t[i].hwnd,
               rect.left, rect.top, rect.right, rect.bottom);
    }

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
                           &device, NULL, &context);
    if (FAILED(hr))
    {
        printf("D3D11CreateDevice hr %#lx\n", hr);
        return 2;
    }
    desc.Width = desc.Height = SIZE;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &desc, &data, &image)))
    {
        printf("CreateTexture2D hr %#lx\n", hr);
        return 2;
    }

    if (early && FAILED(glass(t[0].hwnd, 1))) ok = 0;
    for (int i = 0; i < 2; i++)
    {
        ShowWindow(t[i].hwnd, SW_SHOWNOACTIVATE);
        if (!create_swapchain(device, &t[i])) return 2;
    }
    if (reparent)
    {
        t[2].size = SIZE / 2;
        t[2].hwnd = CreateWindowExW(0, wc.lpszClassName, L"child", WS_CHILD | WS_VISIBLE, 0, SIZE / 2,
                                    SIZE / 2, SIZE / 2, t[1].hwnd, NULL, wc.hInstance, NULL);
        if (!t[2].hwnd || !create_swapchain(device, &t[2])) return 2;
    }
    present(context, image, t, 500);
    if (!early && FAILED(glass(t[0].hwnd, 1))) ok = 0;

    printf("phase glass start\n");
    fflush(stdout);
    present(context, image, t, phase_ms);
    for (int round = 0; clickthrough && round < 2; round++)
    {
        set_ex_style(t[0].hwnd, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, 20);
        printf("phase clickthrough start\n");
        fflush(stdout);
        present(context, image, t, phase_ms);
        set_ex_style(t[0].hwnd, WS_EX_TOPMOST, 20);
        printf("phase hit start\n");
        fflush(stdout);
        present(context, image, t, phase_ms);
    }
    if (reparent)
    {
        move_child(t[2].hwnd, t[0].hwnd, 0, "glass");
        present(context, image, t, phase_ms);
        move_child(t[2].hwnd, t[1].hwnd, SIZE / 2, "control");
        present(context, image, t, phase_ms);
    }
    if (FAILED(glass(t[0].hwnd, 0))) ok = 0;
    printf("phase reset start\n");
    fflush(stdout);
    present(context, image, t, phase_ms);
    printf("done\n");
    fflush(stdout);

    for (int i = 0; i < 3 && t[i].swapchain; i++)
    {
        ID3D11Texture2D_Release(t[i].backbuffer);
        IDXGISwapChain_Release(t[i].swapchain);
        DestroyWindow(t[i].hwnd);
    }
    ID3D11Texture2D_Release(image);
    ID3D11DeviceContext_Release(context);
    ID3D11Device_Release(device);
    return ok ? 0 : 2;
}
