#include "gameapp.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "stb_image.h"
#include <Windows.h>
#include "ShlObj.h"
#include "CommDlg.h"
#include "DXTrace.h"
#include "TableGui.h"
#include <algorithm>
#include <set>
/*
2023 11 18 代办事项
调整UI布局,
把图层抽象出来.使用组合模式

做一个专门的 transform栏.比例 和 偏移值, 

*/

/*
2024 02 24 事项进度
调整UI布局--- 进行中
把图层抽象出来.使用组合模式 -- 已经更新Texture2DManager m_TextureManager;
                                        CAnimationManager m_animationManager;
                                        两个单例类, 现在在Table中可以很好的传递信号

做一个专门的 transform栏.缩放比例 和 偏移值,

*/

extern uint32_t ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// 获取全局纹理索引
UINT32 GameApp::GetGlobalTextureIndex(SPKType type, UINT32 localIndex)
{
    auto it = m_SPKFiles.find(type);
    if (it == m_SPKFiles.end())
    {
        OutputDebugStringA("[Error] SPKType not found in m_SPKFiles\n");
        return 0;
    }

    return it->second.startIndex + localIndex;
}

// 验证纹理索引是否有效
bool GameApp::ValidateTextureIndex(SPKType type, UINT32 localIndex)
{
    auto it = m_SPKFiles.find(type);
    if (it == m_SPKFiles.end())
    {
        OutputDebugStringA("[Error] SPKType not found in m_SPKFiles\n");
        return false;
    }

    if (!it->second.isLoaded)
    {
        OutputDebugStringA("[Warning] SPK file not loaded yet\n");
        return false;
    }

    if (localIndex >= it->second.count)
    {
        char buf[256];
        sprintf(buf, "[Error] Local index %u out of range [0, %u] for SPK type\n", localIndex, it->second.count);
        OutputDebugStringA(buf);
        return false;
    }

    return true;
}

static const char* g_CreatureNames[9] =
{
"Body", "pants", "hair",
"Gun", "knife", "sword",
"Scepter", "Shield", "Motorcycle"
};

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D2DApp(hInstance, windowName, initWidth, initHeight), m_bLoading(false), m_bMapLoading(false), m_LoadingProgress(0.0f), m_bMapReady(false), m_bMapLayersDirty(false),
      m_bSpritesLoading(false), m_bSpritesLoaded(false), m_bPendingTileLayerUpdate(false),
      m_showImageObjectList(false), m_selectedImageObjectID(0), m_pendingPreviewObjectID(0), m_showPreviewWindow(false), m_showImagePreviewWindow(false),
      m_showSpriteDebugWindow(false), m_debugSpriteID(0),
      m_selectedSectorIndex(-1),
      m_ViewportX(0), m_ViewportY(0), m_PrevViewportX(-1), m_PrevViewportY(-1), m_PrevZoomLevel(1.0f), m_ZoomLevel(1.0f), m_TileWidth(48), m_TileHeight(24),
      m_ViewportMoveTimer(0), m_ViewportMoveDelay(0.15f)  // 每秒约6-7格
{
    // 初始化SPK文件元数据
    m_SPKFiles[SPKType::ImageObject] = { L"ImageObject.spk", 0, 0, false };
    m_SPKFiles[SPKType::Tile] = { L"tile.spk", 0, 0, false };
}

GameApp::~GameApp()
{
    // 等待map加载线程结束
    if (m_MapLoadThread.joinable())
    {
        m_MapLoadThread.join();
    }

    // 清理maplib
    m_Zone.Release();

    m_pISPK->Release();
    m_pSPK->Release();
    m_CFPK.Release();
}

bool GameApp::Init()
{
    if (!D2DApp::Init())
        return false;
    m_scale = 1.0f;
    m_Skew_X = 1;
    m_Skew_Y = 2;
    //D2DApp::SetFrameRate(24);
    Keepitforever = false;
    m_position = D2D1_POINT_2F{ 0.0f, 0.0f };
    m_offset = D2D1_POINT_2F{ 0.0f, 0.0f };
    m_ShadowOffset = D2D1_POINT_2F{ 0.0f, 0.0f };

    // 获取程序所在目录和 Texture 目录路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    // 去掉文件名，获取目录
    wchar_t* lastBackslash = wcsrchr(exePath, L'\\');
    if (lastBackslash) *lastBackslash = L'\0';
    // 检查是否存在 Texture 子目录
    wchar_t texPath[MAX_PATH];
    wcscpy_s(texPath, MAX_PATH, exePath);
    wcscat_s(texPath, MAX_PATH, L"\\Texture");
    if (GetFileAttributesW(texPath) != INVALID_FILE_ATTRIBUTES) {
        m_textureDirectory = texPath;
    } else {
        // 如果 Texture 不在程序目录下，尝试 build 目录
        wcscpy_s(texPath, MAX_PATH, exePath);
        wcscat_s(texPath, MAX_PATH, L"\\..\\Texture");
        if (GetFileAttributesW(texPath) != INVALID_FILE_ATTRIBUTES) {
            // 转换为绝对路径
            wchar_t absPath[MAX_PATH];
            GetFullPathNameW(texPath, MAX_PATH, absPath, NULL);
            m_textureDirectory = absPath;
        } else {
            // 回退到相对路径
            m_textureDirectory = L"../Texture";
        }
    }
    char debugBuf[256];
    sprintf(debugBuf, "Texture directory: %ls\n", m_textureDirectory.c_str());
    OutputDebugStringA(debugBuf);

    m_TextureManager.Init(m_pD2DDevice.Get());

    // 初始化MapLayerManager
    if (!m_MapLayerManager.Initialize(m_pd2dImmediateContext.Get(), m_pd3dDevice.Get()))
    {
        OutputDebugStringA("[Error] Failed to initialize MapLayerManager\n");
        return false;
    }

    m_animationManager.Init();//初始化
    if (!InitResource())
        return false;

    ImGuiTree m_guitree;
    return true;
}

void GameApp::OnResize()
{
    
    m_pBlackColorBrush.Reset();
    m_pColorBrush.Reset();//重置画刷
    D2DApp::OnResize();
    HR(m_pd2dImmediateContext->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        m_pColorBrush.GetAddressOf()));

    HR(m_pd2dImmediateContext->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        m_pBlackColorBrush.GetAddressOf()));
}

void GameApp::UpdateScene(float dt)
{
    static ImGuiTree& m_guitree = ImGuiTree::Get();
    static float totDeltaTime = 0;//计算动画播放速度, 这边按游戏每秒24帧
    static bool p_open = true;
    static std::shared_ptr<CAnimation> temporaryAnimationFrame = m_animationManager.GetAnimation("empty");
    static std::shared_ptr<CFrame> temporaryFrame = temporaryAnimationFrame->GetFrame(0);
    totDeltaTime += dt;
    // 获取IO事件
    ImGuiIO& io = ImGui::GetIO();
    static float tx = 0.0f, ty = 0.0f;

    ShowMainGui();//先给m_CFPK_select.top等赋值

    // 处理键盘移动视口
    if (m_bMapReady)
    {
        HandleKeyboardInput();
    }

    // 检查视口是否变化，如果是则重新渲染到CLOBitmap
    bool viewportChanged = (m_ViewportX != m_PrevViewportX || m_ViewportY != m_PrevViewportY || m_ZoomLevel != m_PrevZoomLevel);
    if (m_bMapReady && (viewportChanged || m_bMapLayersDirty))
    {
        // 视口像素偏移
        float viewportOffsetX = (float)m_ViewportX * m_TileWidth;
        float viewportOffsetY = (float)m_ViewportY * m_TileHeight;

        // 视口像素大小（带缩放）
        float viewportPixelW = 1280.0f / m_ZoomLevel;
        float viewportPixelH = 720.0f / m_ZoomLevel;

        // 渲染从全尺寸地图裁剪的视口到CLOBitmap
        m_pd2dImmediateContext->SetTarget(d2dCLOBitmap.Get());
        m_pd2dImmediateContext->BeginDraw();
        m_pd2dImmediateContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        // 绘制map层（从全尺寸裁剪到CLOBitmap）
        DrawMapLayersToTarget(d2dCLOBitmap.Get());

        m_pd2dImmediateContext->EndDraw();

        // 更新上一帧视口状态
        m_PrevViewportX = m_ViewportX;
        m_PrevViewportY = m_ViewportY;
        m_PrevZoomLevel = m_ZoomLevel;
        m_bMapLayersDirty = false;
    }

    // 检查map是否加载完成，如果是则加载需要的sprite
    char buf[256];
    sprintf(buf, "[Debug] UpdateScene: ready=%d, mapLoading=%d, spriteLoading=%d, spriteLoaded=%d, zone=%ux%u\n",
        m_bMapReady ? 1 : 0, m_bMapLoading ? 1 : 0,
        m_bSpritesLoading ? 1 : 0, m_bSpritesLoaded ? 1 : 0,
        m_Zone.GetWidth(), m_Zone.GetHeight());
    OutputDebugStringA(buf);

    // 当map已加载且sprite也加载完成后，才设置m_bMapReady为true
    // 需要同时满足：map没有在加载、sprite没有在加载、sprite已加载完成
    if (!m_bMapReady && !m_bMapLoading && m_Zone.GetWidth() > 0 && !m_bSpritesLoading && m_bSpritesLoaded)
    {
        sprintf(buf, "[Debug] UpdateScene: map and sprites ready, enabling render\n");
        OutputDebugStringA(buf);
        m_bMapReady = true;
    }

    // 检查是否需要在主线程更新地图图层（D2D渲染必须在主线程）
    // 需要同时满足：sprite 加载已完成
    if (m_bPendingTileLayerUpdate && !m_bMapLoading && !m_bSpritesLoading && m_bSpritesLoaded)
    {
        sprintf(buf, "[Debug] UpdateScene: calling UpdateMapTileLayer in main thread\n");
        OutputDebugStringA(buf);
        UpdateMapTileLayer();
        m_bPendingTileLayerUpdate = false;
        m_bMapLayersDirty = true;
    }

    // 检查是否需要渲染预览 bitmap（按需渲染）
    // 检查 SPK 是否已加载（通过检查 m_pISPK 是否有数据）
    bool spkLoaded = m_pISPK && m_pISPK->GetSize() > 0;
    if (m_pendingPreviewObjectID != 0 && spkLoaded)
    {
        // 调用渲染函数生成预览 bitmap
        RenderCompositePreview(m_pendingPreviewObjectID);
        // 清空待渲染标记
        m_pendingPreviewObjectID = 0;
    }

    ////ShowCpkEditGui(&p_open); //刷新CFPKEDIT

    ////FRAME_ARRAY& FA = m_CFPK[m_CurrFramePack][m_CFPK_select.top][m_CFPK_select.right];
    ////D2D1_RECT_F desRc = m_sourceRect;
    ////DrawRectangle(&m_sourceRect);

    //m_pd2dImmediateContext->SetTarget(d2dSpriteBitmap.Get());

    //m_pd2dImmediateContext->BeginDraw();
    //m_pd2dImmediateContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    //if (m_CurrMode == ShowMode::Staticdisplay) {
    //    
    //    m_RoleRect = m_sourceRect;
    //    std::shared_ptr<CAnimation> animation = m_animationManager.GetAnimation("Frame");


    //        if (animation && m_guitree.isPlay()) {
    //            // Animation found, can safely use animation pointers

    //            int count = animation->getSize();
    //            
    //            while (totDeltaTime > 1.0f / 24)
    //            {
    //                totDeltaTime -= 1.0f / 24;
    //                m_CurrFrame = (m_CurrFrame + 1) % count;
    //            }
    //            temporaryFrame = animation->GetFrame(m_CurrFrame);
    //            DrawSprite(temporaryFrame);
    //        }
    //        else {
    //            // Animation not found, processing error or returning error message
    //            temporaryFrame = temporaryAnimationFrame->GetFrame(0);
    //            if (temporaryFrame->GetSpriteID() != 0) {

    //                if (!ImGui::IsAnyItemActive())
    //                {
    //                    // 鼠标左键拖动平移
    //                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    //                    {
    //                        tx = 0;
    //                        ty = 0;
    //                        tx += io.MouseDelta.x;
    //                        ty += io.MouseDelta.y;
    //                        temporaryFrame->Set(temporaryFrame->GetSpriteID(), temporaryFrame->GetCX() + tx, temporaryFrame->GetCY() + ty);
    //                    }

    //                    
    //                }


    //                DrawSprite(temporaryFrame);
    //            }
    //        }
    //    

    //}
    //if (m_CurrMode == ShowMode::PlayCreature) 
    //{
    //    std::vector<D2D1_RECT_F> rects = {};
    //    //绘制选中的部件
    //    if (!my_selection.empty()) {
    //        if (m_guitree.isPlay()) {
    //            if (totDeltaTime > 1.0f / 24)
    //            {
    //                totDeltaTime -= 1.0f / 24;
    //                m_CurrFrame = (m_CurrFrame + 1) % m_DrawingFrameSizeMin;
    //            }

    //        }
    //       
    //        rects.resize(my_selection.size());
    //        int step_i = 0;
    //        for (const auto& element : my_selection) {
    //            std::string str(g_CreatureNames[element]);
    //            std::shared_ptr<CAnimation> animation = m_animationManager.GetAnimation(str);
    //            if (animation) {
    //                // Animation found, can safely use animation pointers
    //                temporaryFrame = animation->GetFrame(m_CurrFrame);
    //                DrawSprite(temporaryFrame);

    //                rects[step_i] = D2D1_RECT_F(m_destRect);
    //                
    //            }
    //            step_i++;


    //        }
    //        auto min_x_max_x = std::minmax_element(rects.begin(), rects.end(), [](const D2D1_RECT_F& a, const D2D1_RECT_F& b) { return a.left < b.left; });
    //        auto min_y_max_y = std::minmax_element(rects.begin(), rects.end(), [](const D2D1_RECT_F& a, const D2D1_RECT_F& b) { return a.top < b.top; });
    //        m_RoleRect = D2D1::RectF(min_x_max_x.first->left, min_y_max_y.first->top, min_x_max_x.second->right, min_y_max_y.second->bottom);
    //    }

   
    //
    //
    //}

    //if (checkNUM == 1) {
    //    EditEffect();
    //}
    //m_pd2dImmediateContext->EndDraw();
    //

    //m_pd2dImmediateContext->SetTarget(d2dCLOBitmap.Get());
    //m_pd2dImmediateContext->BeginDraw();
    //if (checkNUM == 1) {
    //    m_pd2dImmediateContext->DrawRectangle(m_ShadowRect, m_pColorBrush.Get(), 2.0f);
    //    m_pd2dImmediateContext->DrawBitmap(ShadowBitmap.Get(), 0, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, 0);//画影子
    //}
    //
    //m_pd2dImmediateContext->DrawBitmap(d2dSpriteBitmap.Get(), m_sourceRect, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_sourceRect);// 再画人
    //m_pd2dImmediateContext->EndDraw();
}

void GameApp::DrawEffect(FRAME_ARRAY& FA)
{
   
   
    // Draw the image

    m_position = D2D1_POINT_2F{ static_cast<float>(FA[m_CurrFrame].GetCX()), static_cast<float>(FA[m_CurrFrame].GetCY()) };
    CFrame temp = FA[m_CurrFrame];
    int sprid = temp.GetSpriteID();
    ComPtr<ID2D1Bitmap1> sprite;
    sprite = m_TextureManager.GetTexture(sprid);
    if (sprite != nullptr)
    {
        D2D1_SIZE_F bitmapSize = sprite->GetSize();
        m_destRect.left = 650 + m_position.x;
        m_destRect.top = 300 + m_position.y;
        m_destRect.right = m_destRect.left + bitmapSize.width;
        m_destRect.bottom = m_destRect.top + bitmapSize.height;
        m_pd2dImmediateContext->DrawBitmap(sprite.Get(), m_destRect);
    }
       







}
/// <summary>
///  信号槽函数
/// </summary>
/// <param name="Number">信号传来的CFPK序号</param>
/// <param name="FA">当前完整的CFPK</param>
void GameApp::DrawSprite(std::shared_ptr<CFrame> pFrame3)
{
    m_position = D2D1_POINT_2F{ 650 + static_cast<float>(pFrame3->GetCX()), 450+static_cast<float>(pFrame3->GetCY()) };
    int sprid = pFrame3->GetSpriteID();
    ComPtr<ID2D1Bitmap1> sprite;
    sprite = m_TextureManager.GetTexture(sprid);
    if (sprite != nullptr)
    {
        D2D1_SIZE_F bitmapSize = sprite->GetSize();
        // 创建变换矩阵

        D2D1_MATRIX_3X2_F transform;
        transform = D2D1::Matrix3x2F::Translation(m_offset.x, m_offset.y) *
            D2D1::Matrix3x2F::Scale(m_scale, m_scale, m_position);


        // 应用变换矩阵
        m_pd2dImmediateContext->SetTransform(transform);

        // 计算变换后的目标矩形

        m_destRect.left =  m_position.x ;
        m_destRect.top = m_position.y ;
        m_destRect.right = m_destRect.left + bitmapSize.width * m_scale;
        m_destRect.bottom = m_destRect.top + bitmapSize.height * m_scale;

        m_pd2dImmediateContext->DrawBitmap(sprite.Get(), m_destRect);

        if (checkNUM != 1) {
            m_pd2dImmediateContext->DrawRectangle(m_destRect, m_pColorBrush.Get(), 1.0f);
        }
      

        m_pd2dImmediateContext->SetTransform(D2D1::Matrix3x2F::Identity());
    }

}


void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    D2D1_RECT_F desRc = D2D1::RectF(0, 0, 1280, 720);
    D2D1_RECT_F my_desRc = D2D1::RectF(450, 200, 850, 600);

    // 绘制到屏幕（此时CLOBitmap已在UpdateScene中渲染好视口内容）
    m_pd2dImmediateContext->SetTarget(m_pD2DTargetBimtap.Get());
    m_pd2dImmediateContext->BeginDraw();
    m_pd2dImmediateContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    // 直接绘制CLOBitmap到屏幕（CLOBitmap已包含裁剪后的视口内容）
    if (m_bMapReady && d2dCLOBitmap)
    {
        D2D1_RECT_F destRect = D2D1::RectF(0, 0, 1280, 720);
        m_pd2dImmediateContext->DrawBitmap(d2dCLOBitmap.Get(), destRect);
    }

    // 绘制选中的 sector 高亮矩形（每帧实时绘制）
    DrawSelectedSectorsHighlight();

    m_pd2dImmediateContext->EndDraw();

    ImGui::Render();
    // 下面这句话会触发ImGui在Direct3D的绘制
    // 因此需要在此之前将后备缓冲区绑定到渲染管线上
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


    m_pSwapChain->Present(0, 0);
}
void GameApp::EditEffect() 
{
   
    // The input rectangle.  150 x 150 pixels with 10 pixel padding
    //D2D1_VECTOR_4F inputRect = D2D1::Vector4F(desRc->left, desRc->right, desRc->top, desRc->bottom);
    m_pd2dImmediateContext->BeginDraw();
    m_pd2dImmediateContext->SetTarget(ShadowBitmap.Get());
    m_pd2dImmediateContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));



    D2D1_VECTOR_4F shadowColor = D2D1::Vector4F(0.0f, 0.0f, 0.0f, 1.0f);



   

    ComPtr<ID2D1Effect> floodEffect;
    m_pd2dImmediateContext->CreateEffect(CLSID_D2D1Flood, &floodEffect);

    floodEffect->SetValue(D2D1_FLOOD_PROP_COLOR, D2D1::Vector4F(0.0f, 0.0f, 0.0f, 0.0f));
    ComPtr<ID2D1Effect> affineTransformEffect;
    m_pd2dImmediateContext->CreateEffect(CLSID_D2D12DAffineTransform, &affineTransformEffect);

    affineTransformEffect->SetInput(0, d2dSpriteBitmap.Get());
    


    D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Translation(m_ShadowOffset.x, m_ShadowOffset.y) * D2D1::Matrix3x2F::Rotation(m_Rotation,D2D1::Point2F(650, 450)) *
        D2D1::Matrix3x2F::Skew(m_Skew_X, m_Skew_Y,D2D1::Point2F(650, 450))* D2D1::Matrix3x2F::Scale(0.8f, 0.7f, D2D1_POINT_2F{m_RoleRect.left,m_RoleRect.bottom});;

    affineTransformEffect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, matrix);
    m_pEffect->SetInputEffect(0, affineTransformEffect.Get());
    m_pEffect->SetValue(D2D1_SHADOW_PROP_COLOR, shadowColor);
    m_pEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, 0.0f);
    ComPtr<ID2D1Effect> compositeEffect;
    m_pd2dImmediateContext->CreateEffect(CLSID_D2D1Composite, &compositeEffect);

    compositeEffect->SetInputEffect(0, floodEffect.Get());
    

    compositeEffect->SetInputEffect(1, m_pEffect.Get());
    //compositeEffect->SetInput(2,d2dSpriteBitmap.Get());
    //compositeEffect->SetInput(3, d2dSpriteBitmap.Get());D2D1_COMPOSITE_MODE_SOURCE_OUT
    compositeEffect->SetValue(D2D1_COMPOSITE_PROP_MODE, D2D1_COMPOSITE_MODE_SOURCE_OUT);

    m_pd2dImmediateContext->DrawImage(m_pEffect.Get());
    m_pd2dImmediateContext->EndDraw();
    

}
void GameApp::ShowMainGui()
{
    //IMGUI 自定义窗口设置
    static ImGuiTree& m_guitree = ImGuiTree::Get();
    static bool p_open = true;
    ImGui::Begin("Sprite editor-by pikachu");

    // Map文件选择区域
    ImGui::SeparatorText("Map File");
    if (ImGui::Button("Select Map File"))
    {
        // 使用Windows文件选择对话框
        OPENFILENAMEW ofn;
        wchar_t szFile[MAX_PATH] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = L"Map Files (*.map)\0*.map\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileNameW(&ofn))
        {
            // 用户选择了文件，触发异步加载
            AsyncLoadMapFile(szFile);
        }
        else
        {
            // 用户取消了选择
            DWORD err = CommDlgExtendedError();
            if (err != 0)
            {
                OutputDebugStringA("[Error] File dialog error\n");
            }
        }
    }

    // 显示当前选中的map文件路径
    ImGui::Text("Current Map: ");
    ImGui::SameLine();
    if (m_MapFilePath.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No file selected");
    }
    else
    {
        // 将wstring转换为string显示
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, m_MapFilePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Path(bufferSize, 0);
        WideCharToMultiByte(CP_UTF8, 0, m_MapFilePath.c_str(), -1, &utf8Path[0], bufferSize, nullptr, nullptr);
        ImGui::Text("%s", utf8Path.c_str());
    }

    // 显示 ImageObject 列表按钮
   
    if (ImGui::Button("ImageObject List")) {
        m_showImageObjectList = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Sprite Debug")) {
        m_showSpriteDebugWindow = true;
    }

    // 显示map加载状态
    if (m_bMapLoading)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loading...");
    }

    //static int curr_mode_item = static_cast<int>(m_CurrMode);
    //static bool p_open = false;

    //const char* mode_strs[] = {
    //    "SPDM-Single part debugging",//Single part debugging mode
    //    "Animation_MODE",
    //    "Overall mode"
    //};
    //if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
    //{
    //    if (curr_mode_item == 0)
    //    {
    //        // 播放单张
    //        m_CurrMode = ShowMode::Staticdisplay;

    //    }
    //    else if (curr_mode_item == 1)
    //    {
    //        // 播放单部位动画
    //        m_CurrMode = ShowMode::PlayAnim;
    //    }
    //    else if (curr_mode_item == 2)
    //    {
    //        // 播放整体动画
    //        m_CurrMode = ShowMode::PlayCreature;
    //    }
    //}
    //ImGui::InputFloat("Dir", &m_CFPK_select.left);
    //ImGui::SameLine();
    ////float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    //ImGui::PushButtonRepeat(true);
    //if (ImGui::ArrowButton("##left", ImGuiDir_Left))
    //{
    //    if (m_CFPK_select.left > 0)
    //    {
    //        m_CFPK_select.left--;
    //    }
    //}
    //ImGui::SameLine();
    //if (ImGui::ArrowButton("##right", ImGuiDir_Right))
    //{
    //    m_CFPK_select.left = int(m_CFPK_select.left + 1) % 7;
    //}
    //ImGui::PopButtonRepeat();
    //ImGui::SameLine();
    //ImGui::Text("Direction");

    //ImGui::InputFloat("ACT", &m_CFPK_select.top);
    //ImGui::SameLine();
    //ImGui::Text("Action");

    //ImGui::SeparatorText("deformation");
    //ImGui::InputFloat("zoom", &m_scale, 0.1f, 1.0f, "%.3f");


    //static bool showCoordinate = false;
    //ImGui::Checkbox("SHOW Coordinate", &showCoordinate);
    //if (showCoordinate)
    //{
    //    if (ImGui::Begin("Example: Simple overlay", &p_open))
    //    {
    //        ImGuiIO& io = ImGui::GetIO();
    //        ImGui::Text("(right-click to change position)");
    //        ImGui::Separator();
    //        if (ImGui::IsMousePosValid())
    //        {
    //            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x - 650, io.MousePos.y - 450);
    //            ImGui::Text("Shadow Position: (%.1f,%.1f)", m_ShadowRect.left - 650, m_ShadowRect.top - 450);
    //        }

    //        else
    //            ImGui::Text("Mouse Position: <invalid>");
    //        if (ImGui::BeginPopupContextWindow())
    //        {

    //            if (p_open && ImGui::MenuItem("Close")) p_open = false;
    //            ImGui::EndPopup();
    //        }
    //        ImGui::End();
    //    }
    //}
    //ImGui::SeparatorText("Offset");
    //ImGui::InputFloat("X", &m_offset.x);
    //ImGui::SameLine();
    //ImGui::InputFloat("Y", &m_offset.y);


    //ImGui::SeparatorText("Effect");
    //static bool showShadows = false;
    //ImGui::Checkbox("showShadows", &showShadows);
    //if (showShadows) {
    //    checkNUM = 1;
    //}
    //else {
    //    checkNUM = 0;
    //}
    //ImGui::InputFloat("SX", &m_ShadowOffset.x);
    //ImGui::SameLine();
    //ImGui::InputFloat("SY", &m_ShadowOffset.y);
    //ImGui::InputFloat("Rotation", &m_Rotation);
    //ImGui::InputFloat("Rotation2", &m_Skew_X);
    //ImGui::InputFloat("Rotation3", &m_Skew_Y);

    //static int lastSavedPicnum = -1;
    //if (ImGui::Button("Save Shandows") || Keepitforever)
    //{
    //    //TODO

    //    //end

    //    std::shared_ptr<CFrame> temporaryFrame;
    //    int picnum = 0;
    //    std::string str(g_CreatureNames[0]);
    //    std::shared_ptr<CAnimation> animation = m_animationManager.GetAnimation(str);
    //    if (animation) {
    //        // Animation found, can safely use animation pointers
    //        temporaryFrame = animation->GetFrame(m_CurrFrame);
    //        picnum = temporaryFrame->GetSpriteID();

    //        if (Keepitforever)
    //        {



    //            if (picnum != lastSavedPicnum)
    //            {
    //                SaveShadow(picnum);
    //                lastSavedPicnum = picnum;
    //            }
    //        }
    //        else {
    //            SaveShadow(picnum);
    //        }
    //        // Prevent duplicate image writing


    //    }









    //}

    //ImGui::Checkbox("KeepSave", &Keepitforever);
    //static bool CFPKeditor = false;
    //ImGui::Checkbox("CFPKeditor", &CFPKeditor);



    //    
    //static bool showObject = false;
    //ImGui::Checkbox("SHOW OBJECT", &showObject);
    //    



    //
    ImGui::End();

    // =====================================================
    // Debug窗口 - 纹理管理器信息
    // =====================================================
    static bool debug_window_open = true;
    if (debug_window_open)
    {
        ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Debug - Texture Manager Info", &debug_window_open, ImGuiWindowFlags_AlwaysAutoResize))
        {
            // 加载进度条
            ImGui::SeparatorText("SPK Loading Progress");
            if (m_bLoading)
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", m_LoadingStatus.c_str());
                ImGui::ProgressBar(m_LoadingProgress, ImVec2(-1.0f, 0.0f), "");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loading complete!");
            }

            // 第一模块：纹理管理器总数量
            ImGui::SeparatorText("Texture Manager Status");
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "m_TextureManager.size() = %zu", m_TextureManager.GetTextureCount());

            // 第二模块：第一个Sprite的信息
            ImGui::SeparatorText("First Sprite Info (Index 0)");
            ID2D1Bitmap1* firstSprite = m_TextureManager.GetTexture(0);
            if (firstSprite != nullptr)
            {
                D2D1_SIZE_F size = firstSprite->GetSize();
                D2D1_PIXEL_FORMAT pixelFormat = firstSprite->GetPixelFormat();

                ImGui::Text("Pointer: %p", (void*)firstSprite);
                ImGui::Text("Size: %u x %u", size.width, size.height);
                ImGui::Text("Pixel Format:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "DXGI_FORMAT = %d", pixelFormat.format);
                ImGui::Text("Alpha Mode: %d", pixelFormat.alphaMode);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "First sprite is NULL!");
            }

            // 第三模块：SPK加载状态
            ImGui::SeparatorText("SPK Loading Status");
            ImGui::Text("ImageObject SPK:");
            ImGui::SameLine();
            if (m_SPKFiles[SPKType::ImageObject].isLoaded)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
                ImGui::Text("  Count: %u, StartIndex: %u", m_SPKFiles[SPKType::ImageObject].count, m_SPKFiles[SPKType::ImageObject].startIndex);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Loaded");
            }

            ImGui::Text("Tile SPK:");
            ImGui::SameLine();
            if (m_SPKFiles[SPKType::Tile].isLoaded)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
                ImGui::Text("  Count: %u, StartIndex: %u", m_SPKFiles[SPKType::Tile].count, m_SPKFiles[SPKType::Tile].startIndex);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Loaded");
            }

            // 第四模块：Map加载状态
            ImGui::SeparatorText("Map Status");
            if (m_MapFilePath.empty())
            {
                ImGui::Text("No map file selected");
            }
            else
            {
                int bufferSize = WideCharToMultiByte(CP_UTF8, 0, m_MapFilePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string utf8Path(bufferSize, 0);
                WideCharToMultiByte(CP_UTF8, 0, m_MapFilePath.c_str(), -1, &utf8Path[0], bufferSize, nullptr, nullptr);
                ImGui::Text("File: %s", utf8Path.c_str());
            }

            if (m_bMapLoading)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Map is loading...");
            }

            // 第五模块：视口控制
            ImGui::SeparatorText("Viewport Control");
            ImGui::Text("Position: (%d, %d)", m_ViewportX, m_ViewportY);
            ImGui::Text("Map Size: %u x %u", m_Zone.GetWidth(), m_Zone.GetHeight());
            ImGui::Text("Zoom: %.2fx", m_ZoomLevel);

            // 缩放滑块
            ImGui::SliderFloat("Zoom", &m_ZoomLevel, 0.25f, 4.0f, "%.2f");

            // 移动按钮
            if (ImGui::Button("Up##vp")) MoveViewport(0, -1);
            ImGui::SameLine();
            if (ImGui::Button("Down##vp")) MoveViewport(0, 1);
            ImGui::SameLine();
            if (ImGui::Button("Left##vp")) MoveViewport(-1, 0);
            ImGui::SameLine();
            if (ImGui::Button("Right##vp")) MoveViewport(1, 0);

            ImGui::Text("Controls: Arrow Keys / WASD to move");
            ImGui::Text("         Q/E or Slider to zoom");



            ImGui::End();
        }
    }

    // 独立的导航窗窗口 - 使用像素坐标等比例缩小
    // sector = tile，每个tile像素大小为 m_TileWidth x m_TileHeight
    if (m_bMapReady && m_Zone.GetWidth() > 0 && m_Zone.GetHeight() > 0)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 450), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Navigation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            uint32_t mapW = m_Zone.GetWidth();
            uint32_t mapH = m_Zone.GetHeight();

            // sector = tile，像素坐标：宽 = mapW * m_TileWidth，高 = mapH * m_TileHeight
            float mapPixelW = (float)mapW * m_TileWidth;
            float mapPixelH = (float)mapH * m_TileHeight;

            // 缩放到导航窗口大小（最大200px），保持宽高比
            float navMaxW = 200.0f;
            float navScale;
            if (mapPixelW > mapPixelH)
            {
                navScale = navMaxW / mapPixelW;
            }
            else
            {
                navScale = navMaxW / mapPixelH;
            }

            float navW = mapPixelW * navScale;
            float navH = mapPixelH * navScale;

            ImVec2 navMin = ImGui::GetCursorScreenPos();
            ImVec2 navMax = ImVec2(navMin.x + navW, navMin.y + navH);

            // 视口像素大小
            float vpPixelW = 1280.0f / m_ZoomLevel;
            float vpPixelH = 720.0f / m_ZoomLevel;

            // 视口在导航中的位置和大小
            float vpX = (float)m_ViewportX * m_TileWidth * navScale;
            float vpY = (float)m_ViewportY * m_TileHeight * navScale;
            float vpW = vpPixelW * navScale;
            float vpH = vpPixelH * navScale;

            // 绘制背景
            ImGui::GetWindowDrawList()->AddRectFilled(navMin, navMax, IM_COL32(30, 30, 30, 255));

            // 绘制sector (每个sector = 一个tile)
            // 根据 flag 决定颜色：绿色=可通行，红色=不可通行
            for (uint32_t y = 0; y < mapH; y++)
            {
                for (uint32_t x = 0; x < mapW; x++)
                {
                    GameSector* sector = m_Zone.GetSector(x, y);
                    if (sector)
                    {
                        float sx = (float)x * m_TileWidth * navScale;
                        float sy = (float)y * m_TileHeight * navScale;
                        float sw = m_TileWidth * navScale;
                        float sh = m_TileHeight * navScale;
                        ImVec2 cMin = ImVec2(navMin.x + sx, navMin.y + sy);
                        ImVec2 cMax = ImVec2(cMin.x + sw - 0.5f, cMin.y + sh - 0.5f);

                        // 根据 flag 判断颜色
                        // FLAG_SECTOR_PORTAL (0x80) - 蓝色
                        // FLAG_SECTOR_BLOCK_GROUND (0x02) - 淡红色
                        // FLAG_SECTOR_BLOCK_ALL (0x07) - 红色（完全不可通行）
                        // 其他 - 绿色（可通行）
                        ImU32 color;
                        uint8_t flag = sector->GetFlag();
                        if (flag & FLAG_SECTOR_PORTAL)
                        {
                            // 蓝色 - 传送门
                            color = IM_COL32(80, 80, 255, 255);
                        }
                        else if (flag & FLAG_SECTOR_BLOCK_GROUND)
                        {
                            // 淡红色 - 地面不可通行
                            color = IM_COL32(255, 150, 150, 255);
                        }
                        else if (flag & FLAG_SECTOR_BLOCK_ALL)
                        {
                            // 红色 - 完全不可通行
                            color = IM_COL32(200, 50, 50, 255);
                        }
                        else
                        {
                            // 绿色 - 可通行
                            color = IM_COL32(80, 200, 80, 255);
                        }
                        ImGui::GetWindowDrawList()->AddRectFilled(cMin, cMax, color);
                    }
                }
            }

            // 绘制视口框
            ImVec2 vpMin = ImVec2(navMin.x + vpX, navMin.y + vpY);
            ImVec2 vpMax = ImVec2(vpMin.x + vpW, vpMin.y + vpH);
            ImGui::GetWindowDrawList()->AddRect(vpMin, vpMax, IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);

            // 中心点
            ImVec2 ctr = ImVec2(vpMin.x + vpW / 2, vpMin.y + vpH / 2);
            ImGui::GetWindowDrawList()->AddCircleFilled(ctr, 4, IM_COL32(255, 0, 0, 255));

            ImGui::SetCursorScreenPos(ImVec2(navMin.x, navMin.y));
            ImGui::InvisibleButton("nav_btn", ImVec2(navW, navH));

            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
            {
                ImVec2 mp = ImGui::GetIO().MousePos;
                // 将点击位置转换为tile坐标
                int cx = (int)((mp.x - navMin.x) / (m_TileWidth * navScale));
                int cy = (int)((mp.y - navMin.y) / (m_TileHeight * navScale));
                // 将点击位置设为视口中心
                int vpTileW = (int)(vpPixelW / m_TileWidth);
                int vpTileH = (int)(vpPixelH / m_TileHeight);
                SetViewportPosition(cx - vpTileW / 2, cy - vpTileH / 2);
            }

            ImGui::Text("Pos: (%d,%d) | Map: %ux%u", m_ViewportX, m_ViewportY, mapW, mapH);
        }
        ImGui::End();
    }

    //if (CFPKeditor)
    //{
    //    m_guitree.ShowEditgui(m_CFPK_select.left, p_open);
    //}
    //if (showObject)
    //{
    //    ImGuiStyle previousStyle = ImGui::GetStyle();
    //    ImGuiStyle& style = ImGui::GetStyle();
    //    style.WindowRounding = 0.0f; // 取消窗口圆角
    //    style.WindowBorderSize = 0.0f; // 取消窗口边框
    //    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // 设置窗口背景为透明

    //    if (ImGui::Begin("Creature Oject", &p_open,ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoScrollbar))
    //    {
    //        CIndexSprite* m_SP;
    //        m_SP = m_pISPK->GetData(21);
    //        D2D1_SIZE_U bitmapSize = { m_SP->GetWidth(), m_SP->GetHeight() };

    //        //imgui Image 在dx11环境下是ShaderResourceView* 
    //        // 我想要实现的拖拽功能不好使
    //        ImGui::Image((void*)m_ShaderResourceView.Get(), ImVec2(bitmapSize.width, bitmapSize.height));
    //        ImGui::End();
    //    }
    //    ImGui::GetStyle() = previousStyle;
    //
    //}
    //p_open = true;
    //ShowCpkEditGui(&p_open);

    // ========== 显示加载进度 ==========
    if (m_bSpritesLoading || m_asyncLoadProgress > 0.0f) {
        ImGui::Begin("Loading Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", m_asyncLoadStatus.c_str());

        // 进度条
        ImGui::ProgressBar(m_asyncLoadProgress, ImVec2(200.0f, 20.0f));
        ImGui::SameLine();
        ImGui::Text("%.0f%%", m_asyncLoadProgress * 100.0f);

        ImGui::End();
    }

    // 显示 ImageObject 列表窗口和预览窗口（独立窗口）
    ShowImageObjectListWindow();
    ShowCompositePreviewWindow();
    ShowImagePreviewWindow();  // 纯图片预览窗口

    // 显示 Sprite 调试窗口
    ShowSpriteDebugWindow();

    // ========== 地图右键框选检测 ==========
    // 地图显示区域：(0, 0) 到 (1280, 720)
    // 视口坐标 -> 格子坐标的三重转换：
    // 1. 屏幕坐标(鼠标) -> 视口坐标
    // 2. 视口坐标 -> 像素坐标
    // 3. 像素坐标 -> 格子坐标

    static ImVec2 dragStartPos;  // 框选起始位置（屏幕坐标）
    static bool isDragging = false;

    if (m_bMapReady) {
        ImVec2 mousePos = ImGui::GetIO().MousePos;

        // 检查鼠标是否在地图区域内
        bool inMapArea = (mousePos.x >= 0 && mousePos.x < 1280 && mousePos.y >= 0 && mousePos.y < 720);

        // 右键按下：开始框选
        if (inMapArea && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            dragStartPos = mousePos;
            isDragging = true;
            m_selectedSectors.clear();  // 清除之前的选择
        }

        // 右键拖动中：更新框选区域
        if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            // 拖动时不需要额外处理，等待释放时计算
        }

        // 右键释放：完成框选
        if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            isDragging = false;

            ImVec2 dragEndPos = mousePos;

            // 计算框选的矩形区域（屏幕坐标）
            float minX = (std::min)(dragStartPos.x, dragEndPos.x);
            float maxX = (std::max)(dragStartPos.x, dragEndPos.x);
            float minY = (std::min)(dragStartPos.y, dragEndPos.y);
            float maxY = (std::max)(dragStartPos.y, dragEndPos.y);

            // 将屏幕坐标转换为格子坐标
            int startSX = 0, startSY = 0, endSX = 0, endSY = 0;
            {
                // 1. 屏幕 -> 视口
                float vpx = minX / m_ZoomLevel;
                float vpy = minY / m_ZoomLevel;
                // 2. 视口 -> 像素
                D2D1_POINT_2F pixelPos = ViewportToPixel(vpx, vpy);
                // 3. 像素 -> 格子
                D2D1_POINT_2F sectorPos = PixelToSector(pixelPos.x, pixelPos.y);
                startSX = static_cast<int>(sectorPos.x);
                startSY = static_cast<int>(sectorPos.y);
            }
            {
                // 1. 屏幕 -> 视口
                float vpx = maxX / m_ZoomLevel;
                float vpy = maxY / m_ZoomLevel;
                // 2. 视口 -> 像素
                D2D1_POINT_2F pixelPos = ViewportToPixel(vpx, vpy);
                // 3. 像素 -> 格子
                D2D1_POINT_2F sectorPos = PixelToSector(pixelPos.x, pixelPos.y);
                endSX = static_cast<int>(sectorPos.x);
                endSY = static_cast<int>(sectorPos.y);
            }

            int minSX = (std::max)(0, startSX);
            int maxSX = (std::min)(static_cast<int>(m_Zone.GetWidth()) - 1, endSX);
            int minSY = (std::max)(0, startSY);
            int maxSY = (std::min)(static_cast<int>(m_Zone.GetHeight()) - 1, endSY);

            // 遍历框选区域内的所有 sector
            for (int sy = minSY; sy <= maxSY; sy++) {
                for (int sx = minSX; sx <= maxSX; sx++) {
                    GameSector* sector = m_Zone.GetSector(sx, sy);
                    if (sector) {
                        SelectedSector sel;
                        sel.x = sx;
                        sel.y = sy;
                        sel.tileID = sector->GetSpriteID();
                        sel.imgID = sector->GetObjectID();
                        sel.objectSpriteID = sector->GetObjectSpriteID();  // 获取 ImageObject 的 spriteID
                        m_selectedSectors.push_back(sel);
                    }
                }
            }

            char buf[256];
            sprintf(buf, "[Debug] Right-click drag: sectors=(%d,%d)-(%d,%d), count=%zu\n",
                minSX, minSY, maxSX, maxSY, m_selectedSectors.size());
            OutputDebugStringA(buf);
        }
    }

    // 显示 Sector 信息窗口
    ShowSectorInfoWindow();
}

// ImageObject 列表窗口
void GameApp::ShowImageObjectListWindow() {
    if (!m_showImageObjectList) {
        return;
    }

    if (!ImGui::Begin("ImageObject List", &m_showImageObjectList)) {
        ImGui::End();
        return;
    }

    // 显示统计信息
    ImGui::Text("Total Instances: %zu", m_Zone.GetImageObjectCount());
    ImGui::Text("Composite Objects: %zu", m_Zone.GetCompositeObjectCount());

    // 选中项操作按钮
    if (m_selectedImageObjectID != 0) {
        ImGui::SameLine();
        ImGui::Text(" | Selected: %u", m_selectedImageObjectID);
        ImGui::SameLine();
        if (ImGui::Button("Jump To")) {
            char buf[256];
            sprintf(buf, "[UI] Jump button clicked for object %u\n", m_selectedImageObjectID);
            OutputDebugStringA(buf);
            JumpToSelectedObject();
        }
        ImGui::SameLine();
        if (ImGui::Button("Preview")) {
            // 获取 composite object 的第一个 spriteID
            const auto& composites = m_Zone.GetAllCompositeObjects();
            auto it = composites.find(m_selectedImageObjectID);
            if (it != composites.end() && !it->second.spriteIDs.empty()) {
                // 传递 spriteID 而不是 imageID
                m_pendingPreviewObjectID = it->second.spriteIDs[0];
            }
            // 确保预览窗口显示
            m_showImageObjectList = true;
            m_showPreviewWindow = true;  // 打开原始预览窗口
            m_showImagePreviewWindow = true;  // 打开纯图片预览窗口
        }
    } else {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), " | Select an item");
    }

    // 获取所有 composite objects
    const auto& composites = m_Zone.GetAllCompositeObjects();

    // 创建表格 - 3列：ImgID、对象数量、SpriteID列表
    if (ImGui::BeginTable("ImageObjectTable", 3,
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("ImgID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 80.0f);
        ImGui::TableSetupColumn("Object Count", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 100.0f);
        ImGui::TableSetupColumn("SpriteID List", ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();

        // 过滤掉 sprite 数量 <= 1 的项目
        for (const auto& pair : composites) {
            const auto& comp = pair.second;

            // 只显示多个 sprite 组成的大图
            

            // 高亮选中行
            if (comp.objectID == m_selectedImageObjectID) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(100, 149, 237, 128));
            }

            ImGui::TableNextRow();

            // ImgID 列 - 使用 Selectable
            ImGui::TableSetColumnIndex(0);
            bool isSelected = (comp.objectID == m_selectedImageObjectID);
            if (ImGui::Selectable(std::to_string(comp.objectID).c_str(), &isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                char buf[256];
                sprintf(buf, "[UI] Selected: objectID=%u, count=%zu\n", comp.objectID, comp.objectCount);
                OutputDebugStringA(buf);
                m_selectedImageObjectID = comp.objectID;
            }

            // 对象数量 列
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1);
            ImGui::Text("%zu", comp.objectCount);

            // SpriteID列表 列
            ImGui::TableSetColumnIndex(2);
            if (comp.spriteIDs.empty()) {
                ImGui::Text("-");
            } else {
                std::string spriteList;
                for (size_t i = 0; i < comp.spriteIDs.size(); i++) {
                    if (i > 0) spriteList += ", ";
                    spriteList += std::to_string(comp.spriteIDs[i]);
                    if (i >= 20) {
                        spriteList += " ...";
                        break;
                    }
                }
                ImGui::Text("%s", spriteList.c_str());
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

// 保存 sprite 到 PNG 文件
bool GameApp::SaveSpriteToPNG(uint32_t spriteID, const char* filename) {
    if (!m_pISPK) {
        return false;
    }

    CBaseSprite* sprite = m_pISPK->GetData(spriteID);
    if (!sprite) {
        return false;
    }

    // 再次调用 SetImage 确保数据完整
    sprite->SetImage();

    uint32_t width = sprite->GetWidth();
    uint32_t height = sprite->GetHeight();
    uint32_t* rgbaData = sprite->GetImage();

    if (!rgbaData || width == 0 || height == 0) {
        return false;
    }

    // 数据是 RGBA 格式，转换为 RGB
    uint8_t* rgbData = new uint8_t[width * height * 3];
    for (uint32_t i = 0; i < width * height; i++) {
        rgbData[i * 3 + 0] = ((uint8_t*)rgbaData)[i * 4 + 0]; // R
        rgbData[i * 3 + 1] = ((uint8_t*)rgbaData)[i * 4 + 1]; // G
        rgbData[i * 3 + 2] = ((uint8_t*)rgbaData)[i * 4 + 2]; // B
    }

    // 不翻转，测试用
    stbi_flip_vertically_on_write(0);
    int result = stbi_write_png(filename, width, height, 3, rgbData, width * 3);

    delete[] rgbData;

    // 保存原始二进制数据（从第167行开始到结束）
    const int startLine = 167;
    if (height > startLine) {
        std::string binFilename = std::string(filename) + ".bin";
        std::ofstream binFile(binFilename, std::ios::binary);
        if (binFile.is_open()) {
            // 保存从第167行开始的数据
            for (uint32_t y = startLine; y < height; y++) {
                BYTE* lineData = sprite->GetPixelLine(y);
                if (lineData) {
                    // 每行数据：2字节长度 + 像素数据(WORD数组)
                    WORD lineLen = *(WORD*)lineData;
                    binFile.write((char*)&lineLen, 2);
                    if (lineLen > 0) {
                        binFile.write((char*)(lineData + 2), lineLen * 2);
                    }
                } else {
                    // 没有数据写入0
                    WORD zero = 0;
                    binFile.write((char*)&zero, 2);
                }
            }

            binFile.close();
            OutputDebugStringA("[SaveSpriteToPNG] Binary data saved\n");
        }
    }

    return result != 0;
}

// 原始预览窗口（含信息）
void GameApp::ShowCompositePreviewWindow() {
    if (!m_showPreviewWindow || m_selectedImageObjectID == 0) {
        return;
    }

    // 通过 ID 查找 composite object
    const CompositeObject* comp = m_Zone.GetCompositeObject(m_selectedImageObjectID);
    if (!comp || comp->spriteIDs.empty()) {
        return;
    }

    if (!ImGui::Begin("Composite Preview", &m_showPreviewWindow)) {
        ImGui::End();
        return;
    }

    ImGui::Text("objectID=%u, count=%zu", comp->objectID, comp->objectCount);

    // 显示已渲染的预览（使用 D3D SRV）
    if (m_previewSRV) {


        // 保存 PNG 按钮
        ImGui::Separator();
        static int saveSpriteIDInt = 1844;
        uint32_t saveSpriteID = static_cast<uint32_t>(saveSpriteIDInt);
        ImGui::InputInt("SpriteID", &saveSpriteIDInt);
        if (ImGui::Button("Save PNG")) {
            // 获取当前工作目录
            char cwd[256];
            GetCurrentDirectoryA(256, cwd);
            std::string fullPath = std::string(cwd) + "\\sprite_" + std::to_string(saveSpriteID) + ".png";

            if (SaveSpriteToPNG(saveSpriteID, fullPath.c_str())) {
                ImGui::OpenPopup("SaveSuccess");
            }
        }
        if (ImGui::BeginPopup("SaveSuccess")) {
            char cwd[256];
            GetCurrentDirectoryA(256, cwd);
            ImGui::Text("Saved to:");
            ImGui::TextWrapped("%s\\sprite_%u.png", cwd, saveSpriteID);
            ImGui::EndPopup();
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No preview rendered yet");
        ImGui::Text("Please click Preview button to render sprite 1844");
    }

    // 显示 SpriteID 列表
    ImGui::Separator();
    ImGui::Text("SpriteID List (using hardcoded 1844):");

    ImGui::End();
}

// 纯图片预览窗口（无边框透明）- 仅显示图片
void GameApp::ShowImagePreviewWindow() {




    // 检查是否有预览图
    if (!m_previewSRV) {
        return;
    }

    // 参考用户提供的注释代码实现
    ImGuiStyle previousStyle = ImGui::GetStyle();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;  // 取消窗口圆角
    style.WindowBorderSize = 0.0f;  // 取消窗口边框
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // 设置窗口背景为透明

    // 设置窗口位置和大小
    ImGui::SetNextWindowPos(ImVec2(750, 350), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)m_previewWidth + 20, (float)m_previewHeight + 20), ImGuiCond_FirstUseEver);

    // 使用 p_open 变量控制窗口显示
    bool p_open = m_showImagePreviewWindow;
    if (ImGui::Begin("Image Preview", &p_open,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
    {
        D2D1_SIZE_U bitmapSize = { m_previewWidth, m_previewHeight };
        ImGui::Image((void*)m_previewSRV.Get(), ImVec2(bitmapSize.width, bitmapSize.height));
        ImGui::End();
    }

    // 更新窗口显示状态
    m_showImagePreviewWindow = p_open;

    ImGui::GetStyle() = previousStyle;
}

// Sprite 调试窗口 - 检查指定 spriteID 的属性
void GameApp::ShowSpriteDebugWindow() {
    if (!m_showSpriteDebugWindow) {
        return;
    }

    static bool showDebug = true;
    if (!ImGui::Begin("Sprite Debug", &showDebug)) {
        ImGui::End();
        return;
    }

    // 输入 spriteID
    ImGui::Text("Input SpriteID to inspect:");
    ImGui::SameLine();
    ImGui::InputInt("##spriteID", (int*)&m_debugSpriteID);

    // 检查 ImageObject flyweight pool
    ImGui::Separator();
    ImGui::Text("=== ImageObject Flyweight Pool ===");
    const auto& flyweights = m_Zone.GetAllImageObjects();
    auto it = flyweights.find(m_debugSpriteID);
    if (it != flyweights.end()) {
        const auto& objList = it->second;
        ImGui::Text("Found imgID=%u with %zu instances", it->first, objList.size());
        for (size_t i = 0; i < objList.size(); i++) {
            const auto& obj = objList[i];
            ImGui::Text("  [%zu] imgID=%u, spriteID=%u, pixel=(%d,%d), viewpoint=%u",
                i, obj.GetID(), obj.GetSpriteID(), obj.GetPixelX(), obj.GetPixelY(), obj.GetViewpoint());
        }
    } else {
        ImGui::Text("imgID=%u NOT FOUND in flyweight pool", m_debugSpriteID);
    }

    // 显示所有 flyweights 的 spriteID 分布
    ImGui::Separator();
    ImGui::Text("=== All Flyweights SpriteID Stats ===");
    int normalCount = 0, hugeCount = 0;
    uint32_t maxSpriteID = 0, minSpriteID = 0xFFFFFFFF;
    for (const auto& pair : flyweights) {
        for (const auto& obj : pair.second) {
            uint32_t sid = obj.GetSpriteID();
            if (sid > maxSpriteID) maxSpriteID = sid;
            if (sid < minSpriteID) minSpriteID = sid;
            if (sid > 65535) hugeCount++;
            else normalCount++;
        }
    }
    ImGui::Text("Total instances: %zu", flyweights.size());
    ImGui::Text("spriteID range: %u ~ %u", minSpriteID, maxSpriteID);
    ImGui::Text("Normal (<=65535): %d, Huge (>65535): %d", normalCount, hugeCount);

    // 直接从 m_TextureManager 获取纹理信息
    ImGui::Separator();
    ImGui::Text("=== TextureManager Status ===");
    size_t totalTextures = m_TextureManager.GetTextureCount();
    ImGui::Text("Total textures loaded: %zu", totalTextures);

    // 显示 SPK 元数据（作为参考）
    const SPKFileInfo& imgSpkInfo = m_SPKFiles[SPKType::ImageObject];
    ImGui::Text("ImageObject SPK metadata:");
    ImGui::Text("  - isLoaded: %s", imgSpkInfo.isLoaded ? "true" : "false");
    ImGui::Text("  - count: %u", imgSpkInfo.count);
    ImGui::Text("  - startIndex: %u", imgSpkInfo.startIndex);

    // 计算全局索引并尝试获取纹理
    ImGui::Text("Input spriteID: %u", m_debugSpriteID);
    UINT32 globalIndex = imgSpkInfo.startIndex + m_debugSpriteID;
    ImGui::Text("Calculated globalIndex: %u + %u = %u", imgSpkInfo.startIndex, m_debugSpriteID, globalIndex);

    // 检查 ObjectLayer 尺寸
    ImGui::Separator();
    ImGui::Text("=== ObjectLayer Size Check ===");
    ID2D1Bitmap1* objLayer = m_MapLayerManager.GetLayer(LayerType::ObjectLayer);
    if (objLayer) {
        D2D1_SIZE_U layerSize = objLayer->GetPixelSize();
        ImGui::Text("ObjectLayer size: %u x %u (WxH)", layerSize.width, layerSize.height);
    } else {
        ImGui::Text("ObjectLayer: NOT CREATED");
    }

    // 检查 TileLayer 尺寸
    ID2D1Bitmap1* tileLayer = m_MapLayerManager.GetLayer(LayerType::TileLayer);
    if (tileLayer) {
        D2D1_SIZE_U tileLayerSize = tileLayer->GetPixelSize();
        ImGui::Text("TileLayer size: %u x %u (WxH)", tileLayerSize.width, tileLayerSize.height);
    } else {
        ImGui::Text("TileLayer: NOT CREATED");
    }

    // 直接从 TextureManager 获取纹理
    ID2D1Bitmap1* tex = m_TextureManager.GetTexture(globalIndex);
    if (tex) {
        D2D1_SIZE_U size = tex->GetPixelSize();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            ">>> Texture[global=%u]: %u x %u (WxH)", globalIndex, size.width, size.height);

        // 查找该 spriteID 对应的 ImageObject 的像素坐标
        const auto& flyweights = m_Zone.GetAllImageObjects();
        bool found = false;
        for (const auto& pair : flyweights) {
            for (const auto& obj : pair.second) {
                if (obj.GetSpriteID() == m_debugSpriteID) {
                    ImGui::Text("Found ImageObject: imgID=%u, pixel=(%d,%d), viewpoint=%u",
                        obj.GetID(), obj.GetPixelX(), obj.GetPixelY(), obj.GetViewpoint());

                    // 检查绘制目标是否会超出图层边界
                    if (objLayer) {
                        D2D1_SIZE_U layerSize = objLayer->GetPixelSize();
                        float destBottom = obj.GetPixelY() + (float)size.height;
                        float destRight = obj.GetPixelX() + (float)size.width;
                        if (destBottom > layerSize.height) {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                "!!! WARNING: destBottom %.1f > layerHeight %u - CLIPPED!",
                                destBottom, layerSize.height);
                        }
                        if (destRight > layerSize.width) {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                "!!! WARNING: destRight %.1f > layerWidth %u - CLIPPED!",
                                destRight, layerSize.width);
                        }
                    }
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found) {
            ImGui::Text("No ImageObject found with spriteID=%u", m_debugSpriteID);
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            ">>> Texture[global=%u]: NOT FOUND in manager", globalIndex);

        // 显示 manager 中实际存在的一些索引作为参考
        ImGui::Text("TextureManager range check:");
        ImGui::Text("  - Requested index: %u", globalIndex);
        ImGui::Text("  - Total loaded: %zu", totalTextures);
        if (globalIndex >= totalTextures) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                "  - WARNING: index %u >= total %zu", globalIndex, totalTextures);
        }
    }

    // 检查所有 ImageObject 的像素坐标范围
    ImGui::Separator();
    ImGui::Text("=== ObjectLayer Size Check ===");
    uint32_t mapW = m_Zone.GetWidth();
    uint32_t mapH = m_Zone.GetHeight();
    UINT tileBasedWidth = mapW * 48;
    UINT tileBasedHeight = mapH * 24;

    // 计算所有 ImageObject 的最大像素坐标
    int maxPixelX = 0, maxPixelY = 0;
    const auto& objectsByViewpoint = m_Zone.GetImageObjectsByViewpoint();
    for (const auto& vpPair : objectsByViewpoint) {
        for (const MImageObject* obj : vpPair.second) {
            int px = obj->GetPixelX();
            int py = obj->GetPixelY();
            if (px > maxPixelX) maxPixelX = px;
            if (py > maxPixelY) maxPixelY = py;
        }
    }
    ImGui::Text("Map size (tiles): %ux%u", mapW, mapH);
    ImGui::Text("Tile-based layer size: %ux%u", tileBasedWidth, tileBasedHeight);
    ImGui::Text("Max ImageObject pixel: (%d, %d)", maxPixelX, maxPixelY);

    // 警告如果超出
    if (maxPixelX > (int)tileBasedWidth || maxPixelY > (int)tileBasedHeight) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "WARNING: ImageObjects exceed layer bounds!");
        ImGui::Text("Required layer size: >%dx >%d", maxPixelX + 100, maxPixelY + 100);
    }

    // 检查是否在 viewpoint 索引中（objectsByViewpoint 已在上面定义）
    ImGui::Separator();
    ImGui::Text("=== Viewpoint Index ===");
    int foundInViewpoint = 0;
    for (const auto& vpPair : objectsByViewpoint) {
        for (const MImageObject* obj : vpPair.second) {
            if (obj->GetID() == m_debugSpriteID) {
                foundInViewpoint++;
                ImGui::Text("  Found in viewpoint=%u: spriteID=%u, pixel=(%d,%d)",
                    vpPair.first, obj->GetSpriteID(), obj->GetPixelX(), obj->GetPixelY());
            }
        }
    }
    if (foundInViewpoint == 0) {
        ImGui::Text("imgID=%u NOT FOUND in any viewpoint", m_debugSpriteID);
    } else {
        ImGui::Text("Found %d instances in viewpoint index", foundInViewpoint);
    }

    ImGui::End();
}

// 显示点击选中的 Sector 信息窗口
void GameApp::ShowSectorInfoWindow() {
    if (m_selectedSectors.empty()) {
        return;
    }

    static bool showSectorInfo = true;
    if (!ImGui::Begin("Sector Info", &showSectorInfo)) {
        ImGui::End();
        return;
    }

    // 清除选择按钮和预览按钮
    if (ImGui::Button("Clear Selection")) {
        m_selectedSectors.clear();
        m_selectedSectorIndex = -1;
    }
    ImGui::SameLine();
    // 预览按钮 - 渲染选中的 sector 的 objectSpriteID
    bool canPreview = m_selectedSectorIndex >= 0 && m_selectedSectorIndex < (int)m_selectedSectors.size();
    if (ImGui::Button("Preview Sprite") && canPreview) {
        uint32_t spriteID = m_selectedSectors[m_selectedSectorIndex].objectSpriteID;
        if (spriteID != 0) {
            RenderCompositePreview(spriteID);
            m_showPreviewWindow = true;
            m_showImagePreviewWindow = true;
        }
    }
    if (!canPreview) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Select a row first)");
    }
    ImGui::SameLine();
    ImGui::Text("Selected: %zu sectors", m_selectedSectors.size());

    // 显示表格 - 6列：选择, X, Y, TileID, ImgID, ObjSpriteID
    if (ImGui::BeginTable("SectorTable", 6,
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("TileID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("ImgID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("ObjSpriteID", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableHeadersRow();

        int index = 0;
        for (const auto& sel : m_selectedSectors) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool isSelected = (index == m_selectedSectorIndex);
            char label[32];
            sprintf(label, "#%d", index);
            if (ImGui::Selectable(label, &isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                m_selectedSectorIndex = index;
            }
            // 高亮选中行
            if (isSelected) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(100, 149, 237, 128));
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", sel.x);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", sel.y);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", sel.tileID);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%u", sel.imgID);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%u", sel.objectSpriteID);
            index++;
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void GameApp::ClearSelectedSectors() {
    m_selectedSectors.clear();
}

// 绘制选中 sector 的高亮矩形（每帧实时绘制）
void GameApp::DrawSelectedSectorsHighlight() {
    if (m_selectedSectors.empty()) {
        return;
    }

    // 窗口尺寸
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // 视口像素偏移
    float viewportOffsetX = (float)m_ViewportX * m_TileWidth;
    float viewportOffsetY = (float)m_ViewportY * m_TileHeight;

    // 创建红色笔刷
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> redBrush;
    m_pd2dImmediateContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.8f), redBrush.GetAddressOf());

    for (const auto& sel : m_selectedSectors) {
        // 计算 sector 在像素坐标系中的位置（格子坐标 -> 像素坐标）
        float pixelX = (float)sel.x * m_TileWidth;
        float pixelY = (float)sel.y * m_TileHeight;

        // 计算在屏幕上的位置（考虑视口偏移和缩放）
        float screenX = (pixelX - viewportOffsetX) * m_ZoomLevel;
        float screenY = (pixelY - viewportOffsetY) * m_ZoomLevel;

        // 检查是否在可视范围内
        float rectW = m_TileWidth * m_ZoomLevel;
        float rectH = m_TileHeight * m_ZoomLevel;
        if (screenX + rectW < 0 || screenX > windowWidth ||
            screenY + rectH < 0 || screenY > windowHeight) {
            continue;
        }

        // 绘制 48x24 的矩形（带缩放）
        D2D1_RECT_F highlightRect = D2D1::RectF(
            screenX,
            screenY,
            screenX + rectW,
            screenY + rectH
        );
        m_pd2dImmediateContext->DrawRectangle(highlightRect, redBrush.Get(), 2.0f);
    }
}

void GameApp::JumpToSelectedObject() {
    if (m_selectedImageObjectID == 0) {
        return;
    }

    const auto* comp = m_Zone.GetCompositeObject(m_selectedImageObjectID);
    if (!comp) {
        return;
    }

    // 视口是 1280x720，每个 tile 是 48x24 像素
    // 视口可以显示的格子数：1280/48 ≈ 26.6，720/24 ≈ 30
    // 左上角是 sector 中心点，所以偏移量是宽度的一半
    int viewportHalfW = 13;  // 约 27/2
    int viewportHalfH = 15;  // 约 30/2

    // 设置视口位置，使对象 sector 中心位于视口中央
    m_ViewportX = static_cast<int>(comp->sectorX) - viewportHalfW;
    m_ViewportY = static_cast<int>(comp->sectorY) - viewportHalfH;

    // 确保视口不超出地图范围
    uint32_t mapW = m_Zone.GetWidth();
    uint32_t mapH = m_Zone.GetHeight();
    if (m_ViewportX < 0) m_ViewportX = 0;
    if (m_ViewportY < 0) m_ViewportY = 0;
    if (m_ViewportX > static_cast<int>(mapW)) m_ViewportX = static_cast<int>(mapW);
    if (m_ViewportY > static_cast<int>(mapH)) m_ViewportY = static_cast<int>(mapH);

    m_bMapLayersDirty = true;


}

// 渲染选中组合的预览图（按需渲染）
// 参考 ObjectRenderer2 的实现，直接从 m_pISPK 获取 sprite 数据
void GameApp::RenderCompositePreview(uint32_t spriteID) {
    if (!m_pISPK) {
        return;
    }

    // 直接从 m_pISPK 获取精灵数据
    CBaseSprite* sprite = m_pISPK->GetData(spriteID);
    if (!sprite) {
        return;
    }

    // 再次调用 SetImage 确保数据完整
    sprite->SetImage();

    UINT32 texWidth = sprite->GetWidth();
    UINT32 texHeight = sprite->GetHeight();

    // 创建 D3D11 Texture2D（使用 RGBA 格式）
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = texWidth;
    texDesc.Height = texHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> d3dTex;
    HRESULT hr = m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, d3dTex.GetAddressOf());
    if (FAILED(hr)) {
        return;
    }

    // 直接从 sprite 获取像素数据并更新到 D3D 纹理
    m_pd3dImmediateContext->UpdateSubresource(d3dTex.Get(), 0, nullptr,
        sprite->GetImage(), texWidth * sizeof(uint32_t), 0);

    // 创建 ShaderResourceView（使用 RGBA 格式）
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_previewSRV.Reset();
    hr = m_pd3dDevice->CreateShaderResourceView(d3dTex.Get(), &srvDesc,
        m_previewSRV.ReleaseAndGetAddressOf());

    if (FAILED(hr)) {
        return;
    }

    // 保存纹理尺寸
    m_previewWidth = texWidth;
    m_previewHeight = texHeight;
}

void GameApp::SaveShadow(int pic)
{
    HRESULT hr = cpuReadBitmap->CopyFromBitmap(nullptr, ShadowBitmap.Get(), nullptr);
    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_U size = cpuReadBitmap->GetPixelSize();
        D2D1_MAPPED_RECT mappedRect;
        HRESULT hr = cpuReadBitmap->Map(D2D1_MAP_OPTIONS_READ, &mappedRect);

        if (SUCCEEDED(hr))
        {
            bool found = false;

            // 分配内存以存储像素数据
            BYTE* bitmapPixels = new BYTE[size.height * mappedRect.pitch];
            // 一口气复制所有数据给内存
            memcpy(bitmapPixels, mappedRect.bits, size.height * mappedRect.pitch);
            // 逐行分析数据
            for (UINT32 row = 0; row < size.height; ++row)
            {

                BYTE* destRow = bitmapPixels + row * mappedRect.pitch;
                for (UINT32 column = 0; column < size.width; ++column)
                {
                    UINT32 index = column * 4;
                    unsigned char alpha = destRow[index + 3];
                    if (alpha != 0) {

                        if (!found)
                        {
                            m_ShadowRect.left = column;
                            m_ShadowRect.top = row;
                            found = true;
                        }
                        if (column < m_ShadowRect.left)
                        {
                            m_ShadowRect.left = column;
                        }
                        if (column > m_ShadowRect.right)
                        {
                            m_ShadowRect.right = column;
                        }

                        m_ShadowRect.bottom = row;
                    }


                }


            }
            // 完成后解除映射
            cpuReadBitmap->Unmap();
            delete[] bitmapPixels;
            int cropWidth = m_ShadowRect.right - m_ShadowRect.left ;
            int cropHeight = m_ShadowRect.bottom - m_ShadowRect.top ;
            D2D1_RECT_U cropRect = D2D1::RectU(m_ShadowRect.left, m_ShadowRect.top, m_ShadowRect.right, m_ShadowRect.bottom);

            ComPtr<ID2D1Bitmap1> ShadowCpuFinalTexture;
            D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            D2D1_SIZE_U bitmapSize = D2D1::SizeU(cropWidth, cropHeight);
            m_pd2dImmediateContext->CreateBitmap(bitmapSize, nullptr, 0, bitmapProperties, &ShadowCpuFinalTexture);

            HRESULT hr = ShadowCpuFinalTexture->CopyFromBitmap(nullptr, ShadowBitmap.Get(), &cropRect);
            if (SUCCEEDED(hr))
            {
                D2D1_SIZE_U size_Final = ShadowCpuFinalTexture->GetPixelSize();
                D2D1_MAPPED_RECT mappedRect_Final;

                HRESULT hr = ShadowCpuFinalTexture->Map(D2D1_MAP_OPTIONS_READ, &mappedRect_Final);
                // Allocate memory <- pixel data
                if (SUCCEEDED(hr))
                {
                    if (m_CurrMode == ShowMode::PlayCreature)
                    {
                 
                        CHAR strFile[40];
                       
                      
                        sprintf(strFile, "..//Texture//%d.png", pic);

                        BYTE* bitmapPixels2 = new BYTE[size_Final.height * mappedRect_Final.pitch];
                        //Due to the alignment of memory, the width of sizefinal will be larger than we accurately crop
                        //The image will be a bit wider, so in the STB_write, we store cropwidth
                        memcpy(bitmapPixels2, mappedRect_Final.bits, size_Final.height * mappedRect_Final.pitch);
                        stbi_write_png(strFile, cropWidth, cropHeight, 4, bitmapPixels2, mappedRect_Final.pitch);
                        delete[] bitmapPixels2;
            


                        ShadowCpuFinalTexture->Unmap();
                        //ShadowCpuFinalTexture->Release();
                    }

                }

            }

            //Remember to release memory when no longer needed


        }







    }
}

void GameApp::ShowTable(bool& p_open, ACTION_FRAME_ARRAY& cfpk)
{
    static ImGuiTree& m_guitree = ImGuiTree::Get();
    //m_guitree.the_signal.connect(&GameApp::DrawSprite, this);
    m_guitree.ShowCFPKEditTable("CFPK", cfpk, &p_open);

}

void GameApp::ShowCpkEditGui(bool* p_open)
{
    static ImGuiTree& m_guitree = ImGuiTree::Get();
    static bool showFR = false;
    if (*p_open) {
        ImGui::SetNextWindowSize(ImVec2(500, 470), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Darkeden CreatureFarmePack Editor", p_open, ImGuiWindowFlags_MenuBar))
        {

            if (ImGui::BeginMenuBar())
            {

                ImGui::EndMenuBar();
            }

            static int selected = 0;
            {
                ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);

                for (int i = 0; i < m_CFPK.GetSize(); i++)
                {
                    // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
                    char label[128];
                    sprintf(label, "PACKET OBJECT %d", i);
                    if (ImGui::Selectable(label, selected == i))
                        selected = i;
                }




                ImGui::EndChild();
            }
            ImGui::SameLine();


            {
                ImGui::BeginGroup();
                ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
                ImGui::Text("OBJECT: %d", selected);

                float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
                ImGui::PushButtonRepeat(true);
                if (ImGui::ArrowButton("##left", ImGuiDir_Left))
                {
                    if (m_CurrFrame > 0)
                    {
                        m_CurrFrame--;
                    }
                }
                ImGui::SameLine(0.0f, spacing);
                if (ImGui::ArrowButton("##right", ImGuiDir_Right))
                {
                    if (m_CurrMode == ShowMode::PlayCreature)
                    {
                        m_CurrFrame = (m_CurrFrame + 1) % m_DrawingFrameSizeMin;
                    }
                    else {
                        m_CurrFrame = (m_CurrFrame + 1) % 4;
                    }
                }
                ImGui::PopButtonRepeat();
                ImGui::SameLine();

                ImGui::Text("%d", m_CurrFrame);





                ImGui::Checkbox("SHOW FR", &showFR);
                if (showFR)
                {
                    ShowTable(showFR, m_CFPK[selected]);
                }
                ImGui::SameLine();
                
                if (ImGui::Button("Play Creature"))
                {
                    m_CurrFrame = 0;

                    m_CurrMode = ShowMode::PlayCreature;
                    //下面开始处理 复制数据给Frame管理器
                    if (!my_selection.empty()) {
                        for (const auto& element : my_selection) {
                            std::string str(g_CreatureNames[element]);
                            
                            m_CurrFramePack = m_namemap[g_CreatureNames[element]];
                            if (m_guitree.isPlay()) {
                                m_CFPK_select.top = int(m_guitree.getElements());
                            }
                            FRAME_ARRAY& PlayFrame = m_CFPK[m_CurrFramePack][m_CFPK_select.top][m_CFPK_select.left];
                            std::shared_ptr<CAnimation> DirectionAnimat = std::make_shared<CAnimation>();
                            for (int i = 0; i < PlayFrame.GetSize(); ++i)
                            {

                                std::shared_ptr<CFrame> emptyFrame = std::make_shared<CFrame>(PlayFrame[i].GetSpriteID(), PlayFrame[i].GetCX(), PlayFrame[i].GetCY());
                                DirectionAnimat->AddFrame(emptyFrame);

                            }
                            
                            m_animationManager.AddAnimation(str, DirectionAnimat);



                        }

                        m_DrawingFrameSizeMin = m_animationManager.GetMinSize();


                    }


                }


                ImGui::Separator();



                if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
                {
                    
                    if (ImGui::BeginTabItem("DrawingOrderEditor"))
                    {
                        enum Mode
                        {
                            Mode_Assignment,
                            Mode_Move,
                            Mode_Swap
                        };
                        static int mode = 0;


                        if (ImGui::RadioButton("Assign", mode == Mode_Assignment)) { mode = Mode_Assignment; } ImGui::SameLine();
                        if (ImGui::RadioButton("Swap", mode == Mode_Swap)) { mode = Mode_Swap; }
                        for (int n = 0; n < IM_ARRAYSIZE(g_CreatureNames); n++)
                        {
                            ImGui::PushID(n);
                            if ((n % 3) != 0)
                                ImGui::SameLine();
                            
                            if (ImGui::Button(g_CreatureNames[n], ImVec2(60, 60))) {
                                // 当按钮被单击时，给映射赋值
                                if (mode == Mode_Assignment) {
                                    m_namemap[g_CreatureNames[n]] = selected;
                                }

                            }
                            // Our buttons are both drag sources and drag targets here!
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                            {
                                // 设置有效载荷以承载我们项目的索引（可以是任何内容）
                                ImGui::SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));

                                // Display preview (could be anything, e.g. when dragging an image we could decide to display
                                // the filename and a small preview of the image, etc.)
                                if (mode == Mode_Swap) { ImGui::Text("Swap %s", g_CreatureNames[n]); }
                                ImGui::EndDragDropSource();
                            }
                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
                                {
                                    IM_ASSERT(payload->DataSize == sizeof(int));
                                    int payload_n = *(const int*)payload->Data;

                                    if (mode == Mode_Swap)
                                    {
                                        const char* tmp = g_CreatureNames[n];
                                        g_CreatureNames[n] = g_CreatureNames[payload_n];
                                        g_CreatureNames[payload_n] = tmp;
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }


                            ImGui::PopID();
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("DrawingLayerSettings"))
                    {
                        for (int i = 0; i < 8; i++)
                        {
                            
                            ImGuiID id = i;
                            ImGui::PushID(id);
                            // 检查当前的列表项是否被选中
                            bool selected = my_selection.count(id) > 0;

                            if (ImGui::Selectable(g_CreatureNames[i], selected, ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_SpanAllColumns))
                            {
                                // 如果列表项被点击，切换它的选中状态
                                if (selected)
                                {
                                    // 如果已经被选中，从set对象中移除它
                                    my_selection.erase(id);
                                }
                                else
                                {
                                    // 如果没有被选中，添加到set对象中
                                    my_selection.insert(id);
                                }
                            }

                            ImGui::PopID();
                            

                        }
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }

                ImGui::EndChild();


                ImGui::EndGroup();
            }
            ImGui::End();
        }

    }
   
}


bool GameApp::InitResource()
{
    //一大堆硬编码
    m_Rotation = -60;
    m_DrawingFrameSizeMin = 0;
    m_drawMap[0] = 19;
    m_drawMap[1] = 21;
    m_drawMap[2] = 10;
    m_namemap = { {"Body", 0},
    { "pants", 1 },
    { "hair", 2 },
    { "Gun", 3 },
    { "knife", 4 },
    { "sword", 5 },
    { "Scepter", 6 },
    { "Shield", 7 },
    { "Motorcycle", 8 } };

    m_CurrFramePack = 0;
    m_CurrFrame = 0;
    ifstream m_CFPK_file;
    CIndexSprite::SetColorSet();
    m_CFPK_file.open("../Texture/europe_sm.cfpk", ifstream::binary);
    m_CFPK.LoadFromFile(m_CFPK_file);
    m_CFPK_file.close();

    // ========== 启动时不加载SPK文件，等待map加载后按需异步加载 ==========
    // 创建SPK对象（延迟加载）
    m_pSPK = new CBaseSpritePackVector;
    m_pISPK = new CBaseSpritePackVector;

    // 初始化SPK元数据为未加载状态
    m_SPKFiles[SPKType::Tile].startIndex = 0;
    m_SPKFiles[SPKType::Tile].count = 0;
    m_SPKFiles[SPKType::Tile].isLoaded = false;

    m_SPKFiles[SPKType::ImageObject].startIndex = 0;
    m_SPKFiles[SPKType::ImageObject].count = 0;
    m_SPKFiles[SPKType::ImageObject].isLoaded = false;

    m_LoadingStatus = "Ready. Select a map file to load sprites.";
    m_LoadingProgress = 0.0f;
    OutputDebugStringA("[Debug] InitResource finished (SPK files will be loaded on demand)\n");

    try {
        //m_pISPK->LoadFromFile(L"../Texture/test.spk");

        } 
    catch (const std::exception& e) {

         }
    //写死绘图区域
    m_sourceRect = D2D1::RectF(450, 200, 850, 600);
    m_destRect = D2D1::RectF(0, 0, 0, 0);
    m_CFPK_select = D2D1::RectF(0, 0, 0, 0);
    m_ShadowRect = D2D1::RectF(0, 0, 0, 0);
    m_RoleRect = D2D1::RectF(0, 0, 0, 0);
    int width, height, channels;
    //stbi_uc* imageData = stbi_load("blackimage.png", &width, &height, &channels, 4);
    //D2D1_SIZE_U bitmapSize = { width, height }; // Bitmap size
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = {}; // Bitmap properties
    //bitmapProperties.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM; // Bitmap format
    bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED; // Bitmap alpha mode
    //bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE; // Bitmap options

    //m_pd2dImmediateContext->CreateBitmap(
    //    bitmapSize, // Bitmap size
    //    imageData, // Image data
    //    width * channels, // Image data pitch
    //    &bitmapProperties, // Bitmap properties
    //    &d2dBitmap // Bitmap
    //);
    D2D1_SIZE_U bitmapSize = { 1280,720 };

    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET; // Bitmap options
    bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM; // Bitmap format
    m_pd2dImmediateContext->CreateBitmap(
        bitmapSize, // Bitmap size
        nullptr, // Image data
        0, // Image data pitch
        &bitmapProperties, // Bitmap properties
        d2dCLOBitmap.GetAddressOf() // Bitmap
    );

    bitmapSize = { 1280,720 }; // Bitmap size
    bitmapProperties = {}; // Bitmap properties
    bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM; // Bitmap format
    bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED; // Bitmap alpha mode
    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET; // Bitmap options

    m_pd2dImmediateContext->CreateBitmap(
        bitmapSize, // Bitmap size
        nullptr, // Image data
        0, // Image data pitch
        &bitmapProperties, // Bitmap properties
        &d2dSpriteBitmap // Bitmap
    );

    m_pd2dImmediateContext->CreateBitmap(
        bitmapSize, // Bitmap size
        nullptr, // Image data
        0, // Image data pitch
        &bitmapProperties, // Bitmap properties
        &ShadowBitmap // Bitmap
    );

    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;// Bitmap options
    m_pd2dImmediateContext->CreateBitmap(
        bitmapSize, // Bitmap size
        nullptr, // Image data
        0, // Image data pitch
        &bitmapProperties, // Bitmap properties
        &cpuReadBitmap // Bitmap
    );

    //stbi_image_free(imageData);
    //Create the Atlas Effect.
    m_pd2dImmediateContext->CreateEffect(CLSID_D2D1Shadow, &m_pEffect);
    //// Set the input.
    //m_pEffect->SetInput(0, d2dSpriteBitmap.Get());
 



    USHORT anims_size, SpriteID;
    FRAME_ARRAY m_pCPK;

    
    anims_size = m_pISPK->GetSize();
    //m_pSpriteAnims.resize(anims_size);
    //m_pTextureOutputS.resize(anims_size);
    return true;
}

void GameApp::LoadSPKFiles(LoadingProgressCallback progressCallback)
{
    //if (m_bLoading.exchange(true))
    //{
    //    OutputDebugStringA("[Warning] SPK loading already in progress\n");
    //    return;
    //}

    //OutputDebugStringA("[Info] Starting SPK loading (synchronous)...\n");
    //char buf[256];

    //// ========== 加载tile.spk ==========
    //m_LoadingStatus = "Loading tile.spk...";
    //progressCallback(0.0f, m_LoadingStatus.c_str());
    //OutputDebugStringA("[Info] Step 1: Loading tile.spk file...\n");

    //try {
    //    m_pSPK->LoadFromFile(L"../Texture/tile.spk");
    //    OutputDebugStringA("[Info] Step 2: tile.spk file loaded successfully\n");
    //} catch (const std::exception& e) {
    //    sprintf(buf, "[Error] Exception loading tile.spk: %s\n", e.what());
    //    OutputDebugStringA(buf);
    //    m_bLoading = false;
    //    return;
    //}

    //UINT32 tileCount = m_pSPK->GetSize();
    //sprintf(buf, "[Info] Step 3: tile.spk count = %u\n", tileCount);
    //OutputDebugStringA(buf);

    //// tile从索引0开始
    //m_SPKFiles[SPKType::Tile].startIndex = 0;
    //m_SPKFiles[SPKType::Tile].count = tileCount;
    //m_SPKFiles[SPKType::Tile].isLoaded = true;

    //// 加载tile纹理到TextureManager，显示进度
    //OutputDebugStringA("[Info] Step 4: Starting to create tile textures...\n");
    //for (UINT32 i = 0; i < tileCount; i++)
    //{
    //    CBaseSprite* m_SP = m_pSPK->GetData(i);
    //    if (m_SP)
    //    {
    //        D2D1_SIZE_U bitmapSize = { m_SP->GetWidth(), m_SP->GetHeight() };
    //        sprintf(buf, "[Debug] Creating tile texture %u, size %ux%u\n", i, bitmapSize.width, bitmapSize.height);
    //        OutputDebugStringA(buf);
    //        m_TextureManager.CreateFromMemory(i, m_SP->GetImage(), bitmapSize, false, false);
    //    }
    //    // 每100张更新一次进度
    //    if (i % 100 == 0 || i == tileCount - 1)
    //    {
    //        float progress = 0.5f * ((float)i / (float)tileCount);
    //        m_LoadingProgress = progress;
    //        m_LoadingStatus = "Loading tile textures: " + std::to_string(i + 1) + "/" + std::to_string(tileCount);
    //        progressCallback(progress, m_LoadingStatus.c_str());
    //    }
    //}


    //// ========== 加载ImageObject.spk ==========
    //m_LoadingStatus = "Loading ImageObject.spk...";
    //progressCallback(0.5f, m_LoadingStatus.c_str());
    //

    //try {
    //    m_pISPK->LoadFromFile(L"../Texture/ImageObject.spk");
    //    OutputDebugStringA("[Info] Step 7: ImageObject.spk file loaded successfully\n");
    //} catch (const std::exception& e) {
    //    sprintf(buf, "[Error] Exception loading ImageObject.spk: %s\n", e.what());
    //    OutputDebugStringA(buf);
    //    m_bLoading = false;
    //    return;
    //}

    //UINT32 imageObjectCount = m_pISPK->GetSize();
    //m_SPKFiles[SPKType::ImageObject].count = imageObjectCount;
    //m_SPKFiles[SPKType::ImageObject].isLoaded = true;
    //sprintf(buf, "[Info] Step 8: ImageObject.spk count = %u\n", imageObjectCount);
    //OutputDebugStringA(buf);

    //// 加载ImageObject纹理到TextureManager，显示进度
    //OutputDebugStringA("[Info] Step 9: Starting to create ImageObject textures...\n");
    //for (UINT32 i = 0; i < imageObjectCount; i++)
    //{
    //    CBaseSprite* m_SP = m_pISPK->GetData(i);
    //    if (m_SP)
    //    {
    //        D2D1_SIZE_U bitmapSize = { m_SP->GetWidth(), m_SP->GetHeight() };
    //        // 使用全局索引 = tileCount + i
    //        UINT32 globalIndex = tileCount + i;
    //        m_TextureManager.CreateFromMemory(globalIndex, m_SP->GetImage(), bitmapSize, false, false);
    //    }
    //    // 每100张更新一次进度
    //    if (i % 100 == 0 || i == imageObjectCount - 1)
    //    {
    //        float progress = 0.5f + 0.5f * ((float)i / (float)imageObjectCount);
    //        m_LoadingProgress = progress;
    //        m_LoadingStatus = "Loading ImageObject textures: " + std::to_string(i + 1) + "/" + std::to_string(imageObjectCount);
    //        progressCallback(progress, m_LoadingStatus.c_str());
    //    }
    //}
    //OutputDebugStringA("[Info] Step 10: All textures created\n");

    //m_LoadingProgress = 1.0f;
    //m_LoadingStatus = "Loading complete!";
    //progressCallback(1.0f, m_LoadingStatus.c_str());

    //m_bLoading = false;
    //OutputDebugStringA("[Info] SPK loading completed\n");
}

void GameApp::AsyncLoadMapFile(const std::wstring& filePath)
{
    if (m_bMapLoading.exchange(true))
    {
        OutputDebugStringA("[Warning] Map loading already in progress\n");
        return;
    }

    m_MapFilePath = filePath;

    // 先释放旧的zone数据
    m_Zone.Release();
    m_MapLayerManager.ClearLayers();
    m_bMapReady = false;

    // 重置视口位置到地图中心
    m_ViewportX = 0;
    m_ViewportY = 0;
    m_ZoomLevel = 1.0f;

    // 简化：同步加载map文件
    std::ofstream logFile("debug_log.txt", std::ios::app);
    logFile << "[Debug] Loading map..." << std::endl;
    logFile.close();

    // 打开map文件
    std::ifstream file;
    file.open(filePath, std::ios::binary);
    if (!file.is_open())
    {
        OutputDebugStringA("[Error] Failed to open map file\n");
        m_bMapLoading = false;
        return;
    }

    // 加载map数据
    bool success = m_Zone.LoadFromFile(file);
    file.close();

    if (success)
    {
        // 清除之前的 sector 选择
        m_selectedSectors.clear();

        // 构建复合对象
        m_Zone.BuildCompositeObjects();
        // 构建viewpoint索引（用于高效渲染）
        m_Zone.BuildImageObjectViewpointIndex();

        uint32_t mapWidth = m_Zone.GetWidth();
        uint32_t mapHeight = m_Zone.GetHeight();

        char buf[256];
        sprintf(buf, "[Debug] Map size: %ux%u\n", mapWidth, mapHeight);
        OutputDebugStringA(buf);

        // 收集需要的tile ID
        for (uint32_t y = 0; y < mapHeight; y++)
        {
            for (uint32_t x = 0; x < mapWidth; x++)
            {
                GameSector* sector = m_Zone.GetSector(x, y);
                if (sector)
                {
                    uint16_t tileID = sector->GetSpriteID();
                    // tileID 为 0 也需要处理（可能是空tile或默认tile）
                    m_requiredTileIDs.insert(tileID);
                }
            }
        }

        // 收集需要的 sprite ID
        const auto& allImageObjects = m_Zone.GetAllImageObjects();
        for (const auto& pair : allImageObjects)
        {
            const std::vector<MImageObject>& objList = pair.second;
            for (const MImageObject& imgObj : objList)
            {
                m_requiredSpriteIDs.insert(imgObj.GetSpriteID());
            }
        }

        sprintf(buf, "[Info] Map loaded: tiles=%zu, objects=%zu\n",
            m_requiredTileIDs.size(), m_requiredSpriteIDs.size());
        OutputDebugStringA(buf);

        // 触发异步加载sprite（按需加载）
        AsyncLoadRequiredSprites();
    }
    else
    {
        OutputDebugStringA("[Error] Failed to load map\n");
    }

    m_bMapLoading = false;
    OutputDebugStringA("[Debug] Map loading finished\n");
}

void GameApp::LoadRequiredSpritesForMap()
{
    if (m_requiredTileIDs.empty() && m_requiredSpriteIDs.empty())
    {
        OutputDebugStringA("[Info] No required sprites to load\n");
        return;
    }

    OutputDebugStringA("[Info] Loading required sprites for map...\n");
    char buf[256];

    // ========== 1. 加载tile.spk文件 ==========
    if (!m_SPKFiles[SPKType::Tile].isLoaded)
    {
        OutputDebugStringA("[Info] Loading tile.spk file...\n");
        m_pSPK->LoadFromFile(L"../Texture/tile.spk");
        m_SPKFiles[SPKType::Tile].isLoaded = true;
        m_SPKFiles[SPKType::Tile].count = m_pSPK->GetSize();
    }

    // ========== 2. 按需加载需要的tile纹理 ==========
    int tileLoadedCount = 0;
    for (uint32_t tileID : m_requiredTileIDs)
    {
        // 检查是否已经加载
        if (m_TextureManager.GetTexture(tileID) == nullptr)
        {
            CBaseSprite* sprite = m_pSPK->GetData(tileID);
            if (sprite && sprite->GetWidth() > 0 && sprite->GetHeight() > 0)
            {
                D2D1_SIZE_U bitmapSize = { sprite->GetWidth(), sprite->GetHeight() };
                m_TextureManager.CreateFromMemory(tileID, sprite->GetImage(), bitmapSize, false, false);
                tileLoadedCount++;
            }
        }
    }
    sprintf(buf, "[Info] Loaded %d / %zu required tiles\n", tileLoadedCount, m_requiredTileIDs.size());
    OutputDebugStringA(buf);

    // ========== 3. ImageObject已经在InitResource中全部预加载了 ==========
    // 这里不需要做任何事情

    // ========== 4. ImageObject纹理已经在InitResource中全部加载 ==========
    // 无需按需加载，纹理已经存在
    sprintf(buf, "[Info] ImageObject already loaded in InitResource, count=%zu\n", m_requiredSpriteIDs.size());
    OutputDebugStringA(buf);

    // 加载完成后，创建并绘制Tile层
    UpdateMapTileLayer();

    // 标记地图图层需要渲染
    m_bMapLayersDirty = true;

    // 通知观察者加载完成
    NotifyAllSpritesLoadComplete();
}

// ========== 资源加载观察者实现 ==========
void GameApp::RegisterLoadObserver(IResourceLoadObserver* observer) {
    if (observer) {
        m_loadObservers.push_back(observer);
    }
}

void GameApp::UnregisterLoadObserver(IResourceLoadObserver* observer) {
    auto it = std::find(m_loadObservers.begin(), m_loadObservers.end(), observer);
    if (it != m_loadObservers.end()) {
        m_loadObservers.erase(it);
    }
}

void GameApp::NotifySpriteLoadComplete(uint32_t spriteID, bool success) {
    for (auto* observer : m_loadObservers) {
        if (observer) {
            observer->OnSpriteLoadComplete(spriteID, success);
        }
    }
}

void GameApp::NotifyAllSpritesLoadComplete() {
    OutputDebugStringA("[Info] Notifying observers: All sprites loaded\n");
    m_bSpritesLoading = false;
    m_bSpritesLoaded = true;
    for (auto* observer : m_loadObservers) {
        if (observer) {
            observer->OnAllSpritesLoadComplete();
        }
    }
}

// 异步按需加载sprite
void GameApp::AsyncLoadRequiredSprites() {
    if (m_requiredTileIDs.empty() && m_requiredSpriteIDs.empty()) {
        OutputDebugStringA("[Info] No required sprites to load\n");
        NotifyAllSpritesLoadComplete();
        return;
    }

    // 防止重复启动加载
    bool expected = false;
    if (!m_bSpritesLoading.compare_exchange_strong(expected, true)) {
        OutputDebugStringA("[Warning] Sprite loading already in progress\n");
        return;
    }

    m_asyncLoadProgress = 0.0f;
    m_asyncLoadStatus = "Starting...";
    OutputDebugStringA("[Info] Starting async sprite loading...\n");

    std::thread([this]() {
        char buf[256];
        OutputDebugStringA("[Async] Thread started\n");

        try {
            // 输出纹理目录
            sprintf(buf, "[Async] Using texture directory: %ls\n", m_textureDirectory.c_str());
            OutputDebugStringA(buf);

            // ========== 第一步：从磁盘加载SPK文件 ==========
            OutputDebugStringA("[Async] Step 1: Loading SPK files from disk...\n");

            // 1.1 加载 tile.spk
            if (!m_SPKFiles[SPKType::Tile].isLoaded) {
                OutputDebugStringA("[Async] Loading tile.spk...\n");
                m_asyncLoadStatus = "Loading tile.spk...";
                m_asyncLoadProgress = 0.05f;

                // 构建完整路径
                wchar_t tilePath[MAX_PATH];
                wcscpy_s(tilePath, MAX_PATH, m_textureDirectory.c_str());
                wcscat_s(tilePath, MAX_PATH, L"\\tile.spk");
                sprintf(buf, "[Async] Full path: %ls\n", tilePath);
                OutputDebugStringA(buf);

                // 检查文件是否存在
                std::ifstream testFile(tilePath, std::ios::binary);
                if (!testFile.is_open()) {
                    OutputDebugStringA("[Async] ERROR: Cannot open tile.spk\n");
                    m_asyncLoadStatus = "Error: Cannot find tile.spk";
                    m_bSpritesLoading = false;
                    return;
                }
                testFile.close();
                OutputDebugStringA("[Async] tile.spk file exists, loading...\n");

                // 加载文件
                bool loadResult = m_pSPK->LoadFromFile(tilePath);
                sprintf(buf, "[Async] LoadFromFile returned: %d\n", loadResult);
                OutputDebugStringA(buf);

                if (!loadResult) {
                    OutputDebugStringA("[Async] ERROR: LoadFromFile failed\n");
                    m_asyncLoadStatus = "Error: Failed to load tile.spk";
                    m_bSpritesLoading = false;
                    return;
                }

                UINT32 tileCount = m_pSPK->GetSize();
                sprintf(buf, "[Async] tile.spk loaded, count=%u\n", tileCount);
                OutputDebugStringA(buf);

                if (tileCount == 0) {
                    OutputDebugStringA("[Async] ERROR: tile.spk count is 0\n");
                    m_asyncLoadStatus = "Error: tile.spk is empty";
                    m_bSpritesLoading = false;
                    return;
                }

                m_SPKFiles[SPKType::Tile].count = tileCount;
                m_SPKFiles[SPKType::Tile].isLoaded = true;
            }

            // 1.2 加载 ImageObject.spk
            if (!m_SPKFiles[SPKType::ImageObject].isLoaded) {
                OutputDebugStringA("[Async] Loading ImageObject.spk...\n");
                m_asyncLoadStatus = "Loading ImageObject.spk...";
                m_asyncLoadProgress = 0.1f;

                // 构建完整路径
                wchar_t imageObjPath[MAX_PATH];
                wcscpy_s(imageObjPath, MAX_PATH, m_textureDirectory.c_str());
                wcscat_s(imageObjPath, MAX_PATH, L"\\ImageObject.spk");
                sprintf(buf, "[Async] Full path: %ls\n", imageObjPath);
                OutputDebugStringA(buf);

                // 检查文件是否存在
                std::ifstream testFile2(imageObjPath, std::ios::binary);
                if (!testFile2.is_open()) {
                    OutputDebugStringA("[Async] ERROR: Cannot open ImageObject.spk\n");
                    m_asyncLoadStatus = "Error: Cannot find ImageObject.spk";
                    m_bSpritesLoading = false;
                    return;
                }
                testFile2.close();
                OutputDebugStringA("[Async] ImageObject.spk file exists, loading...\n");

                // 加载文件
                bool loadResult = m_pISPK->LoadFromFile(imageObjPath);
                sprintf(buf, "[Async] LoadFromFile returned: %d\n", loadResult);
                OutputDebugStringA(buf);

                if (!loadResult) {
                    OutputDebugStringA("[Async] ERROR: LoadFromFile failed\n");
                    m_asyncLoadStatus = "Error: Failed to load ImageObject.spk";
                    m_bSpritesLoading = false;
                    return;
                }

                UINT32 imageObjectCount = m_pISPK->GetSize();
                sprintf(buf, "[Async] ImageObject.spk loaded, count=%u\n", imageObjectCount);
                OutputDebugStringA(buf);

                if (imageObjectCount == 0) {
                    OutputDebugStringA("[Async] ERROR: ImageObject.spk count is 0\n");
                    m_asyncLoadStatus = "Error: ImageObject.spk is empty";
                    m_bSpritesLoading = false;
                    return;
                }

                m_SPKFiles[SPKType::ImageObject].count = imageObjectCount;
                m_SPKFiles[SPKType::ImageObject].startIndex = m_SPKFiles[SPKType::Tile].count;
                m_SPKFiles[SPKType::ImageObject].isLoaded = true;
            }

            OutputDebugStringA("[Async] All SPK files loaded from disk.\n");

            // ========== 第二步：上传纹理到GPU ==========
            OutputDebugStringA("[Async] Step 2: Uploading textures to GPU...\n");

            // 2.1 上传 tile 纹理
            int tileLoadedCount = 0;
            int tileSkippedCount = 0;
            size_t totalTiles = m_requiredTileIDs.size();
            UINT32 tileCount = m_SPKFiles[SPKType::Tile].count;
            for (uint32_t tileID : m_requiredTileIDs) {
                // 检查 tileID 是否有效
                if (tileID >= tileCount || tileID == 0xFFFF) {
                    tileSkippedCount++;
                    sprintf(buf, "[Async] WARNING: Skipping invalid tileID=%u (max=%u)\n", tileID, tileCount);
                    OutputDebugStringA(buf);
                    continue;
                }

                if (m_TextureManager.GetTexture(tileID) == nullptr) {
                    CBaseSprite* sprite = m_pSPK->GetData(tileID);
                    if (sprite) {
                        D2D1_SIZE_U bitmapSize = { sprite->GetWidth(), sprite->GetHeight() };
                        m_TextureManager.CreateFromMemory(tileID, sprite->GetImage(), bitmapSize, false, false);
                        tileLoadedCount++;
                    }
                }
            }
            sprintf(buf, "[Async] Uploaded %d / %zu tile textures, skipped %d invalid\n", tileLoadedCount, totalTiles, tileSkippedCount);
            OutputDebugStringA(buf);
            m_asyncLoadProgress = 0.5f;

            // 2.2 上传 ImageObject 纹理
            int imageObjLoadedCount = 0;
            int imageObjSkippedCount = 0;
            UINT32 imageObjStartIndex = m_SPKFiles[SPKType::ImageObject].startIndex;
            UINT32 imageObjCount = m_SPKFiles[SPKType::ImageObject].count;
            size_t totalObjs = m_requiredSpriteIDs.size();
            for (uint32_t imageID : m_requiredSpriteIDs) {
                // 检查 imageID 是否有效
                if (imageID >= imageObjCount || imageID == 0xFFFF) {
                    imageObjSkippedCount++;
                    sprintf(buf, "[Async] WARNING: Skipping invalid imageID=%u (max=%u)\n", imageID, imageObjCount);
                    OutputDebugStringA(buf);
                    continue;
                }

                UINT32 globalIndex = imageObjStartIndex + imageID;
                if (m_TextureManager.GetTexture(globalIndex) == nullptr) {
                    CBaseSprite* sprite = m_pISPK->GetData(imageID);
                    if (sprite) {
                        WORD w = sprite->GetWidth();
                        WORD h = sprite->GetHeight();
                        D2D1_SIZE_U bitmapSize = { w, h };
                        ID2D1Bitmap1* tex = m_TextureManager.CreateFromMemory(globalIndex, sprite->GetImage(), bitmapSize, false, false);
                        if (tex) {
                            imageObjLoadedCount++;
                            // 调试：检查加载的纹理尺寸
                            static UINT32 debugImageID = 1844;
                            if (imageID == debugImageID) {
                                D2D1_SIZE_U loadedSize = tex->GetPixelSize();
                                sprintf(buf, "[AsyncDBG] imageID=%u globalIndex=%u requested=%ux%u loaded=%ux%u\n",
                                    imageID, globalIndex, w, h, loadedSize.width, loadedSize.height);
                                OutputDebugStringA(buf);
                            }
                        } else {
                            sprintf(buf, "[Async] ERROR: Failed to create texture for imageID=%u globalIndex=%u size=%ux%u\n",
                                imageID, globalIndex, bitmapSize.width, bitmapSize.height);
                            OutputDebugStringA(buf);
                        }
                    } else {
                        sprintf(buf, "[Async] WARNING: GetData returned nullptr for imageID=%u\n", imageID);
                        OutputDebugStringA(buf);
                    }
                }
            }
            sprintf(buf, "[Async] Uploaded %d / %zu ImageObject textures, skipped %d invalid\n", imageObjLoadedCount, totalObjs, imageObjSkippedCount);
            OutputDebugStringA(buf);

            m_asyncLoadStatus = "Loading complete!";
            m_asyncLoadProgress = 1.0f;
            OutputDebugStringA("[Async] All textures uploaded.\n");

            // 标记需要在主线程更新地图图层
            m_bPendingTileLayerUpdate = true;
            NotifyAllSpritesLoadComplete();
            OutputDebugStringA("[Async] Done.\n");
        }
        catch (const std::exception& e) {
            sprintf(buf, "[Async] Exception: %s\n", e.what());
            OutputDebugStringA(buf);
            m_asyncLoadStatus = "Error: ";
            m_asyncLoadStatus += e.what();
            m_bSpritesLoading = false;
        }
    }).detach();
}

void GameApp::UpdateMapTileLayer()
{
    OutputDebugStringA("[Info] ===== UpdateMapTileLayer started =====\n");

    uint32_t mapWidth = m_Zone.GetWidth();
    uint32_t mapHeight = m_Zone.GetHeight();
    char buf[256];

    sprintf(buf, "[Info] Map size: %ux%u\n", mapWidth, mapHeight);
    OutputDebugStringA(buf);

    if (mapWidth == 0 || mapHeight == 0)
    {
        OutputDebugStringA("[Error] Invalid map size\n");
        return;
    }

    // 检查纹理管理器状态
    sprintf(buf, "[Info] TextureManager has %zu textures\n", m_TextureManager.GetTextureCount());
    OutputDebugStringA(buf);

    // 检查是否能获取到tile纹理
    if (!m_requiredTileIDs.empty())
    {
        uint32_t firstTileID = *m_requiredTileIDs.begin();
        ID2D1Bitmap1* testTex = m_TextureManager.GetTexture(firstTileID);
        sprintf(buf, "[Info] First tile ID: %u, texture pointer: %p\n", firstTileID, (void*)testTex);
        OutputDebugStringA(buf);
    }


   
    UINT layerWidth = mapWidth*48;
    UINT layerHeight = mapHeight*24;


    m_MapLayerManager.ClearLayers();
    bool layerCreated = m_MapLayerManager.CreateLayer(LayerType::TileLayer, layerWidth, layerHeight);
    sprintf(buf, "[Info] CreateLayer result: %d\n", layerCreated);
    OutputDebugStringA(buf);

    // 创建ObjectLayer（需要足够大以容纳所有 ImageObject）
    bool objLayerCreated = m_MapLayerManager.CreateLayer(LayerType::ObjectLayer, layerWidth, layerHeight);
    sprintf(buf, "[Info] CreateObjectLayer result: %d\n", objLayerCreated);
    OutputDebugStringA(buf);

    // 绘制tile层
    OutputDebugStringA("[Info] Calling UpdateTileLayer...\n");
    if (m_MapLayerManager.UpdateTileLayer(m_Zone, &m_TextureManager, 48, 24))
    {
        OutputDebugStringA("[Info] Tile layer updated successfully\n");
    }
    else
    {
        OutputDebugStringA("[Error] Failed to update tile layer\n");
    }

    // 绘制ImageObject层
    // 获取 ImageObject.spk 的起始索引
    UINT32 imageObjectStartIndex = m_SPKFiles[SPKType::ImageObject].startIndex;
    OutputDebugStringA("[Info] Calling UpdateObjectLayer...\n");
    if (m_MapLayerManager.UpdateObjectLayer(m_Zone, &m_TextureManager, imageObjectStartIndex, 48, 24))
    {
        OutputDebugStringA("[Info] Object layer updated successfully\n");
    }
    else
    {
        OutputDebugStringA("[Error] Failed to update object layer\n");
    }

    OutputDebugStringA("[Info] ===== UpdateMapTileLayer finished =====\n");
}

// 绘制map层到指定的目标bitmap（带视口裁剪）
void GameApp::DrawMapLayersToTarget(ID2D1Bitmap1* targetBitmap)
{
    if (!targetBitmap)
        return;

    // 窗口尺寸
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // 视口像素偏移（从格子坐标转换为像素坐标）
    float viewportOffsetX = (float)m_ViewportX * m_TileWidth;
    float viewportOffsetY = (float)m_ViewportY * m_TileHeight;

    // 计算视口尺寸（带缩放）
    float viewportPixelW = windowWidth / m_ZoomLevel;
    float viewportPixelH = windowHeight / m_ZoomLevel;

    // 源矩形：从地图的视口位置裁剪
    D2D1_RECT_F srcRect = D2D1::RectF(
        viewportOffsetX,
        viewportOffsetY,
        viewportOffsetX + viewportPixelW,
        viewportOffsetY + viewportPixelH
    );

    // 目标矩形：填满窗口
    D2D1_RECT_F destRect = D2D1::RectF(0, 0, windowWidth, windowHeight);

    // 绘制Tile层
    ID2D1Bitmap1* tileLayer = m_MapLayerManager.GetLayer(LayerType::TileLayer);
    ID2D1Bitmap1* objectLayer = m_MapLayerManager.GetLayer(LayerType::ObjectLayer);

    if (tileLayer)
    {
        m_pd2dImmediateContext->DrawBitmap(
            tileLayer,
            destRect,
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            &srcRect
        );
    }

    // 绘制Object层（ImageObject）
    if (objectLayer)
    {
        m_pd2dImmediateContext->DrawBitmap(
            objectLayer,
            destRect,
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            &srcRect
        );
    }
    // 高亮矩形现在在 DrawScene 中每帧实时绘制
}

// 旧版本保留兼容
void GameApp::DrawMapLayers()
{
    char buf[256];
    sprintf(buf, "[Debug] DrawMapLayers: m_bMapReady=%d, viewport=(%d,%d), zoom=%.2f\n",
        m_bMapReady ? 1 : 0, m_ViewportX, m_ViewportY, m_ZoomLevel);
    OutputDebugStringA(buf);

    // 窗口尺寸
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // 视口像素偏移（从格子坐标转换为像素坐标）
    float viewportOffsetX = (float)m_ViewportX * m_TileWidth;
    float viewportOffsetY = (float)m_ViewportY * m_TileHeight;

    // 计算视口尺寸（带缩放）
    float viewportPixelW = windowWidth / m_ZoomLevel;
    float viewportPixelH = windowHeight / m_ZoomLevel;

    sprintf(buf, "[Debug] DrawMapLayers: vpOffset=(%.1f,%.1f), vpSize=(%.1f,%.1f)\n",
        viewportOffsetX, viewportOffsetY, viewportPixelW, viewportPixelH);
    OutputDebugStringA(buf);

    // 源矩形：从地图的视口位置裁剪
    D2D1_RECT_F srcRect = D2D1::RectF(
        viewportOffsetX,
        viewportOffsetY,
        viewportOffsetX + viewportPixelW,
        viewportOffsetY + viewportPixelH
    );

    // 目标矩形：填满窗口
    D2D1_RECT_F destRect = D2D1::RectF(0, 0, windowWidth, windowHeight);

    // 绘制Tile层
    ID2D1Bitmap1* tileLayer = m_MapLayerManager.GetLayer(LayerType::TileLayer);
    ID2D1Bitmap1* objectLayer = m_MapLayerManager.GetLayer(LayerType::ObjectLayer);
    sprintf(buf, "[Debug] DrawMapLayers: tileLayer=%p, objectLayer=%p\n",
        (void*)tileLayer, (void*)objectLayer);
    OutputDebugStringA(buf);

    if (tileLayer)
    {
        // 获取tileLayer的尺寸
        D2D1_SIZE_U tileSize = tileLayer->GetPixelSize();
        sprintf(buf, "[Debug] DrawMapLayers: tileLayer size=%ux%u\n", tileSize.width, tileSize.height);
        OutputDebugStringA(buf);

        m_pd2dImmediateContext->DrawBitmap(
            tileLayer,
            destRect,
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            &srcRect
        );
        OutputDebugStringA("[Debug] DrawMapLayers: Tile layer drawn\n");
    }

    // 绘制Object层（ImageObject）
    if (objectLayer)
    {
        // 混合模式绘制Object层（保留透明度）
        m_pd2dImmediateContext->DrawBitmap(
            objectLayer,
            destRect,
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            &srcRect
        );
        OutputDebugStringA("[Debug] DrawMapLayers: Object layer drawn\n");
    }
}

// 按格子移动视口（带边界限制：视口中心不超边界）
void GameApp::MoveViewport(int deltaX, int deltaY)
{
    uint32_t mapWidth = m_Zone.GetWidth();
    uint32_t mapHeight = m_Zone.GetHeight();

    if (mapWidth == 0 || mapHeight == 0)
        return;

    // 视口显示的sector数量（基于1280x720窗口和当前缩放）
    float viewportSectorsXf = 1280.0f / (m_TileWidth * m_ZoomLevel);
    float viewportSectorsYf = 720.0f / (m_TileHeight * m_ZoomLevel);

    // 使用浮点数计算边界，避免整数除法精度问题
    // 视口左上角范围：[0, mapSize - viewportSize]
    // 这样视口中心会在 [viewportSize/2, mapSize - viewportSize/2] 范围内
    int maxX = (int)mapWidth - (int)viewportSectorsXf - 1;
    int maxY = (int)mapHeight - (int)viewportSectorsYf - 1;

    // 确保至少为0
    if (maxX < 0) maxX = 0;
    if (maxY < 0) maxY = 0;

    int newX = m_ViewportX + deltaX;
    int newY = m_ViewportY + deltaY;

    // 限制在边界内
    if (newX < 0) newX = 0;
    if (newY < 0) newY = 0;
    if (newX > maxX) newX = maxX;
    if (newY > maxY) newY = maxY;

    m_ViewportX = newX;
    m_ViewportY = newY;
}

// 直接设置视口位置（用于导航窗拖动）
void GameApp::SetViewportPosition(int x, int y)
{
    uint32_t mapWidth = m_Zone.GetWidth();
    uint32_t mapHeight = m_Zone.GetHeight();

    if (mapWidth == 0 || mapHeight == 0)
        return;

    // 视口显示的sector数量
    float viewportSectorsXf = 1280.0f / (m_TileWidth * m_ZoomLevel);
    float viewportSectorsYf = 720.0f / (m_TileHeight * m_ZoomLevel);

    int maxX = (int)mapWidth - (int)viewportSectorsXf - 1;
    int maxY = (int)mapHeight - (int)viewportSectorsYf - 1;

    if (maxX < 0) maxX = 0;
    if (maxY < 0) maxY = 0;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > maxX) x = maxX;
    if (y > maxY) y = maxY;

    m_ViewportX = x;
    m_ViewportY = y;
}

// 设置缩放级别
void GameApp::SetZoom(float zoom)
{
    // 限制缩放范围：0.25x - 4.0x
    if (zoom < 0.25f) zoom = 0.25f;
    if (zoom > 4.0f) zoom = 4.0f;
    m_ZoomLevel = zoom;
}

// 处理键盘输入（带延迟减速）
void GameApp::HandleKeyboardInput()
{
    ImGuiIO& io = ImGui::GetIO();

    // 更新移动计时器
    m_ViewportMoveTimer -= 1.0f / 60.0f;  // 假设60fps
    if (m_ViewportMoveTimer < 0) m_ViewportMoveTimer = 0;

    bool canMove = (m_ViewportMoveTimer <= 0);
    bool moved = false;

    // 方向键移动视口（每次1个sector，带延迟）
    if (io.KeysDown[VK_UP] || io.KeysDown['W'])
    {
        if (canMove) { MoveViewport(0, -1); moved = true; }
    }
    else if (io.KeysDown[VK_DOWN] || io.KeysDown['S'])
    {
        if (canMove) { MoveViewport(0, 1); moved = true; }
    }
    else if (io.KeysDown[VK_LEFT] || io.KeysDown['A'])
    {
        if (canMove) { MoveViewport(-1, 0); moved = true; }
    }
    else if (io.KeysDown[VK_RIGHT] || io.KeysDown['D'])
    {
        if (canMove) { MoveViewport(1, 0); moved = true; }
    }

    if (moved)
    {
        m_ViewportMoveTimer = m_ViewportMoveDelay;
    }

    // Q/E 缩放
    if (io.KeysDown['Q'])
    {
        SetZoom(m_ZoomLevel - 0.02f);
    }
    if (io.KeysDown['E'])
    {
        SetZoom(m_ZoomLevel + 0.02f);
    }
}

void GameApp::DrawRectangle(D2D1_RECT_F* desRc)
{

    m_pd2dImmediateContext->SetTarget(d2dCLOBitmap.Get());
    m_pd2dImmediateContext->BeginDraw();
    m_pd2dImmediateContext->Clear(D2D1::ColorF(D2D1::ColorF::LawnGreen));




    m_pd2dImmediateContext->DrawLine(
        D2D1::Point2F(650.0f, desRc->top),
        D2D1::Point2F(650.0f, desRc->bottom),
        m_pColorBrush.Get(),
        0.5f
    );
    m_pd2dImmediateContext->DrawLine(
        D2D1::Point2F(desRc->left, 450),
        D2D1::Point2F(desRc->right, 450),
        m_pColorBrush.Get(),
        0.5f
    );
    m_pd2dImmediateContext->EndDraw();
}