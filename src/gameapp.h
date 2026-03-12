#ifndef GAMEAPP_H
#define GAMEAPP_H

/*
 --------------------------------
 整个UI重写, 20240218 - 已经完成
 --------------------------------
 TODO
 实现影子生成,离屏保存为PNG 打包用其它工具做
 这里只专注写CPFK 20240303
*/

#include "d2dapp.h"
#include "FR.h"
#include "SP.h"
#include <map>
#include <set>
#include <memory>
#include <string>
#include <fstream>
#include "SpreiteFrameMap.h"

#include "CAnimationManager.h"
#include "Zone.h"
#include "MapLayerManager.h"
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <mutex>

// SPK文件元数据结构
struct SPKFileInfo
{
    std::wstring fileName;      // 文件名
    UINT32 startIndex;          // 起始索引
    UINT32 count;               // 资源数量
    bool isLoaded;              // 是否加载完成
};

enum class SPKType { ImageObject, Tile };

// 资源加载观察者接口
class IResourceLoadObserver {
public:
    virtual ~IResourceLoadObserver() = default;
    virtual void OnSpriteLoadComplete(uint32_t spriteID, bool success) = 0;
    virtual void OnAllSpritesLoadComplete() = 0;
};

enum class ShowMode { Staticdisplay, PlayAnim,PlayCreature };
typedef CTypePackVector<CIndexSprite> CIndexSpritePackVector;
typedef CTypePackVector<CBaseSprite> CBaseSpritePackVector;
class GameApp : public D2DApp 
{
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    virtual ~GameApp();
    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();
    void ShowMainGui();
    void ShowTable(bool& p_open, ACTION_FRAME_ARRAY& cfpk);
    void ShowCpkEditGui(bool* p_open);
    void DrawEffect(FRAME_ARRAY& FA);
    void DrawSprite(std::shared_ptr<CFrame> pFrame3);
    void DrawRectangle(D2D1_RECT_F* desRc );
    void EditEffect();
    void SaveShadow(int pic);
    //void LoadResource(UINT16 num);
private:
    bool Keepitforever;
    bool InitResource();

    // SPK文件元数据
    std::map<SPKType, SPKFileInfo> m_SPKFiles;
    std::atomic<bool> m_bLoading;
    float m_LoadingProgress;        // 加载进度 0.0 - 1.0
    std::string m_LoadingStatus;   // 加载状态文本
    std::mutex m_LoadingMutex;     // 进度更新锁

    // 异步加载进度（用于UI显示）
    std::atomic<float> m_asyncLoadProgress;  // 异步加载进度 0.0 - 1.0
    std::string m_asyncLoadStatus;            // 异步加载状态文本
    std::mutex m_asyncLoadMutex;              // 异步加载状态锁

    // 资源目录路径（初始化时获取）
    std::wstring m_textureDirectory;         // Texture 目录的完整路径

    // SPK索引计算函数
    UINT32 GetGlobalTextureIndex(SPKType type, UINT32 localIndex);
    bool ValidateTextureIndex(SPKType type, UINT32 localIndex);

    // 同步加载SPK文件（带进度回调）
    typedef std::function<void(float progress, const char* status)> LoadingProgressCallback;
    void LoadSPKFiles(LoadingProgressCallback progressCallback);

    // 按需加载单个纹理
    bool LoadTextureOnDemand(SPKType type, UINT32 localIndex);

    // Map文件相关
    Zone m_Zone;                           // Maplib核心对象
    std::wstring m_MapFilePath;            // 当前选中的map文件路径
    std::atomic<bool> m_bMapLoading;        // map加载状态
    std::thread m_MapLoadThread;           // map加载线程
    std::set<uint32_t> m_requiredSpriteIDs; // map需要的ImageObject ID列表
    std::set<uint32_t> m_requiredTileIDs;    // map需要的tile ID列表
    bool m_bMapReady;                      // map是否准备好绘制
    bool m_bMapLayersDirty;                // map图层是否需要重新渲染

    // ========== 资源加载观察者系统 ==========
    std::vector<IResourceLoadObserver*> m_loadObservers;
    std::atomic<bool> m_bSpritesLoading;    // sprite是否正在加载
    std::atomic<bool> m_bSpritesLoaded;     // sprite是否加载完成
    bool m_bPendingTileLayerUpdate;         // 是否需要在主线程更新地图图层

    // ImageObject 列表窗口
    bool m_showImageObjectList;
    uint32_t m_selectedImageObjectID;  // 选中的 ImageObject ID
    uint32_t m_pendingPreviewObjectID;  // 待渲染预览的 objectID（0表示无）
    bool m_showPreviewWindow;  // 原始预览窗口显示状态
    bool m_showImagePreviewWindow;  // 纯图片预览窗口显示状态
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_previewBitmap;  // 预览 bitmap (D2D)
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_previewSRV;  // 预览 ShaderResourceView (D3D)
    UINT32 m_previewWidth;
    UINT32 m_previewHeight;
    void ShowImageObjectListWindow();
    void JumpToSelectedObject();
    void RenderCompositePreview(uint32_t spriteID);  // 渲染预览到 bitmap
    void ShowCompositePreviewWindow();  // 原始预览窗口（含信息）
    void ShowImagePreviewWindow();      // 纯图片预览窗口（无边框透明）
    bool SaveSpriteToPNG(uint32_t spriteID, const char* filename);  // 保存 sprite 到 PNG

    // 调试窗口 - 检查 sprite 属性
    bool m_showSpriteDebugWindow;
    uint32_t m_debugSpriteID;  // 要检查的 spriteID
    void ShowSpriteDebugWindow();

    // 框选的 Sector 信息
    struct SelectedSector {
        int x, y;
        uint16_t tileID;
        uint32_t imgID;
        uint32_t objectSpriteID;  // ImageObject 对应的 spriteID
    };
    std::vector<SelectedSector> m_selectedSectors;  // 选中的多个 sector
    int m_selectedSectorIndex;  // 当前选中的 sector 索引（用于预览）
    void ShowSectorInfoWindow();      // 显示 sector 信息窗口
    void ClearSelectedSectors();      // 清除选中的 sector
    void DrawSelectedSectorsHighlight();  // 绘制选中 sector 的高亮矩形

    // 观察者注册/注销
    void RegisterLoadObserver(IResourceLoadObserver* observer);
    void UnregisterLoadObserver(IResourceLoadObserver* observer);

    // 触发观察者回调
    void NotifySpriteLoadComplete(uint32_t spriteID, bool success);
    void NotifyAllSpritesLoadComplete();

    // 异步按需加载（线程函数）
    void AsyncLoadRequiredSprites();

    // 视口控制系统（三套坐标）
    // 1. 格子坐标（逻辑层）：sector位置
    // 2. 像素坐标（绘制层）：地图实际像素位置
    // 3. 视口坐标（显示层）：屏幕显示位置
    int m_ViewportX;           // 视口左上角对应的格子X坐标
    int m_ViewportY;           // 视口左上角对应的格子Y坐标
    int m_PrevViewportX;       // 上一帧视口X坐标（用于检测变化）
    int m_PrevViewportY;       // 上一帧视口Y坐标（用于检测变化）
    float m_PrevZoomLevel;     // 上一帧缩放级别（用于检测变化）
    float m_ZoomLevel;         // 缩放级别（1.0 = 原始大小）
    float m_TileWidth;         // tile像素宽度（默认48）
    float m_TileHeight;        // tile像素高度（默认24）

    // 视口操作函数
    void MoveViewport(int deltaX, int deltaY);  // 按格子移动视口
    void SetViewportPosition(int x, int y);    // 直接设置视口位置（导航窗用）
    void SetZoom(float zoom);                    // 设置缩放级别
    void HandleKeyboardInput();                  // 处理键盘输入

    // ========== 三套坐标转换公式 ==========
    // 1. 格子坐标（逻辑层）：sector 位置 (x, y)
    // 2. 像素坐标（绘制层）：地图实际像素位置
    // 3. 视口坐标（显示层）：屏幕显示位置
    //
    // 转换公式（假设 tile 中心点为锚点）：
    // - 格子 -> 像素：pixelX = sectorX * tileW + tileW/2
    //                    pixelY = sectorY * tileH + tileH/2
    // - 像素 -> 视口：viewportX = (pixelX - viewportPixelX) * zoom
    //                    viewportY = (pixelY - viewportPixelY) * zoom
    // - 视口 -> 像素：pixelX = viewportX / zoom + viewportPixelX
    //                    pixelY = viewportY / zoom + viewportPixelY
    // - 像素 -> 格子：sectorX = (pixelX - tileW/2) / tileW

    // 获取视口左上角对应的像素坐标
    D2D1_POINT_2F GetViewportPixelOffset() const {
        return D2D1::Point2F(
            static_cast<float>(m_ViewportX) * m_TileWidth,
            static_cast<float>(m_ViewportY) * m_TileHeight
        );
    }

    // 格子坐标 -> 像素坐标（中心点）
    D2D1_POINT_2F SectorToPixel(int sectorX, int sectorY) const {
        return D2D1::Point2F(
            static_cast<float>(sectorX) * m_TileWidth + m_TileWidth / 2.0f,
            static_cast<float>(sectorY) * m_TileHeight + m_TileHeight / 2.0f
        );
    }

    // 像素坐标 -> 格子坐标（中心点）
    D2D1_POINT_2F PixelToSector(float pixelX, float pixelY) const {
        return D2D1::Point2F(
            (pixelX - m_TileWidth / 2.0f) / m_TileWidth,
            (pixelY - m_TileHeight / 2.0f) / m_TileHeight
        );
    }

    // 像素坐标 -> 视口坐标
    D2D1_POINT_2F PixelToViewport(float pixelX, float pixelY) const {
        D2D1_POINT_2F offset = GetViewportPixelOffset();
        return D2D1::Point2F(
            (pixelX - offset.x) * m_ZoomLevel,
            (pixelY - offset.y) * m_ZoomLevel
        );
    }

    // 视口坐标 -> 像素坐标
    D2D1_POINT_2F ViewportToPixel(float viewportX, float viewportY) const {
        D2D1_POINT_2F offset = GetViewportPixelOffset();
        return D2D1::Point2F(
            viewportX / m_ZoomLevel + offset.x,
            viewportY / m_ZoomLevel + offset.y
        );
    }

    // 格子坐标 -> 视口坐标
    D2D1_POINT_2F SectorToViewport(int sectorX, int sectorY) const {
        D2D1_POINT_2F pixel = SectorToPixel(sectorX, sectorY);
        return PixelToViewport(pixel.x, pixel.y);
    }

    // 视口坐标 -> 格子坐标
    D2D1_POINT_2F ViewportToSector(float viewportX, float viewportY) const {
        D2D1_POINT_2F pixel = ViewportToPixel(viewportX, viewportY);
        return PixelToSector(pixel.x, pixel.y);
    }

    // 视口移动计时器（减慢移动速度）
    float m_ViewportMoveTimer;     // 移动计时器
    float m_ViewportMoveDelay;     // 移动延迟（秒）

    // 异步加载map文件
    void AsyncLoadMapFile(const std::wstring& filePath);

    // 按需加载map需要的sprite
    void LoadRequiredSpritesForMap();
    void UpdateMapTileLayer();

    // 绘制map层
    void DrawMapLayers();
    void DrawMapLayersToTarget(ID2D1Bitmap1* targetBitmap);

    Texture2DManager m_TextureManager;
    MapLayerManager m_MapLayerManager;  // 地图图层管理器
    CAnimationManager m_animationManager;

    ComPtr<ID2D1Effect> m_pEffect;//特效
    ComPtr<ID2D1Bitmap1> d2dCLOBitmap;
    ComPtr<ID2D1Bitmap1> d2dBitmap;
    ComPtr<ID2D1Bitmap1> d2dSpriteBitmap;
    ComPtr<ID2D1Bitmap1> cpuReadBitmap;
    ComPtr<ID2D1Bitmap1> ShadowBitmap;
    ComPtr<ID3D11ShaderResourceView> m_ShaderResourceView;
    int m_CurrFrame;//当前播放帧
    ComPtr<ID2D1SolidColorBrush> m_pColorBrush;//单色画刷
    ComPtr<ID2D1SolidColorBrush> m_pBlackColorBrush;//单色画刷
    CCreatureFramePack m_CFPK;//角色帧动画数据
    CBaseSpritePackVector* m_pISPK;                 // 游戏精灵库
    CBaseSpritePackVector* m_pSPK;
    int checkNUM=0;
    ShowMode m_CurrMode;
    int m_CurrFramePack;//当前动画集

    //绘图变量区
    D2D1_RECT_F m_sourceRect;
    D2D1_RECT_F m_destRect;
    D2D1_RECT_F m_CFPK_select;
    D2D1_RECT_F m_ShadowRect;
    D2D1_RECT_F m_RoleRect;
    std::map<int, int> m_drawMap;//map是有序的,这里索引也代表着图层顺序.
    D2D1_POINT_2F m_position;
    float m_scale;
    D2D1_POINT_2F m_offset ;
    D2D1_POINT_2F m_ShadowOffset;
    int m_DrawingFrameSizeMin;//动画最小值
    float m_Rotation;//Rotation angle
    float m_Skew_X;//Rotation angle
    float m_Skew_Y;//Rotation angle
    std::map<std::string, int> m_namemap;
    std::set<int> my_selection;
};

#endif