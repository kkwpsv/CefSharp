#pragma once

typedef HRESULT(WINAPI* PFN_D3DCOMPILE)(
    LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*,
    ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

public class RendererImp
{
public:
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
    ID3D11Buffer* buffer = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11ShaderResourceView* shaderResourceView = nullptr;

    UINT viewportWidth = -1;
    UINT viewportHeight = -1;

    D3D_DRIVER_TYPE driverType;
    D3D_FEATURE_LEVEL featureLevel;

    void* lastTexture = nullptr;


    bool CreateTexture(int width, int height, DXGI_FORMAT format, const void* data, size_t row_stride);
    bool CreateTestTexture();


    ID3D11Texture2D* outputResource;
};


