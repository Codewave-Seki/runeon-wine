/* Standalone check of the actual module policy, run inside an isolated prefix. */
#include "driver.c"
#include <stdio.h>
#include <assert.h>

static HRESULT original_result;
static LONGLONG original_version;
static HRESULT WINAPI fake_check(IDXGIAdapter *adapter, REFIID iid, LARGE_INTEGER *value)
{
    (void)adapter; (void)iid;
    if (value) value->QuadPart = original_version;
    SetLastError(0x4321);
    return original_result;
}
static HRESULT WINAPI no_description(IDXGIAdapter *adapter, DXGI_ADAPTER_DESC *desc)
{
    (void)adapter; (void)desc;
    SetLastError(0x9876);
    return E_FAIL;
}
int main(void)
{
    LARGE_INTEGER version;
    const WCHAR *invalid[] = {L"",L"1.2.3",L"1.2.3.4.5",L"1.2.3.65536",L"-1.2.3.4",
        L"1.2.3.4 ",L"1..3.4",L"0000001.2.3.4",L"0.0.0.0",L"65535.65535.65535.65535"};
    IDXGIAdapterVtbl table = {0};
    IDXGIAdapter adapter = {&table};
    unsigned int i;
    assert(parse_version(L"35.0.10.1000", &version) && version.QuadPart == 0x00230000000a03e8LL);
    assert(parse_version(L"1.2.3.65535", &version) && version.QuadPart == 0x000100020003ffffLL);
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) assert(!parse_version(invalid[i], &version));
    table.CheckInterfaceSupport = fake_check;
    table.GetDesc = no_description;
    patch_adapter(&adapter);
    patch_adapter(&adapter);
    assert(table_count == 1);
    original_result = S_OK;
    original_version = 0x0001000200030004LL;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK);
    assert(version.QuadPart == original_version && GetLastError() == 0x4321);
    original_version = -1;
    original_result = DXGI_ERROR_UNSUPPORTED;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == DXGI_ERROR_UNSUPPORTED);
    assert(version.QuadPart == -1 && GetLastError() == 0x4321);
    original_result = S_OK;
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, &version) == S_OK);
    assert(version.QuadPart == -1 && GetLastError() == 0x4321);
    assert(IDXGIAdapter_CheckInterfaceSupport(&adapter, &IID_IDXGIDevice, NULL) == S_OK);
    assert(!device_version((LUID){0xdeadbeef, 0x12345678}, &version));
    puts("PASS: parsing, unchanged valid/failure/null, missing identity, last error, repeated hook");
    return 0;
}
