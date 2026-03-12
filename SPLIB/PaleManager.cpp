#include "PaleManager.h"
namespace
{
    // PaleManager单例
    PaleManager* s_pInstance = nullptr;
}


PaleManager::PaleManager()
{
    if (s_pInstance)
        throw std::exception("PaleManager is a singleton!");
    s_pInstance = this;
}

PaleManager::~PaleManager()
{

}

PaleManager& PaleManager::Get()
{
    if (!s_pInstance)
        throw std::exception("PaleManager needs an instance!");
    return *s_pInstance;
}



WORD PaleManager::getcolorapproximate(WORD* Color)
{

    return Color_v_List[*Color];
}

bool PaleManager::addcolorapproximate(WORD* Color1, WORD* Color2)
{
    return Color_v_List.try_emplace(*Color1, *Color2).second;;
}

/// <summary>
/// RGB 调色板转565 且字符串化
/// </summary>
/// <param name="pDataSource">调色版数组指针</param>
void PaleManager::init(MPalette& mpale)
{

    if (mpale.GetSize() > 255) {
        throw std::exception("Palette need less 255");
    }
    for (int i = 0; i < mpale.GetSize(); i++) {


        if (m_PaleMap.try_emplace(mpale[i], i).second) {
            m_indexMap.push_back(mpale[i]);
        }
       

    }
    m_indexMap.push_back(0);
    m_PaleMap.try_emplace(0, 255).second;
}

bool PaleManager::AddPalette(WORD* Color, WORD index)
{


    m_indexMap.push_back(*Color);

    return m_PaleMap.try_emplace(*Color, index).second;
}

void PaleManager::RemovePalette(WORD* Color)
{

    m_PaleMap.erase(*Color);
}

WORD PaleManager::GetPaletteIndex(WORD* Color)
{

    return m_PaleMap[*Color];

}

bool PaleManager::CanGetPalette(WORD& Color)
{
    //如果存在 返回真 颜色在序列表中
    if (m_PaleMap.count(Color)) {
        return true;
    }
    return false;
}

bool PaleManager::CanGetAPXPalette(WORD& Color)
{
    //如果存在 返回真 颜色在序列表中
    if (Color_v_List.count(Color)) {
        return true;
    }
    return false;
}
