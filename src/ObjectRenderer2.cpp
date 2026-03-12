#include "ObjectRenderer2.h"
#include "DXTrace.h"
#include <imgui.h>
#include <algorithm>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// 顶点结构
struct VertexPosTex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
};

// 常量缓冲区结构（必须16字节对齐）
struct ConstantBuffer
{
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMFLOAT4 texTransform; // x: offsetX, y: offsetY, z: scaleX, w: scaleY
};

ObjectRenderer2::ObjectRenderer2()
    : m_ispk(nullptr)
    , m_spriteIndex(0)
    , m_spriteWidth(0)
    , m_spriteHeight(0)
    , m_position({0.0f, 0.0f})
    , m_scale(1.0f)
    , m_texOffset({0.0f, 0.0f})
    , m_texScale({1.0f, 1.0f})
    , m_viewport({0.0f, 0.0f, 0.0f, 0.0f})
    , m_useCustomViewport(false)
    , m_screenWidth(0)
    , m_screenHeight(0)
    , m_initialized(false)
{
    // 初始化矩阵为单位矩阵
    DirectX::XMStoreFloat4x4(&m_orthoMatrix, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
}

ObjectRenderer2::~ObjectRenderer2()
{
    // 自动释放ComPtr资源
}

bool ObjectRenderer2::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
                                 ID2D1Device* d2dDevice, CIndexSpritePackVector* ispk)
{
    if (!device || !context || !d2dDevice || !ispk)
        return false;

    m_device = device;
    m_context = context;
    m_d2dDevice = d2dDevice;
    m_ispk = ispk;

    // 设置默认精灵索引为21（与现有代码保持一致）
    SetSpriteIndex(21);

    // 创建Direct3D渲染资源
    if (!CreateShaders())
        OutputDebugString(L"ObjectRenderer2: Failed to create shaders\n");
    if (!CreateGeometry())
        OutputDebugString(L"ObjectRenderer2: Failed to create geometry\n");
    if (!CreateConstantBuffer())
        OutputDebugString(L"ObjectRenderer2: Failed to create constant buffer\n");

    // 创建采样器状态
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HRESULT hr = m_device->CreateSamplerState(&sampDesc, &m_samplerState);
    if (FAILED(hr))
        OutputDebugString(L"ObjectRenderer2: Failed to create sampler state\n");

    // 创建混合状态（启用Alpha混合）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = m_device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr))
        OutputDebugString(L"ObjectRenderer2: Failed to create blend state\n");

    // 假设屏幕尺寸为1280x720（后续可在渲染时更新）
    m_screenWidth = 1280;
    m_screenHeight = 720;
    UpdateOrthoMatrix();

    m_initialized = true;
    return true;
}

void ObjectRenderer2::SetSpriteIndex(int index)
{
    if (!m_ispk || index < 0 || index >= m_ispk->GetSize())
        return;

    m_spriteIndex = index;
    CreateShaderResourceViewFromCurrentSprite();
}

void ObjectRenderer2::SetPosition(float x, float y)
{
    m_position.x = x;
    m_position.y = y;
}

void ObjectRenderer2::SetScale(float scale)
{
    m_scale = std::max(0.1f, scale);  // 防止缩放过小
}

bool ObjectRenderer2::CreateShaderResourceViewFromCurrentSprite()
{
    if (!m_device || !m_context || !m_ispk)
        return false;

    // 获取精灵数据
    CIndexSprite* sprite = m_ispk->GetData(m_spriteIndex);
    if (!sprite)
        return false;

    m_spriteWidth = sprite->GetWidth();
    m_spriteHeight = sprite->GetHeight();

    // 创建纹理描述
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_spriteWidth;
    texDesc.Height = m_spriteHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = m_device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 更新纹理数据
    m_context->UpdateSubresource(tex.Get(), 0, nullptr,
                                 sprite->GetImage(),
                                 m_spriteWidth * sizeof(uint32_t), 0);

    // 创建着色器资源视图描述
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_shaderResourceView.Reset();
    hr = m_device->CreateShaderResourceView(tex.Get(), &srvDesc,
                                           m_shaderResourceView.ReleaseAndGetAddressOf());

    return SUCCEEDED(hr);
}

void ObjectRenderer2::RenderImGui()
{
    if (!m_shaderResourceView || !m_initialized)
        return;

    // 计算渲染大小（应用缩放）
    float renderWidth = m_spriteWidth * m_scale;
    float renderHeight = m_spriteHeight * m_scale;

    // 使用ImGui渲染图像
    ImGui::Image((void*)m_shaderResourceView.Get(),
                 ImVec2(renderWidth, renderHeight));
}

void ObjectRenderer2::ShowControlPanel(bool* p_open)
{
    if (!ImGui::Begin("Object Renderer 2 Controls", p_open))
        return;

    // 精灵索引选择
    if (m_ispk)
    {
        int spriteCount = m_ispk->GetSize();
        if (ImGui::SliderInt("Sprite Index", &m_spriteIndex, 0, spriteCount - 1))
        {
            // 索引改变时重新创建SRV
            CreateShaderResourceViewFromCurrentSprite();
        }
        ImGui::Text("Sprite Size: %d x %d", m_spriteWidth, m_spriteHeight);
    }

    ImGui::Separator();

    // 位置控制
    float pos[2] = { m_position.x, m_position.y };
    if (ImGui::DragFloat2("Position", pos, 1.0f, -1000.0f, 1000.0f, "%.1f"))
    {
        m_position.x = pos[0];
        m_position.y = pos[1];
    }

    // 缩放控制 - 使用滑块
    ImGui::Text("Scale: %.1f%%", m_scale * 100.0f);

    // 滑块控制
    if (ImGui::SliderFloat("##ScaleSlider", &m_scale, 0.1f, 5.0f, "%.2f"))
    {
        m_scale = std::max(0.1f, m_scale);
    }

    // 显示实际尺寸
    int actualWidth = static_cast<int>(m_spriteWidth * m_scale);
    int actualHeight = static_cast<int>(m_spriteHeight * m_scale);
    ImGui::Text("Size: %d x %d pixels", actualWidth, actualHeight);

    ImGui::Separator();

    // 纹理坐标控制
    if (ImGui::CollapsingHeader("Texture Coordinates", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float texOffset[2] = { m_texOffset.x, m_texOffset.y };
        if (ImGui::DragFloat2("Texture Offset", texOffset, 0.01f, -1.0f, 1.0f, "%.3f"))
        {
            m_texOffset.x = texOffset[0];
            m_texOffset.y = texOffset[1];
        }

        float texScale[2] = { m_texScale.x, m_texScale.y };
        if (ImGui::DragFloat2("Texture Scale", texScale, 0.01f, 0.1f, 5.0f, "%.3f"))
        {
            m_texScale.x = std::max(0.1f, texScale[0]);
            m_texScale.y = std::max(0.1f, texScale[1]);
        }

        ImGui::Text("UV Range: (%.3f, %.3f) to (%.3f, %.3f)",
                   m_texOffset.x, m_texOffset.y,
                   m_texOffset.x + m_texScale.x, m_texOffset.y + m_texScale.y);
    }

    // 视口控制
    if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Use Custom Viewport", &m_useCustomViewport);
        if (m_useCustomViewport)
        {
            float viewportValues[4] = { m_viewport.x, m_viewport.y, m_viewport.z, m_viewport.w };
            if (ImGui::DragFloat4("Viewport (x,y,w,h)", viewportValues, 1.0f, 0.0f, 2000.0f, "%.1f"))
            {
                m_viewport.x = viewportValues[0];
                m_viewport.y = viewportValues[1];
                m_viewport.z = std::max(1.0f, viewportValues[2]);
                m_viewport.w = std::max(1.0f, viewportValues[3]);
            }

            ImGui::Text("Viewport: (%.1f, %.1f) - (%.1f, %.1f)",
                       m_viewport.x, m_viewport.y,
                       m_viewport.x + m_viewport.z, m_viewport.y + m_viewport.w);
        }
        else
        {
            ImGui::Text("Using full screen viewport: %d x %d", m_screenWidth, m_screenHeight);
        }
    }

    ImGui::Separator();

    // 预览窗口
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 创建一个可拖拽的窗口区域来预览精灵
        ImGui::BeginChild("PreviewRegion", ImVec2(0, 200), true);

        // 计算中心位置
        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImVec2 imageSize(m_spriteWidth * m_scale, m_spriteHeight * m_scale);

        // 计算渲染位置（考虑偏移）
        ImVec2 renderPos = ImGui::GetCursorScreenPos();
        renderPos.x += m_position.x + regionSize.x * 0.5f - imageSize.x * 0.5f;
        renderPos.y += m_position.y + regionSize.y * 0.5f - imageSize.y * 0.5f;

        // 设置渲染位置
        ImGui::SetCursorScreenPos(renderPos);

        // 渲染图像
        if (m_shaderResourceView)
        {
            ImGui::Image((void*)m_shaderResourceView.Get(), imageSize);
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

void ObjectRenderer2::UpdateOrthoMatrix()
{
    // 正交投影矩阵，原点在左上角，y轴向下，单位：像素
    DirectX::XMMATRIX ortho = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight), 0.0f,
        0.0f, 1.0f);
    DirectX::XMStoreFloat4x4(&m_orthoMatrix, ortho);
}

bool ObjectRenderer2::CreateShaders()
{
    // 编译顶点着色器
    ComPtr<ID3DBlob> vsBlob;
    const char* vsCode = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
            float4 texTransform; // x: offsetX, y: offsetY, z: scaleX, w: scaleY
        };

        struct VertexIn
        {
            float3 pos : POSITION;
            float2 tex : TEXCOORD;
        };

        struct VertexOut
        {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD;
        };

        VertexOut VS(VertexIn vin)
        {
            VertexOut vout;
            vout.pos = mul(float4(vin.pos, 1.0f), World);
            vout.pos = mul(vout.pos, View);
            vout.pos = mul(vout.pos, Projection);
            // 应用纹理坐标变换: tex = tex * scale + offset
            vout.tex = vin.tex * texTransform.zw + texTransform.xy;
            return vout;
        }
    )";

    HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
                            "VS", "vs_5_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                      nullptr, &m_vertexShader);
    if (FAILED(hr))
        return false;

    // 编译像素着色器
    ComPtr<ID3DBlob> psBlob;
    const char* psCode = R"(
        Texture2D tex : register(t0);
        SamplerState sam : register(s0);

        struct VertexOut
        {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD;
        };

        float4 PS(VertexOut pin) : SV_TARGET
        {
            return tex.Sample(sam, pin.tex);
        }
    )";

    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
                    "PS", "ps_5_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr))
        return false;

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                     nullptr, &m_pixelShader);
    if (FAILED(hr))
        return false;

    // 创建输入布局
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = m_device->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc),
                                     vsBlob->GetBufferPointer(),
                                     vsBlob->GetBufferSize(),
                                     &m_inputLayout);
    return SUCCEEDED(hr);
}

bool ObjectRenderer2::CreateGeometry()
{
    // 创建顶点缓冲区（一个四边形）
    VertexPosTex vertices[] = {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } }, // 左下
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } }, // 左上
        { {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } }, // 右上
        { {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } }  // 右下
    };

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices;

    HRESULT hr = m_device->CreateBuffer(&vbd, &vinitData, &m_vertexBuffer);
    if (FAILED(hr))
        return false;

    // 创建索引缓冲区
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices;

    hr = m_device->CreateBuffer(&ibd, &iinitData, &m_indexBuffer);
    return SUCCEEDED(hr);
}

bool ObjectRenderer2::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = 0;

    HRESULT hr = m_device->CreateBuffer(&cbd, nullptr, &m_constantBuffer);
    return SUCCEEDED(hr);
}

void ObjectRenderer2::UpdateConstantBuffer()
{
    if (!m_constantBuffer)
        return;

    // 更新世界矩阵：考虑位置、缩放和精灵尺寸
    float width = m_spriteWidth * m_scale;
    float height = m_spriteHeight * m_scale;
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(width, height, 1.0f) *
                              DirectX::XMMatrixTranslation(m_position.x + halfWidth, m_position.y + halfHeight, 0.0f);

    DirectX::XMStoreFloat4x4(&m_worldMatrix, world);

    ConstantBuffer cb = {};
    // HLSL默认使用列主序矩阵，所以需要转置
    cb.world = DirectX::XMMatrixTranspose(world);
    cb.view = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    cb.projection = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&m_orthoMatrix));
    cb.texTransform = DirectX::XMFLOAT4(m_texOffset.x, m_texOffset.y, m_texScale.x, m_texScale.y);

    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
}

void ObjectRenderer2::Render()
{
    if (!m_initialized || !m_shaderResourceView || !m_vertexBuffer || !m_indexBuffer)
        return;

    // 设置视口
    D3D11_VIEWPORT viewport;
    if (m_useCustomViewport && m_viewport.z > 0 && m_viewport.w > 0)
    {
        viewport.TopLeftX = m_viewport.x;
        viewport.TopLeftY = m_viewport.y;
        viewport.Width = m_viewport.z;
        viewport.Height = m_viewport.w;
    }
    else
    {
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_screenWidth);
        viewport.Height = static_cast<float>(m_screenHeight);
    }
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);

    // 更新常量缓冲区
    UpdateConstantBuffer();

    // 设置顶点缓冲区
    UINT stride = sizeof(VertexPosTex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_inputLayout.Get());

    // 设置着色器
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // 设置常量缓冲区
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // 设置纹理和采样器
    m_context->PSSetShaderResources(0, 1, m_shaderResourceView.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // 设置混合状态
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);

    // 绘制
    m_context->DrawIndexed(6, 0, 0);
}

void ObjectRenderer2::SetScreenSize(int width, int height)
{
    if (width != m_screenWidth || height != m_screenHeight)
    {
        m_screenWidth = width;
        m_screenHeight = height;
        UpdateOrthoMatrix();
    }
}