#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

/* Isolated DXVK diagnostic. This is NOT the game executable. */
int _fltused = 0;
void *memset(void *target, int value, SIZE_T count) {
    volatile unsigned char *p = (volatile unsigned char*)target;
    while (count--) *p++ = (unsigned char)value;
    return target;
}
static HANDLE output;
static void text(const char *s) {
    DWORD n = 0, written;
    while (s[n]) ++n;
    WriteFile(output, s, n, &written, NULL);
    FlushFileBuffers(output);
}
static void hex(UINT v) {
    char b[11] = "0x00000000";
    const char *digits = "0123456789abcdef";
    for (int i = 0; i < 8; ++i) b[9-i] = digits[(v >> (i*4)) & 15];
    text(b);
}
static void check(const char *step, HRESULT hr) {
    text(step); text(" "); hex(hr); text("\n");
    if (FAILED(hr)) ExitProcess(2);
}
void mainCRTStartup(void) {
    char mode[64] = {0};
    GetEnvironmentVariableA("BRUH_UAV_CASE", mode, sizeof(mode));
    BOOL read = mode[0] == 'r';
    BOOL half = mode[1] == 'h';
    BOOL bufferCase = mode[2] == 'b';
    char path[MAX_PATH] = {0};
    GetEnvironmentVariableA("BRUH_UAV_RESULT", path, sizeof(path));
    output = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (output == INVALID_HANDLE_VALUE) ExitProcess(2);
    text("ISOLATED UAV SELFTEST mode="); text(mode); text("\n");
    const char *readShader =
        "RWTexture2D<float4> image : register(u0);\n"
        "RWStructuredBuffer<float4> result : register(u1);\n"
        "[numthreads(1,1,1)] void main(uint3 id : SV_DispatchThreadID) {\n"
        "result[0] = image[uint2(0,0)]; }\n";
    const char *writeShader =
        "RWTexture2D<float4> image : register(u0);\n"
        "RWStructuredBuffer<float4> result : register(u1);\n"
        "[numthreads(1,1,1)] void main(uint3 id : SV_DispatchThreadID) {\n"
        "image[uint2(0,0)] = float4(0.5, 0.25, 0.125, 1.0);\n"
        "result[0] = float4(1.0, 1.0, 1.0, 1.0); }\n";
    const char *readBufferShader =
        "RWBuffer<float4> image : register(u0);\n"
        "RWStructuredBuffer<float4> result : register(u1);\n"
        "[numthreads(1,1,1)] void main(uint3 id : SV_DispatchThreadID) {\n"
        "result[0] = image[0]; }\n";
    const char *writeBufferShader =
        "RWBuffer<float4> image : register(u0);\n"
        "RWStructuredBuffer<float4> result : register(u1);\n"
        "[numthreads(1,1,1)] void main(uint3 id : SV_DispatchThreadID) {\n"
        "image[0] = float4(0.5, 0.25, 0.125, 1.0);\n"
        "result[0] = float4(1.0, 1.0, 1.0, 1.0); }\n";
    const char *source = bufferCase ? (read ? readBufferShader : writeBufferShader)
        : (read ? readShader : writeShader);
    SIZE_T size = 0;
    while (source[size]) ++size;
    ID3DBlob *code = NULL, *errors = NULL;
    HRESULT hr = D3DCompile(source, size, "isolated-uav-selftest", NULL, NULL,
        "main", "cs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);
    if (errors) text((const char*)ID3D10Blob_GetBufferPointer(errors));
    check("D3DCompile", hr);
    ID3D11Device *device = NULL;
    ID3D11DeviceContext *context = NULL;
    const D3D_FEATURE_LEVEL requested[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL actual;
    check("D3D11CreateDevice", D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE,
        NULL, 0, requested, 2, D3D11_SDK_VERSION, &device, &actual, &context));
    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 support = {
        half ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM, 0};
    check("format query", ID3D11Device_CheckFeatureSupport(device,
        D3D11_FEATURE_FORMAT_SUPPORT2, &support, sizeof(support)));
    text("typed load supported=");
    hex(!!(support.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD)); text("\n");
    ID3D11ComputeShader *shader = NULL;
    check("CreateComputeShader", ID3D11Device_CreateComputeShader(device,
        ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &shader));
    D3D11_TEXTURE2D_DESC td = {0};
    td.Width = td.Height = td.MipLevels = td.ArraySize = 1;
    td.Format = support.InFormat;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    const unsigned short halfData[] = {0x3800, 0x3400, 0x3000, 0x3c00};
    const unsigned char byteData[] = {128, 64, 32, 255};
    const unsigned short zeroData[] = {0, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA initial = {read
        ? (half ? (const void*)halfData : (const void*)byteData) : (const void*)zeroData,
        half ? 8 : 4, 0};
    ID3D11Texture2D *texture = NULL;
    ID3D11Buffer *typedBuffer = NULL;
    ID3D11UnorderedAccessView *views[2] = {NULL, NULL};
    if (bufferCase) {
        D3D11_BUFFER_DESC typedDesc = {0};
        typedDesc.ByteWidth = half ? 8 : 4;
        typedDesc.Usage = D3D11_USAGE_DEFAULT;
        typedDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        check("Create typed buffer", ID3D11Device_CreateBuffer(device, &typedDesc, &initial, &typedBuffer));
        D3D11_UNORDERED_ACCESS_VIEW_DESC typedView = {0};
        typedView.Format = td.Format;
        typedView.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        typedView.Buffer.NumElements = 1;
        check("Create typed buffer UAV", ID3D11Device_CreateUnorderedAccessView(device,
            (ID3D11Resource*)typedBuffer, &typedView, &views[0]));
    } else {
        check("CreateTexture2D", ID3D11Device_CreateTexture2D(device, &td, &initial, &texture));
        check("Create image UAV", ID3D11Device_CreateUnorderedAccessView(device,
            (ID3D11Resource*)texture, NULL, &views[0]));
    }
    D3D11_BUFFER_DESC bd = {0};
    bd.ByteWidth = 16;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = 16;
    ID3D11Buffer *buffer = NULL, *staging = NULL;
    check("Create result buffer", ID3D11Device_CreateBuffer(device, &bd, NULL, &buffer));
    check("Create result UAV", ID3D11Device_CreateUnorderedAccessView(device,
        (ID3D11Resource*)buffer, NULL, &views[1]));
    bd.Usage = D3D11_USAGE_STAGING;
    bd.BindFlags = 0;
    bd.MiscFlags = 0;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    check("Create staging", ID3D11Device_CreateBuffer(device, &bd, NULL, &staging));
    ID3D11DeviceContext_CSSetShader(context, shader, NULL, 0);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(context, 0, 2, views, NULL);
    text("ABOUT TO DISPATCH: unsupported read case MUST abort here before command recording\n");
    ID3D11DeviceContext_Dispatch(context, 1, 1, 1);
    ID3D11DeviceContext_CopyResource(context, (ID3D11Resource*)staging, (ID3D11Resource*)buffer);
    ID3D11DeviceContext_Flush(context);
    D3D11_MAPPED_SUBRESOURCE mapped = {0};
    check("Map completed result", ID3D11DeviceContext_Map(context,
        (ID3D11Resource*)staging, 0, D3D11_MAP_READ, 0, &mapped));
    const float *v = (const float*)mapped.pData;
    for (int i = 0; i < 4; ++i) { hex(((const UINT*)mapped.pData)[i]); text(" "); }
    text("\n");
    BOOL correct = read
        ? v[0] > 0.49f && v[0] < 0.51f && v[1] > 0.24f && v[1] < 0.26f
          && v[2] > 0.12f && v[2] < 0.13f && v[3] == 1.0f
        : v[0] == 1.0f && v[1] == 1.0f && v[2] == 1.0f && v[3] == 1.0f;
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)staging, 0);
    if (!read) {
        ID3D11Resource *imageStaging = NULL;
        if (bufferCase) {
            D3D11_BUFFER_DESC sd = {0};
            sd.ByteWidth = half ? 8 : 4;
            sd.Usage = D3D11_USAGE_STAGING;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            check("Create typed buffer staging", ID3D11Device_CreateBuffer(device,
                &sd, NULL, (ID3D11Buffer**)&imageStaging));
        } else {
            td.Usage = D3D11_USAGE_STAGING;
            td.BindFlags = 0;
            td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            check("Create image staging", ID3D11Device_CreateTexture2D(device,
                &td, NULL, (ID3D11Texture2D**)&imageStaging));
        }
        ID3D11DeviceContext_CopyResource(context, imageStaging,
            bufferCase ? (ID3D11Resource*)typedBuffer : (ID3D11Resource*)texture);
        check("Map actual typed UAV store", ID3D11DeviceContext_Map(context,
            imageStaging, 0, D3D11_MAP_READ, 0, &mapped));
        if (half) {
            const unsigned short *stored = (const unsigned short*)mapped.pData;
            for (int i = 0; i < 4; ++i) correct = correct && stored[i] == halfData[i];
        } else {
            const unsigned char *stored = (const unsigned char*)mapped.pData;
            for (int i = 0; i < 4; ++i) correct = correct && stored[i] == byteData[i];
        }
        ID3D11DeviceContext_Unmap(context, imageStaging, 0);
    }
    text(correct ? "PASS completed GPU readback\n" : "FAIL unexpected GPU data\n");
    CloseHandle(output);
    ExitProcess(correct ? 0 : 3);
}
