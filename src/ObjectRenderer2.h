#ifndef OBJECT_RENDERER2_H
#define OBJECT_RENDERER2_H

#include "../COMMON/WinMin.h"
#include <wrl/client.h>
#include <DirectXMath.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <string>
// 此文件弃用
// 包含必要的SPLIB头文件
#include "../SPLIB/CTypePackVector.h"
#include "../SPLIB/CIndexSprite.h"

typedef CTypePackVector<CIndexSprite> CIndexSpritePackVector;

class ObjectRenderer2
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ObjectRenderer2();
    ~ObjectRenderer2();

    // 初始化渲染器，需要D3D设备和上下文
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
                    ID2D1Device* d2dDevice, CIndexSpritePackVector* ispk);

    // 设置要渲染的精灵索引
    void SetSpriteIndex(int index);

    // 获取当前精灵索引
    int GetSpriteIndex() const { return m_spriteIndex; }

    // 设置位置
    void SetPosition(float x, float y);

    // 设置缩放
    void SetScale(float scale);

    // 获取位置
    DirectX::XMFLOAT2 GetPosition() const { return m_position; }

    // 获取缩放
    float GetScale() const { return m_scale; }

    // 获取精灵宽度
    int GetSpriteWidth() const { return m_spriteWidth; }

    // 获取精灵高度
    int GetSpriteHeight() const { return m_spriteHeight; }

    // 获取着色器资源视图（用于ImGui渲染）
    ID3D11ShaderResourceView* GetShaderResourceView() const { return m_shaderResourceView.Get(); }

    // 渲染到ImGui窗口
    void RenderImGui();

    // 使用Direct3D渲染精灵
    void Render();

    // 设置屏幕尺寸（用于正交投影）
    void SetScreenSize(int width, int height);

    // 显示UI控制面板
    void ShowControlPanel(bool* p_open = nullptr);

private:
    // 从当前精灵索引重新创建SRV
    bool CreateShaderResourceViewFromCurrentSprite();

    // Direct3D渲染资源创建
    bool CreateShaders();
    bool CreateGeometry();
    bool CreateConstantBuffer();
    void UpdateConstantBuffer();
    void UpdateOrthoMatrix();

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<ID2D1Device> m_d2dDevice;
    ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;

    CIndexSpritePackVector* m_ispk;  // 指向精灵包向量的指针，不拥有所有权

    int m_spriteIndex;               // 当前精灵索引
    int m_spriteWidth;               // 精灵宽度
    int m_spriteHeight;              // 精灵高度

    DirectX::XMFLOAT2 m_position;    // 位置 (x, y)
    float m_scale;                   // 缩放比例

    // 纹理坐标变换
    DirectX::XMFLOAT2 m_texOffset;   // 纹理坐标偏移
    DirectX::XMFLOAT2 m_texScale;    // 纹理坐标缩放

    // 视口控制
    DirectX::XMFLOAT4 m_viewport;    // x, y, width, height (像素坐标)
    bool m_useCustomViewport;        // 是否使用自定义视口

    // Direct3D渲染资源
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11SamplerState> m_samplerState;
    ComPtr<ID3D11BlendState> m_blendState;
    DirectX::XMFLOAT4X4 m_orthoMatrix;
    DirectX::XMFLOAT4X4 m_worldMatrix;
    int m_screenWidth;
    int m_screenHeight;

    bool m_initialized;              // 是否已初始化
};

#endif // OBJECT_RENDERER2_H