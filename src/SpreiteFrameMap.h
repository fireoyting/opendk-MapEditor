#ifndef TEXTURE2D_GAME
#define TEXTURE2D_GAME
#include "WinMin.h"
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <wrl/client.h>
#include <unordered_map>

/*
这里我们创建d2d1bitmap1
由于项目目前主要以D2D绘制
预留接口给后续更换。

*/
class Texture2DManager
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;
    Texture2DManager();
    ~Texture2DManager();
    Texture2DManager(Texture2DManager&) = delete;//不允许拷贝，允许移动
    Texture2DManager& operator=(const Texture2DManager&) = delete;
    Texture2DManager(Texture2DManager&&) = default;
    Texture2DManager& operator=(Texture2DManager&&) = default;

    static Texture2DManager& Get();
    void Init(ID2D1Device* device);
    ID2D1Bitmap1* CreateFromMemory(UINT32 name, void* data, D2D1_SIZE_U  byteWidth, bool enableMips = false, bool forceSRGB = false);
    bool AddTexture(UINT32 name, ID2D1Bitmap1* texture);
    void RemoveTexture(UINT32 name);
    ID2D1Bitmap1* GetTexture(UINT32 filename);
    ID2D1Bitmap1* GetNullTexture();
    size_t GetTextureCount() const { return m_TextureSRVs.size(); }

private:


    ComPtr<ID2D1Device> m_pDevice;
    ComPtr<ID2D1DeviceContext> m_pD2dContext;
    std::unordered_map<UINT32, ComPtr<ID2D1Bitmap1>> m_TextureSRVs;
};
#endif