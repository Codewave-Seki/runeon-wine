/* Test the production policy with deterministic device enumeration at the OS boundary. */
#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <dxgi1_6.h>
#include <setupapi.h>
#include <devpropdef.h>
#include <assert.h>
#include <stdio.h>

static unsigned int device_count = 1, query_count;
static LUID device_luid = {0x1234, 0};
static UINT reported_vendor = 0x1002;
static HDEVINFO WINAPI test_devices(const GUID *guid, const WCHAR *enumerator, HWND window, DWORD flags)
{
    ++query_count;
    return (HDEVINFO)(ULONG_PTR)1;
}
static BOOL WINAPI test_enum(HDEVINFO devices, DWORD index, SP_DEVINFO_DATA *device)
{
    if (index < device_count) { device->Reserved = index; return TRUE; }
    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}
static BOOL WINAPI test_property(HDEVINFO devices, SP_DEVINFO_DATA *device, const DEVPROPKEY *key,
        DEVPROPTYPE *type, BYTE *buffer, DWORD size, DWORD *required, DWORD flags)
{
    assert(size >= sizeof(device_luid));
    memcpy(buffer, &device_luid, sizeof(device_luid));
    *type = DEVPROP_TYPE_UINT64;
    *required = sizeof(device_luid);
    return TRUE;
}
static BOOL WINAPI test_destroy(HDEVINFO devices) { return TRUE; }
static DWORD first_protection;
static BOOL WINAPI test_protect(void *address, SIZE_T size, DWORD protection, DWORD *old)
{
    if (!first_protection) first_protection = protection;
    return VirtualProtect(address, size, protection, old);
}
#define SetupDiGetClassDevsW test_devices
#define SetupDiEnumDeviceInfo test_enum
#define SetupDiGetDevicePropertyW test_property
#define SetupDiDestroyDeviceInfoList test_destroy
#define VirtualProtect test_protect
#include "driver.c"
#undef VirtualProtect

static HRESULT original_result, description_result;
static LONGLONG original_version;
static HRESULT WINAPI fake_check(IDXGIAdapter *adapter, REFIID iid, LARGE_INTEGER *value)
{
    if (value) value->QuadPart = original_version;
    SetLastError(0x4321);
    return original_result;
}
static HRESULT WINAPI description(IDXGIAdapter *adapter, DXGI_ADAPTER_DESC *desc)
{
    desc->AdapterLuid = device_luid;
    desc->VendorId = reported_vendor;
    SetLastError(0x9876);
    return description_result;
}
int main(void)
{
    LARGE_INTEGER version;
    IDXGIAdapterVtbl *table = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    IDXGIAdapter adapter = {table};
    MEMORY_BASIC_INFORMATION memory;
    DWORD old;
    table->CheckInterfaceSupport = fake_check;
    table->GetDesc = description;
    assert(VirtualProtect(table, 4096, PAGE_EXECUTE_READ, &old));
    patch_adapter(&adapter);
    patch_adapter(&adapter);
    assert(table_count == 1 && first_protection == PAGE_EXECUTE_READWRITE);
    assert(VirtualQuery(table, &memory, sizeof(memory)) && memory.Protect == PAGE_EXECUTE_READ);
    original_result = S_OK;
    description_result = S_OK;
    original_version = -1;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK);
    assert(version.QuadPart == 0x0023000052210400LL && GetLastError() == 0x4321 && query_count == 1);
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && query_count == 1);
    /* Same LUID with a different reported vendor must not reuse the AMD value. */
    reported_vendor = 0x8086;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == 0x00230000006518aaLL);
    reported_vendor = 0x10de;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == 0x00230000000f17ceLL);
    reported_vendor = 0x106b;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == -1 && query_count == 3);
    reported_vendor = 0xffff;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == -1 && query_count == 3);
    reported_vendor = 0x1002;
    version_count = 0;
    device_count = 2;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == -1);
    version_count = 0;
    device_count = 0;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == -1);
    original_version = 0x0001000200030004LL;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == original_version);
    original_version = -1;
    original_result = DXGI_ERROR_UNSUPPORTED;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == DXGI_ERROR_UNSUPPORTED && version.QuadPart == -1);
    original_result = S_OK;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, NULL) == S_OK && GetLastError() == 0x4321);
    description_result = E_FAIL;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && version.QuadPart == -1);
    puts("PASS: AMD/Intel/NVIDIA vendor versions, unknown vendor, LUID/vendor cache, duplicate/missing identity, normal/failure/null, last error, executable protection");
    return 0;
}
