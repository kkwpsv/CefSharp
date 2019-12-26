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

    if (blob_err)
    {
        blob_err->Release();
    }

    if (FAILED(hr))
    {
        return nullptr;
    }
    else
    {
        return blob;
    }
}

RendererImp::~RendererImp()
{
    if (device)
    {
        device->Release();
    }
    if (deviceContext)
    {
        deviceContext->Release();
    }
    if (vertexBuffer)
    {
        vertexBuffer->Release();
    }
    if (vertexShaderBlob)
    {
        vertexShaderBlob->Release();
    }
    if (pixelShaderBlob)
    {
        pixelShaderBlob->Release();
    }
    if (vertexShader)
    {
        vertexShader->Release();
    }
    if (inputLayout)
    {
        inputLayout->Release();
    }
    if (pixelShader)
    {
        pixelShader->Release();
    }
    if (sampler)
    {
        sampler->Release();
    }
    if (renderTargetView)
    {
        renderTargetView->Release();
    }
    if (shaderResourceView)
    {
        shaderResourceView->Release();
    }
}

bool RendererImp::Init()
{
    return InitDevice() && InitQuad() && InitEffect() && InitSampler() && InitPipelines();
}

bool RendererImp::SetTexture(void* sharedHandle, int& width, int& height)
{
    if (shaderResourceView)
    {
        shaderResourceView->Release();
    }
    ID3D11ShaderResourceView* views[1] = { nullptr };
    deviceContext->PSSetShaderResources(0, 1, views);

    ID3D11Texture2D* texture = nullptr;
    auto hr = device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (void**)(&texture));
    if (FAILED(hr))
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
        texture->Release();
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    srv_desc.Format = td.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(texture, &srv_desc, &shaderResourceView);
    texture->Release();
    if (FAILED(hr))
    {
        return false;
    }

    views[0] = shaderResourceView;
    deviceContext->PSSetShaderResources(0, 1, views);

    return true;
}

void RendererImp::Render(void* surface, bool isNewSurface)
{
    if (isNewSurface)
    {
        if (renderTargetView)
        {
            renderTargetView->Release();
        }
        deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        InitRenderTarget(surface);
    }

    deviceContext->Draw(4, 0);
    deviceContext->Flush();
}

bool RendererImp::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

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

    auto result = device->CreateBuffer(&desc, &srd, &vertexBuffer);
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

    vertexShaderBlob = CompileShader(vsh, "main", "vs_4_0");
    pixelShaderBlob = CompileShader(psh, "main", "ps_4_0");

    if (!vertexShaderBlob || !pixelShaderBlob)
    {
        return false;
    }

    if (FAILED(device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader)))
    {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT elements = ARRAYSIZE(layoutDesc);

    if (FAILED(device->CreateInputLayout(layoutDesc, elements, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &inputLayout)))
    {
        return false;
    }

    if (FAILED(device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader)))
    {
        return false;
    }

    return true;
}

bool RendererImp::InitSampler()
{
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

    return true;
}

bool RendererImp::InitPipelines()
{

    UINT offset = 0;
    UINT strides = sizeof(SimpleVertex);

    ID3D11Buffer* vertexBuffers[1] = { vertexBuffer };
    deviceContext->IASetVertexBuffers(0, 1, vertexBuffers, &strides, &offset);
    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);

    ID3D11SamplerState* samplers[1] = { sampler };
    deviceContext->PSSetSamplers(0, 1, samplers);

    return true;
}

bool RendererImp::InitRenderTarget(void* surface)
{
    HRESULT hr = S_OK;

    IUnknown* unknown = (IUnknown*)surface;
    IDXGIResource* dxgiResource;
    ID3D11Texture2D* outputResource;

    hr = unknown->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);
    if (FAILED(hr))
    {
        return false;
    }

    HANDLE sharedHandle;
    hr = dxgiResource->GetSharedHandle(&sharedHandle);
    dxgiResource->Release();
    if (FAILED(hr))
    {
        return false;
    }

    hr = device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (void**)(&outputResource));
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
    rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D.MipSlice = 0;

    hr = device->CreateRenderTargetView(outputResource, &rtDesc, &renderTargetView);
    if (FAILED(hr))
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
