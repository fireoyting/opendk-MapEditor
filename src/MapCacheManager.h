#ifndef MAP_CACHE_MANAGER_H
#define MAP_CACHE_MANAGER_H

#include "WinMin.h"
#include <wrl/client.h>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "../maplib/Zone.h"
//由于某些原因 我们弃用了此对象
class MapCacheManager
{
public:
    MapCacheManager();
    ~MapCacheManager();

    // 初始化，需要D2D和D3D设备
    bool Initialize(ID2D1DeviceContext* d2dContext, ID3D11Device* d3dDevice);

    // 释放资源
    void Release();

    // 更新地图缓存（需要在地图像素数据改变时调用）
    // 参数：zone数据，纹理管理器，tile尺寸
    template<typename TTextureManager>
    bool UpdateMapCache(Zone& zone, TTextureManager* texManager, int tileW = 48, int tileH = 24)
    {
        if (!m_initialized || !d2dContext.Get())
            return false;

        uint32_t mapWidth = zone.GetWidth();
        uint32_t mapHeight = zone.GetHeight();

        if (mapWidth == 0 || mapHeight == 0)
            return false;

        // 计算地图像素大小
        uint32_t mapPixelW = mapWidth * tileW;
        uint32_t mapPixelH = mapHeight * tileH;

        // 限制最大尺寸
        if (mapPixelW > 4096) mapPixelW = 4096;
        if (mapPixelH > 4096) mapPixelH = 4096;

        // 创建地图缓存Bitmap
        D2D1_SIZE_U bitmapSize = { mapPixelW, mapPixelH };
        D2D1_BITMAP_PROPERTIES1 bitmapProps = {};
        bitmapProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmapProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;

        mapCacheBitmap.Reset();

        HRESULT hr = d2dContext->CreateBitmap(bitmapSize, nullptr, 0, &bitmapProps, mapCacheBitmap.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        // 设置目标并绘制
        d2dContext->SetTarget(mapCacheBitmap.Get());
        d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        // 绘制所有tile
        for (uint32_t y = 0; y < mapHeight; y++)
        {
            for (uint32_t x = 0; x < mapWidth; x++)
            {
                GameSector* sector = zone.GetSector(x, y);
                if (sector)
                {
                    uint16_t spriteID = sector->GetSpriteID();
                    auto sprite = texManager->GetTexture(spriteID);
                    if (sprite)
                    {
                        float drawX = (float)x * tileW;
                        float drawY = (float)y * tileH;

                        D2D1_RECT_F tileDest = D2D1::RectF(drawX, drawY, drawX + (float)tileW, drawY + (float)tileH);
                        if (sprite)
                            d2dContext->DrawBitmap(sprite, tileDest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                    }
                }
            }
        }

        // 恢复目标
        d2dContext->SetTarget(nullptr);

        m_mapCacheValid = true;
        m_mapWidth = mapWidth;
        m_mapHeight = mapHeight;
        m_tileW = tileW;
        m_tileH = tileH;

        return true;
    }

    // 更新Minimap
    bool UpdateMinimap(int maxSize = 200);

    // 渲染地图到目标（带缩放和偏移）
    void RenderMap(ID2D1DeviceContext* d2dContext, D2D1_RECT_F destRect);

    // 获取Minimap SRV
    ID3D11ShaderResourceView* GetMinimapSRV() const { return minimapSRV.Get(); }
    bool IsMinimapValid() const { return minimapValid; }

    // 获取地图缓存
    ID2D1Bitmap1* GetMapCacheBitmap() const { return mapCacheBitmap.Get(); }
    bool IsMapCacheValid() const { return m_mapCacheValid; }

    // 获取地图尺寸
    int GetMapWidth() const { return m_mapWidth; }
    int GetMapHeight() const { return m_mapHeight; }

private:
    bool m_initialized;
    bool m_mapCacheValid;
    bool minimapValid;

    // 地图缓存
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> mapCacheBitmap;
    int m_mapWidth;
    int m_mapHeight;
    int m_tileW;
    int m_tileH;

    // Minimap
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> minimapBitmap;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> minimapSRV;

    // 设备
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext;
    Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice;
};

#endif
