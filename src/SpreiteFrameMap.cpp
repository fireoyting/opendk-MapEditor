#include "SpreiteFrameMap.h"
#include"DXTrace.h"
using namespace Microsoft::WRL;

namespace
{
    // TextureManager单例
    Texture2DManager* s_pInstance = nullptr;
}
Texture2DManager::Texture2DManager()
{
    if (s_pInstance)
        throw std::exception("TextureManager is a singleton!");
    s_pInstance = this;
}

Texture2DManager::~Texture2DManager()
{

}
Texture2DManager& Texture2DManager::Get()
{
    if (!s_pInstance)
        throw std::exception("TextureManager needs an instance!");
    return *s_pInstance;
}
void Texture2DManager::Init(ID2D1Device* device)
{
    m_pDevice = device;
    m_pDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_pD2dContext.GetAddressOf());
    BYTE imageData[4] = { 0, 0, 0, 0 };

    // 创建一个位图大小为1x1的结构体
    D2D1_SIZE_U bitmapSize = D2D1::SizeU(1, 1);

    // 创建一个位图属性结构体，指定像素格式和DPI
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f
    );
    ComPtr<ID2D1Bitmap1> tex;

    m_pD2dContext->CreateBitmap(
    bitmapSize, // Bitmap size
    imageData, // Image data
    1 * sizeof(uint32_t), // Image data pitch
    &bitmapProperties, // Bitmap properties
    tex.GetAddressOf()
    );
   

   
    m_TextureSRVs.try_emplace(65536, tex.Get()).second;


}

ID2D1Bitmap1* Texture2DManager::CreateFromMemory(UINT32 name, void* data, D2D1_SIZE_U  byteWidth, bool enableMips, bool forceSRGB)
{

    if (m_TextureSRVs.count(name))
        return m_TextureSRVs[name].Get();//已存在就直接返回

    auto& res = m_TextureSRVs[name];

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f
    );



    m_pD2dContext->CreateBitmap(
        byteWidth, // Bitmap size
        data, // Image data
        byteWidth.width * sizeof(uint32_t), // Image data pitch
        &bitmapProperties, // Bitmap properties
        res.GetAddressOf()
    );


   

    return res.Get();
}
bool Texture2DManager::AddTexture(UINT32 name, ID2D1Bitmap1* texture)
{

    return m_TextureSRVs.try_emplace(name, texture).second;
}

void Texture2DManager::RemoveTexture(UINT32 name)
{

    m_TextureSRVs.erase(name);
}

ID2D1Bitmap1* Texture2DManager::GetTexture(UINT32 filename)
{

    if (m_TextureSRVs.count(filename))
        return m_TextureSRVs[filename].Get();
    return nullptr;
}

ID2D1Bitmap1* Texture2DManager::GetNullTexture()
{
    return m_TextureSRVs[65536].Get();
}
