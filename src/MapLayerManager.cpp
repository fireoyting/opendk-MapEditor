#include "MapLayerManager.h"

MapLayerManager::MapLayerManager()
    : m_initialized(false)
    , m_tileLayerValid(false)
    , m_objectLayerValid(false)
{
}

MapLayerManager::~MapLayerManager()
{
    Release();
}

bool MapLayerManager::Initialize(ID2D1DeviceContext* d2dCtx, ID3D11Device* d3dDev)
{
    if (!d2dCtx || !d3dDev)
        return false;

    m_d2dContext = d2dCtx;
    m_d3dDevice = d3dDev;
    m_initialized = true;

    return true;
}

void MapLayerManager::Release()
{
    // 只清理图层数据，不释放D2D/D3D上下文（由外部管理）
    m_layers.clear();
    m_initialized = false;
    m_tileLayerValid = false;
    m_objectLayerValid = false;
}

// 只清理图层数据（用于加载新地图时）
void MapLayerManager::ClearLayers()
{
    m_layers.clear();
    m_tileLayerValid = false;
    m_objectLayerValid = false;
}

bool MapLayerManager::CreateLayer(LayerType type, UINT width, UINT height)
{
    if (!m_initialized || width == 0 || height == 0)
        return false;

    D2D1_SIZE_U bitmapSize = { width, height };
    D2D1_BITMAP_PROPERTIES1 bitmapProps = {};
    bitmapProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmapProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    HRESULT hr = m_d2dContext->CreateBitmap(bitmapSize, nullptr, 0, &bitmapProps, bitmap.GetAddressOf());
    if (FAILED(hr))
        return false;

    m_layers[type] = bitmap;
    return true;
}

ID2D1Bitmap1* MapLayerManager::GetLayer(LayerType type)
{
    auto it = m_layers.find(type);
    if (it != m_layers.end())
        return it->second.Get();
    return nullptr;
}

void MapLayerManager::SetTarget(LayerType type)
{
    auto it = m_layers.find(type);
    if (it != m_layers.end())
    {
        m_d2dContext->SetTarget(it->second.Get());
    }
}

void MapLayerManager::SetTargetBitmap(ID2D1Bitmap1* bitmap)
{
    m_d2dContext->SetTarget(bitmap);
}

void MapLayerManager::Clear(LayerType type, D2D1::ColorF color)
{
    auto it = m_layers.find(type);
    if (it != m_layers.end())
    {
        // 保存当前渲染目标
        Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
        m_d2dContext->GetTarget(&originalTarget);

        m_d2dContext->SetTarget(it->second.Get());
        // 根据图层类型选择清除颜色
        if (type == LayerType::TileLayer)
            m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        else
            m_d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0)); // 透明
        
        // 恢复原始渲染目标
        m_d2dContext->SetTarget(originalTarget.Get());
    }
}

void MapLayerManager::CopyLayer(LayerType dest, LayerType src)
{
    auto itDest = m_layers.find(dest);
    auto itSrc = m_layers.find(src);

    if (itDest == m_layers.end() || itSrc == m_layers.end())
        return;

    // 保存当前渲染目标
    Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
    m_d2dContext->GetTarget(&originalTarget);

    m_d2dContext->SetTarget(itDest->second.Get());
    // 根据目标图层类型选择清除颜色
    if (dest == LayerType::TileLayer)
        m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
    else
        m_d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0)); // 透明

    // 绘制源图层到目标图层
    D2D1_SIZE_U srcSize = itSrc->second->GetPixelSize();
    D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)srcSize.width, (float)srcSize.height);
    D2D1_RECT_F destRect = D2D1::RectF(0, 0, (float)srcSize.width, (float)srcSize.height);

    m_d2dContext->DrawBitmap(itSrc->second.Get(), destRect, 1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);

    // 恢复原始渲染目标
    m_d2dContext->SetTarget(originalTarget.Get());
}

void MapLayerManager::DrawHighlightRectInViewport(float x, float y, float w, float h)
{
    auto it = m_layers.find(LayerType::EditorLayer);
    if (it == m_layers.end() || !it->second)
        return;

    // 保存当前渲染目标
    Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
    m_d2dContext->GetTarget(&originalTarget);

    m_d2dContext->SetTarget(it->second.Get());

    // 清除Editor层（透明背景）
    m_d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0));

    // 创建红色画刷
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> redBrush;
    HRESULT hr = m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f), redBrush.GetAddressOf());
    if (FAILED(hr) || !redBrush)
    {
        // 恢复原始渲染目标
        m_d2dContext->SetTarget(originalTarget.Get());
        return;
    }

    // 绘制矩形边框（3像素宽）
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
    m_d2dContext->DrawRectangle(rect, redBrush.Get(), 3.0f);

    // 恢复原始渲染目标
    m_d2dContext->SetTarget(originalTarget.Get());
}

void MapLayerManager::ClearEditorLayer()
{
    auto it = m_layers.find(LayerType::EditorLayer);
    if (it == m_layers.end() || !it->second)
        return;

    // 保存当前渲染目标
    Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
    m_d2dContext->GetTarget(&originalTarget);

    m_d2dContext->SetTarget(it->second.Get());
    m_d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0));
    
    // 恢复原始渲染目标
    m_d2dContext->SetTarget(originalTarget.Get());
}

void MapLayerManager::CompositeToTarget(ID2D1Bitmap1* targetBitmap)
{
    if (!targetBitmap)
        return;

    // 保存当前渲染目标
    Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
    m_d2dContext->GetTarget(&originalTarget);

    m_d2dContext->SetTarget(targetBitmap);
    m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    // 合成所有图层：Tile -> Object -> Effect
    // 这里按照顺序绘制到目标上
    // 实际项目中可能需要使用透明混合

    // 绘制Tile层
    auto itTile = m_layers.find(LayerType::TileLayer);
    if (itTile != m_layers.end() && itTile->second)
    {
        D2D1_SIZE_U size = itTile->second->GetPixelSize();
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)size.width, (float)size.height);
        D2D1_RECT_F destRect = srcRect;
        m_d2dContext->DrawBitmap(itTile->second.Get(), destRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    }

    // 绘制Object层
    auto itObject = m_layers.find(LayerType::ObjectLayer);
    if (itObject != m_layers.end() && itObject->second)
    {
        D2D1_SIZE_U size = itObject->second->GetPixelSize();
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)size.width, (float)size.height);
        D2D1_RECT_F destRect = srcRect;
        m_d2dContext->DrawBitmap(itObject->second.Get(), destRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    }

    // 绘制Effect层
    auto itEffect = m_layers.find(LayerType::EffectLayer);
    if (itEffect != m_layers.end() && itEffect->second)
    {
        D2D1_SIZE_U size = itEffect->second->GetPixelSize();
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)size.width, (float)size.height);
        D2D1_RECT_F destRect = srcRect;
        m_d2dContext->DrawBitmap(itEffect->second.Get(), destRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    }

    // 绘制Editor层（高亮矩形）
    auto itEditor = m_layers.find(LayerType::EditorLayer);
    if (itEditor != m_layers.end() && itEditor->second)
    {
        D2D1_SIZE_U size = itEditor->second->GetPixelSize();
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)size.width, (float)size.height);
        D2D1_RECT_F destRect = srcRect;
        m_d2dContext->DrawBitmap(itEditor->second.Get(), destRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    }

    // 恢复原始渲染目标
    m_d2dContext->SetTarget(originalTarget.Get());
}
