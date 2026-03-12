#ifndef GUI_TREE_H
#define GUI_TREE_H
#include "windows.h"
#include <imgui.h>
#include "FR.h"
#include "CAnimationManager.h"
//#include "signal.hpp"


typedef struct FRAME_VECTOR_3U
{
    int spriteID;
    int x;
    int y;

} FRAME_VECTOR_3U;

class ImGuiTree
{
public:
    //这个是单例模式
    ImGuiTree();
    static ImGuiTree& Get();//获取已有的
    static bool HasInstance();
    void Clear();
    void ShowCFPKEditTable(const char* title, ACTION_FRAME_ARRAY& cfpk, bool* p_open = nullptr);
    void ShowObject(const char* prefix, FRAME_ARRAY& cfpk, int uid);
    void ShowObject(const char* prefix, DIRECTION_FRAME_ARRAY& cfpk, int uid);
    void ShowEditgui(int dir, bool& p_open);
    bool isPlay() { return isPlayAnimation; }
    UINT16 getElements() { return ActionElements;}
private:
    bool isPlayAnimation;
    UINT16 ActionElements;
    std::shared_ptr<CAnimation> m_pAAnimation;

};


#endif