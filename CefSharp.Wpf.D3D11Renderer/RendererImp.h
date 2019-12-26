#pragma once

public class RendererImp
{
public:
    ~RendererImp();
    bool Init();
    bool SetTexture(void* sharedHandle, int& width, int& height);
    void Render(void* surface, bool isNewSurface);

private:
    bool InitDevice();
    bool InitQuad();
    bool InitEffect();
    bool InitSampler();
    bool InitRenderTarget(void* surface);
    bool InitPipelines();


    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D10Blob* vertexShaderBlob = nullptr;
    ID3D10Blob* pixelShaderBlob = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11ShaderResourceView* shaderResourceView = nullptr;

    UINT viewportWidth = -1;
    UINT viewportHeight = -1;

    D3D_DRIVER_TYPE driverType;
    D3D_FEATURE_LEVEL featureLevel;

};


