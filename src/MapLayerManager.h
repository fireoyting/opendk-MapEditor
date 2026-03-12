#ifndef MAP_LAYER_MANAGER_H
#define MAP_LAYER_MANAGER_H

#include "../COMMON/WinMin.h"
#include <wrl/client.h>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "../maplib/Zone.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

enum class LayerType
{
    TileLayer,      // 静态Tile层（完全不变）
    ObjectLayer,    // 静态Object层（墙壁等，可能有遮盖）
    EffectLayer,    // 动态Effect层（每帧更新）
    EditorLayer,    // 编辑器高亮层（显示选中pattern）
    FinalLayer      // 最终呈现层
};

// LayerType的哈希函数
struct LayerTypeHash
{
    std::size_t operator()(LayerType type) const noexcept
    {
        return static_cast<std::size_t>(type);
    }
};

class MapLayerManager
{
public:
    MapLayerManager();
    ~MapLayerManager();

    // 初始化
    bool Initialize(ID2D1DeviceContext* d2dContext, ID3D11Device* d3dDevice);

    // 释放资源
    void Release();

    // 检查是否已初始化
    bool IsInitialized() const { return m_initialized; }

    // 只清理图层数据，不释放D2D上下文（用于加载新地图时）
    void ClearLayers();

    // 创建或调整图层大小
    bool CreateLayer(LayerType type, UINT width, UINT height);

    // 获取图层
    ID2D1Bitmap1* GetLayer(LayerType type);

    // 设置当前绘制目标
    void SetTarget(LayerType type);
    void SetTargetBitmap(ID2D1Bitmap1* bitmap);

    // 清除图层
    void Clear(LayerType type, D2D1::ColorF color = D2D1::ColorF::Black);

    // 复制图层（用于合成）
    void CopyLayer(LayerType dest, LayerType src);

    // 合成所有图层到目标
    void CompositeToTarget(ID2D1Bitmap1* targetBitmap);

    // 更新Tile层（静态，只需更新一次）
    // 注意：此函数会设置target到TileLayer，需要BeginDraw/EndDraw
    template<typename TTextureManager>
    bool UpdateTileLayer(Zone& zone, TTextureManager* texManager, int tileW = 48, int tileH = 24)
    {
        if (!m_initialized || !m_layers[LayerType::TileLayer])
            return false;

        uint32_t mapWidth = zone.GetWidth();
        uint32_t mapHeight = zone.GetHeight();
        if (mapWidth == 0 || mapHeight == 0)
            return false;

        // 保存当前渲染目标
        Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
        m_d2dContext->GetTarget(&originalTarget);

        // 设置目标为Tile层
        m_d2dContext->SetTarget(m_layers[LayerType::TileLayer].Get());
        m_d2dContext->BeginDraw();
        m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        // 绘制所有tile
        int drawnCount = 0;
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
                        D2D1_RECT_F tileDest = D2D1::RectF(drawX, drawY, drawX + tileW, drawY + tileH);
                        m_d2dContext->DrawBitmap(sprite, tileDest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                        drawnCount++;
                    }
                }
            }
        }

        HRESULT hr = m_d2dContext->EndDraw();
        
        // 恢复原始渲染目标
        if (originalTarget)
            m_d2dContext->SetTarget(originalTarget.Get());
        else
            m_d2dContext->SetTarget(nullptr);

        m_tileLayerValid = (SUCCEEDED(hr) && drawnCount > 0);
        return m_tileLayerValid;
    }

    // 检查Tile层是否有效
    bool IsTileLayerValid() const { return m_tileLayerValid; }

    // 检查Object层是否有效
    bool IsObjectLayerValid() const { return m_objectLayerValid; }

    // 使Tile层无效（用于地图切换时）
    void InvalidateTileLayer() { m_tileLayerValid = false; }

    // 使Object层无效（用于地图切换时）
    void InvalidateObjectLayer() { m_objectLayerValid = false; }

    // 更新Object层（绘制ImageObject）
    // 注意：此函数会设置target到ObjectLayer，需要BeginDraw/EndDraw
    // imageObjectStartIndex: ImageObject.spk 的起始纹理索引（用于计算全局索引）
    template<typename TTextureManager>
    bool UpdateObjectLayer(Zone& zone, TTextureManager* texManager, UINT32 imageObjectStartIndex, int tileW = 48, int tileH = 24)
    {
        if (!m_initialized || !m_layers[LayerType::ObjectLayer])
            return false;

        uint32_t mapWidth = zone.GetWidth();
        uint32_t mapHeight = zone.GetHeight();
        if (mapWidth == 0 || mapHeight == 0)
            return false;

        // 保存当前渲染目标
        Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
        m_d2dContext->GetTarget(&originalTarget);

        // 设置目标为Object层
        m_d2dContext->SetTarget(m_layers[LayerType::ObjectLayer].Get());
        m_d2dContext->BeginDraw();
        // 透明背景
        m_d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0));

        // 使用 viewpoint 分组绘制（MImageObject 包含像素坐标）
        const auto& objectsByViewpoint = zone.GetImageObjectsByViewpoint();

        int drawnCount = 0;
        int missingTexture = 0;

        // 按 viewpoint 从低到高绘制
        for (const auto& vpPair : objectsByViewpoint)
        {
            const auto& imgObjects = vpPair.second;
            for (const MImageObject* imgObj : imgObjects)
            {
                uint32_t spriteID = imgObj->GetSpriteID();
                UINT32 globalIndex = imageObjectStartIndex + spriteID;
                auto sprite = texManager->GetTexture(globalIndex);
                if (!sprite) {
                    missingTexture++;
                    continue;
                }

                // 使用 MImageObject 自身的像素坐标
                float drawX = (float)imgObj->GetPixelX();
                float drawY = (float)imgObj->GetPixelY();

                D2D1_SIZE_U size = sprite->GetPixelSize();
                D2D1_RECT_F objDest = D2D1::RectF(drawX, drawY, drawX + size.width, drawY + size.height);

                m_d2dContext->DrawBitmap(sprite, objDest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                drawnCount++;
            }
        }

        HRESULT hr = m_d2dContext->EndDraw();

        // 恢复原始渲染目标
        if (originalTarget)
            m_d2dContext->SetTarget(originalTarget.Get());
        else
            m_d2dContext->SetTarget(nullptr);

        m_objectLayerValid = (SUCCEEDED(hr) && drawnCount >= 0);
        return m_objectLayerValid;
    }

    // 绘制高亮矩形到Editor层（视口大小）
    void DrawHighlightRectInViewport(float x, float y, float w, float h);

    // 清除Editor层
    void ClearEditorLayer();

private:
    bool m_initialized;
    bool m_tileLayerValid;
    bool m_objectLayerValid;

    // 所有图层
    std::unordered_map<LayerType, Microsoft::WRL::ComPtr<ID2D1Bitmap1>, LayerTypeHash> m_layers;

    // 设备引用
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
};

#endif
