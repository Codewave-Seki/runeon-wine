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
static const WCHAR *driver_text = L"35.0.10.1000";
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
static HKEY WINAPI test_key(HDEVINFO devices, SP_DEVINFO_DATA *device, DWORD scope,
        DWORD profile, DWORD type, REGSAM access) { return (HKEY)(ULONG_PTR)1; }
static LSTATUS WINAPI test_value(HKEY key, const WCHAR *subkey, const WCHAR *name,
        DWORD flags, DWORD *type, void *data, DWORD *size)
{
    DWORD required = (lstrlenW(driver_text) + 1) * sizeof(WCHAR);
    assert(*size >= required);
    memcpy(data, driver_text, required);
    *size = required;
    return ERROR_SUCCESS;
}
static LSTATUS WINAPI test_close(HKEY key) { return ERROR_SUCCESS; }
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
#define SetupDiOpenDevRegKey test_key
#define RegGetValueW test_value
#define RegCloseKey test_close
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
    SetLastError(0x9876);
    return description_result;
}
int main(void)
{
    LARGE_INTEGER version;
    const WCHAR *invalid[] = {L"",L"1.2.3",L"1.2.3.4.5",L"1.2.3.65536",L"-1.2.3.4",
        L"1.2.3.4 ",L"1..3.4",L"0000001.2.3.4",L"0.0.0.0",L"65535.65535.65535.65535"};
    IDXGIAdapterVtbl *table = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    IDXGIAdapter adapter = {table};
    MEMORY_BASIC_INFORMATION memory;
    DWORD old;
    unsigned int i;
    assert(parse_version(L"35.0.10.1000", &version) && version.QuadPart == 0x00230000000a03e8LL);
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) assert(!parse_version(invalid[i], &version));
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
    assert(version.QuadPart == 0x00230000000a03e8LL && GetLastError() == 0x4321 && query_count == 1);
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK && query_count == 1);
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
    puts("PASS: unique replacement/cache, duplicate/missing identity, normal/failure/null, last error, executable protection");
    return 0;
}
