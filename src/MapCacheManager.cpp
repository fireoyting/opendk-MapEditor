#include "MapCacheManager.h"

MapCacheManager::MapCacheManager()
    : m_initialized(false)
    , m_mapCacheValid(false)
    , minimapValid(false)
    , m_mapWidth(0)
    , m_mapHeight(0)
    , m_tileW(48)
    , m_tileH(24)
{
}

MapCacheManager::~MapCacheManager()
{
    Release();
}

bool MapCacheManager::Initialize(ID2D1DeviceContext* d2dCtx, ID3D11Device* d3dDev)
{
    if (!d2dCtx || !d3dDev)
        return false;

    d2dContext = d2dCtx;
    d3dDevice = d3dDev;
    m_initialized = true;

    return true;
}

void MapCacheManager::Release()
{
    // 只释放资源，保留设备引用
    mapCacheBitmap.Reset();
    minimapBitmap.Reset();
    minimapSRV.Reset();

    // 不释放设备引用，因为D3D/D2D设备在程序生命周期内一直存在

    m_mapCacheValid = false;
    minimapValid = false;
    m_mapWidth = 0;
    m_mapHeight = 0;
}

bool MapCacheManager::UpdateMinimap(int maxSize)
{
    if (!m_initialized || !m_mapCacheValid || !d2dContext.Get() || !d3dDevice.Get())
        return false;

    // 计算minimap尺寸
    float minimapW = (float)maxSize;
    float minimapH = (float)maxSize;

    if (m_mapWidth * m_tileW > m_mapHeight * m_tileH)
    {
        minimapH = minimapW * (float)(m_mapHeight * m_tileH) / (float)(m_mapWidth * m_tileW);
    }
    else
    {
        minimapW = minimapH * (float)(m_mapWidth * m_tileW) / (float)(m_mapHeight * m_tileH);
    }

    UINT width = (UINT)minimapW;
    UINT height = (UINT)minimapH;

    // 创建D3D Texture2D
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = d3dDevice->CreateTexture2D(&texDesc, nullptr, texture.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 获取DXGI Surface
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
    hr = texture.As(&dxgiSurface);
    if (FAILED(hr))
        return false;

    // 创建D2D Bitmap
    D2D1_BITMAP_PROPERTIES1 bitmapProps = {};
    bitmapProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmapProps.dpiX = 96.0f;
    bitmapProps.dpiY = 96.0f;

    minimapBitmap.Reset();
    hr = d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bitmapProps, minimapBitmap.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 绘制Minimap
    d2dContext->SetTarget(minimapBitmap.Get());
    d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    if (mapCacheBitmap)
    {
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0,
            (float)mapCacheBitmap->GetPixelSize().width,
            (float)mapCacheBitmap->GetPixelSize().height);
        D2D1_RECT_F dstRect = D2D1::RectF(0, 0, minimapW, minimapH);

        d2dContext->DrawBitmap(mapCacheBitmap.Get(), dstRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    }

    d2dContext->SetTarget(nullptr);
    d2dContext->Flush(); // 关键：刷新确保D2D数据写入D3D Texture

    // 创建SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    minimapSRV.Reset();
    hr = d3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, minimapSRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    minimapValid = true;
    return true;
}

void MapCacheManager::RenderMap(ID2D1DeviceContext* d2dCtx, D2D1_RECT_F destRect)
{
    if (!d2dCtx || !m_mapCacheValid || !mapCacheBitmap)
        return;

    D2D1_RECT_F srcRect = D2D1::RectF(0, 0,
        (float)mapCacheBitmap->GetPixelSize().width,
        (float)mapCacheBitmap->GetPixelSize().height);

    d2dCtx->DrawBitmap(mapCacheBitmap.Get(), destRect, 1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
}
