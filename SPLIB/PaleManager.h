#ifndef __MPALEMANAGER__
#define __MPALEMANAGER__


#include <unordered_map>
#include <string>
#include <wrl/client.h>
#include "CPalette.h"
class PaleManager 
{
public:
    /// <summary>
    /// C11还没吃透特性,  C23都快出来了.调色板管理是个单例模式
    /// </summary>
    PaleManager();
    ~PaleManager();

    PaleManager(PaleManager&) = delete;
    PaleManager& operator=(const PaleManager&) = delete;
    PaleManager(PaleManager&&) = default;
    PaleManager& operator=(PaleManager&&) = default;

    static PaleManager& Get();

    WORD operator [] (unsigned int n) { return m_indexMap[n]; }
    WORD operator [] (unsigned int n) const { return m_indexMap[n]; }
    WORD getcolorapproximate(WORD* Color);
    bool addcolorapproximate(WORD* Color1, WORD* Color2);
    void init(MPalette& mpale);
    bool AddPalette(WORD* Color, WORD index);
    void RemovePalette(WORD* Color);
    WORD GetPaletteIndex(WORD* Color);
    bool CanGetPalette(WORD& Color);
    bool CanGetAPXPalette(WORD& Color);
    int  size() { return m_indexMap.size(); }
    WORD* GetPoint() { return &m_indexMap[0]; }
private:
    std::unordered_map<WORD, WORD> m_PaleMap;//调色板管理器 用于颜色序号整合
    std::unordered_map<WORD, WORD> Color_v_List;
public:
    std::vector<WORD> m_indexMap;//用于最后按顺序存入调色板

};



using XID = size_t;
inline XID StringToID(std::string str)
{
    static std::hash<std::string> hash;
    return hash(str);
}




























#endif // !__MPALEMANAGER__
