/*
 * Independent API regression probes for the Runeon Wine .9 candidate.
 * The JPEG fixture is from Wine dlls/windowscodecs/tests/metadata.c:
 * Copyright 2011 Vincent Povirk for CodeWeavers
 * Copyright 2012,2017 Dmitry Timoshkov
 * This file is licensed under LGPL-2.1-or-later.
 */
#define COBJMACROS
#define CONST_VTABLE
#define SECURITY_WIN32
#include <windows.h>
#include <security.h>
#include <schannel.h>
#include <initguid.h>
#include <d3d10_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned failures, checks;
#define CHECK(c, ...) do { ++checks; if (!(c)) { ++failures; printf("FAIL: "); } else printf("PASS: "); printf(__VA_ARGS__); printf("\n"); } while (0)

static void check_bad_context(CtxtHandle context, const char *kind)
{
    SecBufferDesc empty = {SECBUFFER_VERSION, 0, NULL};
    SECURITY_STATUS status;
    status = EncryptMessage(&context, 0, &empty, 0);
    CHECK(status == SEC_E_INVALID_HANDLE, "SChannel %s EncryptMessage status=%08lx expected=%08lx", kind, status, (ULONG)SEC_E_INVALID_HANDLE);
    status = DecryptMessage(&context, &empty, 0, NULL);
    CHECK(status == SEC_E_INVALID_HANDLE, "SChannel %s DecryptMessage status=%08lx expected=%08lx", kind, status, (ULONG)SEC_E_INVALID_HANDLE);
}

static void test_schannel(void)
{
    SCHANNEL_CRED config = {0};
    CredHandle credential;
    CtxtHandle context, bad;
    SECURITY_STATUS status;
    ULONG attributes;
    char token[8192];
    SecBuffer output = {sizeof(token), SECBUFFER_TOKEN, token};
    SecBufferDesc outputs = {SECBUFFER_VERSION, 1, &output};

    config.dwVersion = SCHANNEL_CRED_VERSION;
    config.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
    config.dwFlags = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION;
    status = AcquireCredentialsHandleA(NULL, UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
                                      &config, NULL, NULL, &credential, NULL);
    CHECK(status == SEC_E_OK, "SChannel acquire outbound credentials status=%08lx", status);
    if (status != SEC_E_OK) return;

    /* Preserve a real provider pointer so the wrapper cannot reject the handle first. */
    bad = credential;
    check_bad_context(bad, "credential used as context");
    bad.dwLower = ~(ULONG_PTR)0;
    check_bad_context(bad, "invalid object index");

    status = InitializeSecurityContextA(&credential, NULL, "localhost", ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM,
        0, SECURITY_NATIVE_DREP, NULL, 0, &context, &outputs, &attributes, NULL);
    CHECK(status == SEC_I_CONTINUE_NEEDED || status == SEC_E_OK,
          "SChannel create context without a network connection status=%08lx", status);
    if (status == SEC_I_CONTINUE_NEEDED || status == SEC_E_OK)
    {
        bad = context;
        status = DeleteSecurityContext(&context);
        CHECK(status == SEC_E_OK, "SChannel delete context status=%08lx", status);
        check_bad_context(bad, "deleted context");
    }
    status = FreeCredentialsHandle(&credential);
    CHECK(status == SEC_E_OK, "SChannel free credentials status=%08lx", status);
}

struct failing_adapter { IDXGIAdapter iface; LONG refs; unsigned parent_calls; };
static struct failing_adapter *adapter_from_iface(IDXGIAdapter *iface) { return (struct failing_adapter *)iface; }
static HRESULT WINAPI adapter_qi(IDXGIAdapter *iface, REFIID iid, void **out)
{
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDXGIObject) && !IsEqualIID(iid, &IID_IDXGIAdapter)) return E_NOINTERFACE;
    *out = iface; IDXGIAdapter_AddRef(iface); return S_OK;
}
static ULONG WINAPI adapter_addref(IDXGIAdapter *iface) { return ++adapter_from_iface(iface)->refs; }
static ULONG WINAPI adapter_release(IDXGIAdapter *iface) { return --adapter_from_iface(iface)->refs; }
static HRESULT WINAPI adapter_set_data(IDXGIAdapter *iface, REFGUID guid, UINT size, const void *data) { return E_NOTIMPL; }
static HRESULT WINAPI adapter_set_interface(IDXGIAdapter *iface, REFGUID guid, const IUnknown *object) { return E_NOTIMPL; }
static HRESULT WINAPI adapter_get_data(IDXGIAdapter *iface, REFGUID guid, UINT *size, void *data) { return E_NOTIMPL; }
static HRESULT WINAPI adapter_get_parent(IDXGIAdapter *iface, REFIID iid, void **out)
{
    ++adapter_from_iface(iface)->parent_calls; *out = NULL; return E_FAIL;
}
static HRESULT WINAPI adapter_outputs(IDXGIAdapter *iface, UINT index, IDXGIOutput **output) { *output = NULL; return DXGI_ERROR_NOT_FOUND; }
static HRESULT WINAPI adapter_desc(IDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc) { return E_NOTIMPL; }
static HRESULT WINAPI adapter_support(IDXGIAdapter *iface, REFGUID guid, LARGE_INTEGER *version) { return E_NOTIMPL; }
static const IDXGIAdapterVtbl adapter_vtbl = {adapter_qi, adapter_addref, adapter_release,
    adapter_set_data, adapter_set_interface, adapter_get_data, adapter_get_parent,
    adapter_outputs, adapter_desc, adapter_support};

static void test_d3d10_adapter(void)
{
    struct failing_adapter adapter = {{&adapter_vtbl}, 1, 0};
    ID3D10Device *device = NULL;
    ID3D10Device1 *device1 = NULL;
    HMODULE module;
    HRESULT (WINAPI *create_device1)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device1 **);
    HRESULT hr;
    hr = D3D10CreateDevice(&adapter.iface, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device);
    CHECK(hr == E_FAIL && !device && adapter.parent_calls == 1, "D3D10 GetParent failure reached hr=%08lx calls=%u", hr, adapter.parent_calls);
    CHECK(adapter.refs == 1, "D3D10 caller adapter reference remains one, actual=%ld", adapter.refs);
    adapter.refs = 1; adapter.parent_calls = 0;
    module = LoadLibraryA("d3d10_1.dll");
    create_device1 = module ? (void *)GetProcAddress(module, "D3D10CreateDevice1") : NULL;
    CHECK(create_device1 != NULL, "D3D10.1 entry point available");
    if (!create_device1) { if (module) FreeLibrary(module); return; }
    hr = create_device1(&adapter.iface, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_FEATURE_LEVEL_10_1, D3D10_1_SDK_VERSION, &device1);
    CHECK(hr == E_FAIL && !device1 && adapter.parent_calls == 1, "D3D10.1 GetParent failure reached hr=%08lx calls=%u", hr, adapter.parent_calls);
    CHECK(adapter.refs == 1, "D3D10.1 caller adapter reference remains one, actual=%ld", adapter.refs);
    FreeLibrary(module);
}


struct state_device { ID3D10Device iface; LONG refs; float blend; unsigned gets, sets; };
static HRESULT WINAPI state_qi(ID3D10Device *iface, REFIID iid, void **out)
{
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_ID3D10Device)) return E_NOINTERFACE;
    *out = iface; ID3D10Device_AddRef(iface); return S_OK;
}
static ULONG WINAPI state_addref(ID3D10Device *iface) { return ++((struct state_device *)iface)->refs; }
static ULONG WINAPI state_release(ID3D10Device *iface) { return --((struct state_device *)iface)->refs; }
static void WINAPI state_get_blend(ID3D10Device *iface, ID3D10BlendState **state, FLOAT *factor, UINT *mask)
{
    struct state_device *device = (struct state_device *)iface;
    ++device->gets; *state = NULL; memset(factor, 0, 4 * sizeof(*factor)); factor[0] = device->blend; *mask = 0;
}
static void WINAPI state_set_blend(ID3D10Device *iface, ID3D10BlendState *state, const FLOAT *factor, UINT mask)
{
    struct state_device *device = (struct state_device *)iface;
    ++device->sets; device->blend = factor[0];
}
static const ID3D10DeviceVtbl state_vtbl = {
    .QueryInterface = state_qi, .AddRef = state_addref, .Release = state_release,
    .OMGetBlendState = state_get_blend, .OMSetBlendState = state_set_blend
};
static void test_stateblock(void)
{
    static const BYTE values[] = {0, 1, 2, 254, 255};
    unsigned i;
    for (i = 0; i < sizeof(values); ++i)
    {
        struct state_device device = {{&state_vtbl}, 1, 0.25f, 0, 0};
        D3D10_STATE_BLOCK_MASK mask;
        ID3D10StateBlock *block;
        BOOL enabled;
        HRESULT hr = D3D10StateBlockMaskDisableAll(&mask);
        CHECK(hr == S_OK, "D3D10 state mask initialization hr=%08lx", hr);
        mask.OMBlendState = values[i];
        if (values[i] == 254)
        {
            mask.OMBlendState = 255;
            hr = D3D10StateBlockMaskDisableCapture(&mask, D3D10_DST_OM_BLEND_STATE, 0, 1);
            CHECK(hr == S_OK && mask.OMBlendState == 254, "D3D10 disabling capture retains high bits mask=%u", mask.OMBlendState);
        }
        enabled = D3D10StateBlockMaskGetSetting(&mask, D3D10_DST_OM_BLEND_STATE, 0);
        hr = D3D10CreateStateBlock(&device.iface, &mask, &block);
        CHECK(hr == S_OK, "D3D10 state block creation mask=%u hr=%08lx", mask.OMBlendState, hr);
        if (FAILED(hr)) continue;
        hr = ID3D10StateBlock_Capture(block);
        CHECK(hr == S_OK, "D3D10 capture mask=%u hr=%08lx", mask.OMBlendState, hr);
        device.blend = 0.5f;
        hr = ID3D10StateBlock_Apply(block);
        CHECK(hr == S_OK && device.blend == (enabled ? 0.25f : 0.5f),
              "D3D10 state block obeys mask bit zero mask=%u enabled=%d blend=%.2f", mask.OMBlendState, enabled, device.blend);
        CHECK(device.gets == !!enabled && device.sets == !!enabled,
              "D3D10 only enabled state is read or written mask=%u gets=%u sets=%u", mask.OMBlendState, device.gets, device.sets);
        ID3D10StateBlock_Release(block);
        CHECK(device.refs == 1, "D3D10 state block releases its device reference refs=%ld", device.refs);
    }
}

/* Analyze() only needs the TTC header; no font outlines or installed fonts are required. */
static const BYTE ttc_data[] = {'t','t','c','f', 0,1,0,0, 0,0,0,3, 0,0,0,16};
struct font_stream { IDWriteFontFileStream iface; LONG refs; };
static UINT64 largest_fragment;
static unsigned fragments, released_fragments;
static HRESULT WINAPI font_qi(IDWriteFontFileStream *iface, REFIID iid, void **out)
{
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDWriteFontFileStream)) return E_NOINTERFACE;
    *out = iface; IDWriteFontFileStream_AddRef(iface); return S_OK;
}
static ULONG WINAPI font_addref(IDWriteFontFileStream *iface) { return ++((struct font_stream *)iface)->refs; }
static ULONG WINAPI font_release(IDWriteFontFileStream *iface)
{
    struct font_stream *stream = (struct font_stream *)iface;
    ULONG refs = --stream->refs; if (!refs) free(stream); return refs;
}
static HRESULT WINAPI font_read(IDWriteFontFileStream *iface, const void **data, UINT64 offset, UINT64 size, void **context)
{
    BYTE *copy;
    *data = NULL; *context = NULL;
    if (offset > sizeof(ttc_data) || size > sizeof(ttc_data) - offset) return E_FAIL;
    /* Only requested bytes become readable font data. Padding makes an old under-read deterministic without crashing. */
    if (!(copy = calloc(1, sizeof(ttc_data)))) return E_OUTOFMEMORY;
    memcpy(copy, ttc_data + offset, size);
    if (size > largest_fragment) largest_fragment = size;
    ++fragments; *context = copy; *data = copy; return S_OK;
}
static void WINAPI font_release_fragment(IDWriteFontFileStream *iface, void *context) { ++released_fragments; free(context); }
static HRESULT WINAPI font_size(IDWriteFontFileStream *iface, UINT64 *size) { *size = sizeof(ttc_data); return S_OK; }
static HRESULT WINAPI font_time(IDWriteFontFileStream *iface, UINT64 *time) { return E_NOTIMPL; }
static const IDWriteFontFileStreamVtbl font_vtbl = {font_qi, font_addref, font_release, font_read, font_release_fragment, font_size, font_time};
static HRESULT WINAPI loader_qi(IDWriteFontFileLoader *iface, REFIID iid, void **out)
{
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDWriteFontFileLoader)) return E_NOINTERFACE;
    *out = iface; IDWriteFontFileLoader_AddRef(iface); return S_OK;
}
static ULONG WINAPI loader_addref(IDWriteFontFileLoader *iface) { return 2; }
static ULONG WINAPI loader_release(IDWriteFontFileLoader *iface) { return 1; }
static HRESULT WINAPI loader_stream(IDWriteFontFileLoader *iface, const void *key, UINT32 key_size, IDWriteFontFileStream **out)
{
    struct font_stream *stream;
    *out = NULL;
    if (key_size != sizeof(unsigned) || *(const unsigned *)key != 1) return E_INVALIDARG;
    if (!(stream = calloc(1, sizeof(*stream)))) return E_OUTOFMEMORY;
    stream->iface.lpVtbl = &font_vtbl; stream->refs = 1; *out = &stream->iface; return S_OK;
}
static const IDWriteFontFileLoaderVtbl loader_vtbl = {loader_qi, loader_addref, loader_release, loader_stream};
static IDWriteFontFileLoader loader = {&loader_vtbl};

static void test_dwrite(void)
{
    IDWriteFactory *factory;
    IDWriteFontFile *font;
    unsigned key = 1;
    UINT32 count = 0;
    BOOL supported = FALSE;
    DWRITE_FONT_FILE_TYPE file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    DWRITE_FONT_FACE_TYPE face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown **)&factory);
    CHECK(hr == S_OK, "DWrite factory hr=%08lx", hr); if (FAILED(hr)) return;
    hr = IDWriteFactory_RegisterFontFileLoader(factory, &loader);
    CHECK(hr == S_OK, "DWrite custom font loader registration hr=%08lx", hr);
    if (FAILED(hr)) { IDWriteFactory_Release(factory); return; }
    hr = IDWriteFactory_CreateCustomFontFileReference(factory, &key, sizeof(key), &loader, &font);
    CHECK(hr == S_OK, "DWrite custom font file hr=%08lx", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDWriteFontFile_Analyze(font, &supported, &file_type, &face_type, &count);
        CHECK(hr == S_OK && supported && file_type == DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION && count == 3,
              "DWrite TTC count from requested fragments hr=%08lx supported=%d type=%u count=%u expected=3", hr, supported, file_type, count);
        CHECK(largest_fragment >= 12 && fragments == released_fragments,
              "DWrite reads complete TTC count and releases fragments max=%llu acquired=%u released=%u", largest_fragment, fragments, released_fragments);
        IDWriteFontFile_Release(font);
    }
    IDWriteFactory_UnregisterFontFileLoader(factory, &loader);
    IDWriteFactory_Release(factory);
}

/* Wine metadata-test image (4x8 white), with a minimal APP1/Exif ImageWidth entry. */
static const char jpeg[] =
{
    0xff, 0xd8,
    0xff, 0xe1, 0x00, 0x22, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00,
    0x49, 0x49, 0x2a, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43,
    0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x03,
    0x03, 0x03, 0x03, 0x04, 0x06, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x06,
    0x06, 0x05, 0x06, 0x09, 0x08, 0x0a, 0x0a, 0x09, 0x08, 0x09, 0x09, 0x0a,
    0x0c, 0x0f, 0x0c, 0x0a, 0x0b, 0x0e, 0x0b, 0x09, 0x09, 0x0d, 0x11, 0x0d,
    0x0e, 0x0f, 0x10, 0x10, 0x11, 0x10, 0x0a, 0x0c, 0x12, 0x13, 0x12, 0x10,
    0x13, 0x0f, 0x10, 0x10, 0x10, 0xff, 0xc0, 0x00, 0x0b, 0x08, 0x00, 0x08,
    0x00, 0x04, 0x01, 0x01, 0x11, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x09, 0xff, 0xc4, 0x00, 0x14, 0x10, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xda, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3f, 0x00,
    0x54, 0xdf, 0xff, 0xd9
};

struct strict_stream { IStream iface; LONG refs; IStream *inner; unsigned null_reads, header_reads, exif_reads, reads; ULONGLONG position; };
static HRESULT WINAPI stream_qi(IStream *iface, REFIID iid, void **out)
{
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_ISequentialStream) && !IsEqualIID(iid, &IID_IStream)) return E_NOINTERFACE;
    *out = iface; IStream_AddRef(iface); return S_OK;
}
static ULONG WINAPI stream_addref(IStream *iface) { return ++((struct strict_stream *)iface)->refs; }
static ULONG WINAPI stream_release(IStream *iface)
{
    struct strict_stream *stream = (struct strict_stream *)iface;
    ULONG refs = --stream->refs;
    if (!refs) { IStream_Release(stream->inner); free(stream); }
    return refs;
}
static HRESULT WINAPI stream_read(IStream *iface, void *buffer, ULONG size, ULONG *read)
{
    struct strict_stream *stream = (struct strict_stream *)iface;
    HRESULT hr;
    ++stream->reads;
    if (size == 4 && stream->position == 2) ++stream->header_reads;
    if (size == 4 && stream->position == 6) ++stream->exif_reads;
    if (!read) { ++stream->null_reads; return E_POINTER; }
    hr = IStream_Read(stream->inner, buffer, size, read); stream->position += *read; return hr;
}
static HRESULT WINAPI stream_write(IStream *iface, const void *data, ULONG size, ULONG *written) { return E_NOTIMPL; }
static HRESULT WINAPI stream_seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *position)
{
    struct strict_stream *stream = (struct strict_stream *)iface;
    ULARGE_INTEGER pos;
    HRESULT hr = IStream_Seek(stream->inner, move, origin, &pos);
    if (SUCCEEDED(hr)) { stream->position = pos.QuadPart; if (position) *position = pos; }
    return hr;
}
static HRESULT WINAPI stream_setsize(IStream *iface, ULARGE_INTEGER size) { return E_NOTIMPL; }
static HRESULT WINAPI stream_copy(IStream *iface, IStream *target, ULARGE_INTEGER size, ULARGE_INTEGER *read, ULARGE_INTEGER *written) { return E_NOTIMPL; }
static HRESULT WINAPI stream_commit(IStream *iface, DWORD flags) { return E_NOTIMPL; }
static HRESULT WINAPI stream_revert(IStream *iface) { return E_NOTIMPL; }
static HRESULT WINAPI stream_lock(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD flags) { return E_NOTIMPL; }
static HRESULT WINAPI stream_unlock(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD flags) { return E_NOTIMPL; }
static HRESULT WINAPI stream_stat(IStream *iface, STATSTG *stat, DWORD flags) { return IStream_Stat(((struct strict_stream *)iface)->inner, stat, flags); }
static HRESULT WINAPI stream_clone(IStream *iface, IStream **out) { *out = NULL; return E_NOTIMPL; }
static const IStreamVtbl stream_vtbl = {stream_qi, stream_addref, stream_release, stream_read, stream_write, stream_seek,
    stream_setsize, stream_copy, stream_commit, stream_revert, stream_lock, stream_unlock, stream_stat, stream_clone};

static void test_wic(void)
{
    struct strict_stream *stream;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICMetadataQueryReader *metadata;
    PROPVARIANT value = {0};
    LARGE_INTEGER zero = {{0}};
    ULONG written;
    HRESULT hr;
    if (!(stream = calloc(1, sizeof(*stream)))) { CHECK(0, "WIC stream allocation"); return; }
    stream->iface.lpVtbl = &stream_vtbl; stream->refs = 1;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream->inner);
    CHECK(hr == S_OK, "WIC memory stream hr=%08lx", hr);
    if (FAILED(hr)) { free(stream); return; }
    hr = IStream_Write(stream->inner, jpeg, sizeof(jpeg), &written);
    CHECK(hr == S_OK && written == sizeof(jpeg), "WIC JPEG fixture write hr=%08lx bytes=%lu", hr, written);
    IStream_Seek(stream->inner, zero, STREAM_SEEK_SET, NULL);
    hr = CoCreateInstance(&CLSID_WICJpegDecoder, NULL, CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void **)&decoder);
    CHECK(hr == S_OK, "WIC JPEG decoder hr=%08lx", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapDecoder_Initialize(decoder, &stream->iface, WICDecodeMetadataCacheOnDemand);
        CHECK(hr == S_OK, "WIC JPEG initialization hr=%08lx", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
            CHECK(hr == S_OK, "WIC JPEG frame hr=%08lx", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &metadata);
                CHECK(hr == S_OK, "WIC metadata query reader hr=%08lx", hr);
                if (SUCCEEDED(hr))
                {
                    /* Creating a query reader alone does not load metadata blocks. */
                    hr = IWICMetadataQueryReader_GetMetadataByName(metadata, L"/app1/ifd/{ushort=256}", &value);
                    CHECK(hr == S_OK && value.vt == VT_UI4 && value.ulVal == 4,
                          "WIC Exif ImageWidth hr=%08lx type=%u value=%lu expected=4", hr, value.vt, value.ulVal);
                    PropVariantClear(&value);
                    IWICMetadataQueryReader_Release(metadata);
                }
                IWICBitmapFrameDecode_Release(frame);
            }
        }
        CHECK(stream->header_reads > 0, "WIC metadata parser reached four-byte header reads=%u", stream->header_reads);
        CHECK(stream->exif_reads > 0, "WIC metadata parser reached four-byte Exif signature reads=%u", stream->exif_reads);
        CHECK(stream->null_reads == 0, "WIC nonconforming stream never receives NULL pcbRead, null=%u total=%u", stream->null_reads, stream->reads);
        IWICBitmapDecoder_Release(decoder);
    }
    IStream_Release(&stream->iface);
}

int main(int argc, char **argv)
{
    const char *test = argc > 1 ? argv[1] : "all";
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CHECK(SUCCEEDED(hr), "COM initialize hr=%08lx", hr);
    if (FAILED(hr)) return 2;
    if (!strcmp(test, "all") || !strcmp(test, "schannel")) test_schannel();
    if (!strcmp(test, "all") || !strcmp(test, "adapter")) test_d3d10_adapter();
    if (!strcmp(test, "all") || !strcmp(test, "stateblock")) test_stateblock();
    if (!strcmp(test, "all") || !strcmp(test, "dwrite")) test_dwrite();
    if (!strcmp(test, "all") || !strcmp(test, "wic")) test_wic();
    if (strcmp(test, "all") && strcmp(test, "schannel") && strcmp(test, "adapter") && strcmp(test, "dwrite") && strcmp(test, "wic") && strcmp(test, "stateblock"))
        CHECK(0, "Unknown probe mode %s", test);
    CoUninitialize();
    printf("RESULT checks=%u failures=%u\n", checks, failures);
    return failures ? 1 : 0;
}
