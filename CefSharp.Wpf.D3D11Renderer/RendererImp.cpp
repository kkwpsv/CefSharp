#include "pch.h"
#include "RendererImp.h"


struct SimpleVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
};

ID3DBlob* CompileShader(const char* sourceCode, const char* entryPoint, const char* model)
{
    DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG;
    flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    flags |= D3DCOMPILE_AVOID_FLOW_CONTROL;
#endif

    ID3DBlob* blob = nullptr;
    ID3DBlob* blob_err = nullptr;

    auto const hr = D3DCompile(sourceCode, strlen(sourceCode), nullptr, nullptr, nullptr, entryPoint, model, flags, 0, &blob, &blob_err);

    if (FAILED(hr))
    {
        if (blob_err)
        {
            blob_err->Release();
        }
        return nullptr;
    }

    if (blob_err)
    {
        blob_err->Release();
    }
    return blob;
}

bool RendererImp::Init()
{
    return InitDevice() && InitQuad() && InitEffect() /*&& CreateTestTexture()*/ && InitPipelines();
}

bool RendererImp::SetTexture(void* sharedHandle, int& width, int& height)
{
    ID3D11Texture2D* texture = nullptr;
    if (FAILED(device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (void**)(&texture))))
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC td;
    texture->GetDesc(&td);

    width = td.Width;
    height = td.Height;

    ID3D11ShaderResourceView* srv = nullptr;

    if (!(td.BindFlags & D3D11_BIND_SHADER_RESOURCE))
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    srv_desc.Format = td.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;

    if (FAILED(device->CreateShaderResourceView(texture, &srv_desc, &shaderResourceView)))
    {
        texture->Release();
        return false;
    }

    ID3D11ShaderResourceView* views[1] = { shaderResourceView };
    deviceContext->PSSetShaderResources(0, 1, views);

    return true;
}

void RendererImp::Render(void* surface, bool isNewSurface)
{
    if (isNewSurface)
    {
        deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        InitRenderTarget(surface);
    }

    //float color[4] = { 0, 0, 1, 1 };
    //deviceContext->ClearRenderTargetView(renderTargetView, color);

    deviceContext->Draw(4, 0);
    deviceContext->Flush();
}

bool RendererImp::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    // DX10 or 11 devices are suitable
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex)
    {
        hr = D3D11CreateDevice(NULL, driverTypes[driverTypeIndex], NULL, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);

        if (SUCCEEDED(hr))
        {
            driverType = driverTypes[driverTypeIndex];
            break;
        }
    }

    return SUCCEEDED(hr);
}

bool RendererImp::InitQuad()
{
    float x = (0 * 2.0f) - 1.0f;
    float y = 1.0f - (0 * 2.0f);
    float width = 1 * 2.0f;
    float height = 1 * 2.0f;
    float z = 1.0f;

    SimpleVertex vertices[] = {

        { DirectX::XMFLOAT3(x, y, z), DirectX::XMFLOAT2(0.0f, 1.0f) },
        { DirectX::XMFLOAT3(x + width, y, z), DirectX::XMFLOAT2(1.0f, 1.0f) },
        { DirectX::XMFLOAT3(x, y - height, z), DirectX::XMFLOAT2(0.0f, 0.0f) },
        { DirectX::XMFLOAT3(x + width, y - height, z), DirectX::XMFLOAT2(1.0f, 0.0f) }
    };

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(SimpleVertex) * 4;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = vertices;

    auto result = device->CreateBuffer(&desc, &srd, &buffer);
    return SUCCEEDED(result);
}

bool RendererImp::InitEffect()
{
    auto const vsh =
        R"--(struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = input.pos;
	output.tex = input.tex;
	return output;
})--";

    auto const psh =
        R"--(Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target
{
	return tex0.Sample(samp0, input.tex);
})--";
    auto const vs_blob = CompileShader(vsh, "main", "vs_4_0");
    auto const ps_blob = CompileShader(psh, "main", "ps_4_0");

    if (!vs_blob || !ps_blob)
    {
        return false;
    }

    if (FAILED(device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vertexShader)))
    {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT elements = ARRAYSIZE(layout_desc);

    if (FAILED(device->CreateInputLayout(layout_desc, elements, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &inputLayout)))
    {
        return false;
    }

    if (FAILED(device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &pixelShader)))
    {
        return false;
    }

    return true;
}

bool RendererImp::InitSampler()
{
    ID3D11SamplerState* sampler = nullptr;

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    if (FAILED(device->CreateSamplerState(&samplerDesc, &sampler)))
    {
        return false;
    }

    ID3D11SamplerState* samplers[1] = { sampler };
    deviceContext->PSSetSamplers(0, 1, samplers);
    return true;
}

bool RendererImp::InitRenderTarget(void* surface)
{
    HRESULT result = S_OK;

    IUnknown* unknown = (IUnknown*)surface;
    IDXGIResource* dxgiResource;

    result = unknown->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);
    if (FAILED(result))
    {
        return false;
    }

    HANDLE sharedHandle;
    result = dxgiResource->GetSharedHandle(&sharedHandle);
    dxgiResource->Release();
    if (FAILED(result))
    {
        return false;
    }

    IUnknown* tempResource;
    result = device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)(&tempResource));
    if (FAILED(result))
    {
        return false;
    }


    result = tempResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&outputResource));
    tempResource->Release();
    if (FAILED(result))
    {
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
    rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D.MipSlice = 0;

    result = device->CreateRenderTargetView(outputResource, &rtDesc, &renderTargetView);
    if (FAILED(result))
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC outputResourceDesc;
    outputResource->GetDesc(&outputResourceDesc);
    if (outputResourceDesc.Width != viewportWidth || outputResourceDesc.Height != viewportHeight)
    {
        viewportWidth = outputResourceDesc.Width;
        viewportHeight = outputResourceDesc.Height;

        D3D11_VIEWPORT vp = {};
        vp.Width = (float)viewportWidth;
        vp.Height = (float)viewportHeight;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        deviceContext->RSSetViewports(1, &vp);
    }

    deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    outputResource->Release();

    return true;
}


bool RendererImp::InitPipelines()
{

    UINT offset = 0;
    UINT strides = sizeof(SimpleVertex);

    ID3D11Buffer* buffers[1] = { buffer };
    deviceContext->IASetVertexBuffers(0, 1, buffers, &strides, &offset);
    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    return true;
}

bool RendererImp::CreateTexture(int width, int height, DXGI_FORMAT format, const void* data, size_t row_stride)
{
    D3D11_TEXTURE2D_DESC td;
    td.ArraySize = 1;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = data ? 0 : D3D11_CPU_ACCESS_WRITE;
    td.Format = format;
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.MiscFlags = 0;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = data ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;

    D3D11_SUBRESOURCE_DATA srd;
    srd.pSysMem = data;
    srd.SysMemPitch = static_cast<uint32_t>(row_stride);
    srd.SysMemSlicePitch = 0;

    ID3D11Texture2D* tex = nullptr;
    auto hr = device->CreateTexture2D(&td, data ? &srd : nullptr, &tex);
    if (FAILED(hr)) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    srv_desc.Format = td.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(tex, &srv_desc, &shaderResourceView);

    if (FAILED(hr))
    {
        tex->Release();
        return false;
    }

    return true;
}

bool RendererImp::CreateTestTexture()
{
    auto size = 100 * 100 * 4;
    byte* texture = new byte[size];
    for (int i = 0; i < size; i += 4)
    {
        *((UINT*)(texture + i)) = 0xFF0000FF;
    }

    CreateTexture(100, 100, DXGI_FORMAT_R8G8B8A8_UNORM, texture, 100 * 4);
    return true;
}
