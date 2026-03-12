#include "CPalette.h"

MPalette::MPalette()
{
    m_Count = 0;
    m_pColor = nullptr;
}

MPalette::~MPalette()
{
    Release();

}

void MPalette::operator = (const MPalette& pal)
{
    // 皋葛府 秦力
    Release();

    m_Count = pal.m_Count;

    m_pColor = new WORD[m_Count];

    memcpy(m_pColor, pal.m_pColor, m_Count * 2);
}
void MPalette::CopyData(const MPalette& pal)
{
    Release();

    m_Count = pal.m_Count;

    m_pColor = new WORD[m_Count];

    memcpy(m_pColor, pal.m_pColor, m_Count * 2);
}

void MPalette::Release()
{
    if (m_pColor != nullptr)
    {
        delete[]m_pColor;
        m_pColor = nullptr;
        m_Count = 0;
    }
}

bool MPalette::LoadFromManager(WORD* Source, WORD size) {

    Release();

    m_Count = size;
    m_pColor = new WORD[m_Count];
    memcpy(m_pColor, Source, m_Count << 1);

    return true;
}
bool MPalette::LoadFromFile(ifstream& file)
{

    if (m_pColor != nullptr)
    {
        delete[]m_pColor;
        m_pColor = nullptr;
        m_Count = 0;
    }

    file.read((char*)&m_Count, 1);

    if (m_Count != 0)
    {
        m_pColor = new WORD[m_Count];
    }

    for (int i = 0; i < m_Count; i++)
    {
        file.read((char*)&m_pColor[i], 2);
    }

    return true;
}

bool MPalette::SaveToFile(ofstream& file)
{

    file.write((const char*)&m_Count, 1);

    for (int i = 0; i < m_Count; i++)
    {
        file.write((const char*)&m_pColor[i], 2);
    }

    return true;

}
bool MPalette::SaveToACTFile(ofstream& file)
{
    BYTE p_r;
    BYTE p_g;
    BYTE p_b;
    BYTE zreo[772]{};
    for (int i = 0; i < m_Count; i++)
    {
        p_r = (m_pColor[i] & RGB565_MASK_RED) >> 8;//
        p_g = (m_pColor[i] & RGB565_MASK_GREEN) >> 3;
        p_b = (m_pColor[i] & RGB565_MASK_BLUE) << 3;


        file.write((const char*)&p_r, 1);
        file.write((const char*)&p_g, 1);
        file.write((const char*)&p_b, 1);
    }
    int nums = 769 - m_Count * 3;
    file.write((const char*)&zreo, nums);
    file.write((const char*)&m_Count, 1);
    file.write((const char*)&zreo, 2);

    return true;

}
/// <summary>
/// 读取ACT文件
/// </summary>
/// <param name="file"></param>
/// <returns>TRUE</returns>
bool MPalette::LoadFromACTFile(ifstream& file)
{
    BYTE p_r;
    BYTE p_g;
    BYTE p_b;
    UINT16 ColorTemp;
    BYTE* ACT_data;
    UINT16 tempcolor;
    UINT64 fsize, fend;
    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fend = file.tellg();
    fsize = fend - fsize;
    file.seekg(0, std::ios::beg);

    if (m_pColor != nullptr)
    {
        delete[]m_pColor;
        m_pColor = nullptr;
        m_Count = 0;
    }
    ACT_data = new BYTE[772];
    if (fsize > 768) {
        file.read((char*)ACT_data, 772);
        m_Count = ACT_data[769];
    }
    else {
        file.read((char*)ACT_data, 768);
        m_Count = 255;
    }
    //act分成2种格式， 导出是768 还有就是后面不满足256色的772格式


    if (m_Count != 0)
    {
        m_pColor = new WORD[m_Count];
    }
    for (int i = 0; i < m_Count; i++)
    {
        p_r = ACT_data[3 * i];
        p_g = ACT_data[3 * i + 1];
        p_b = ACT_data[3 * i + 2];

        tempcolor = ((p_r >> 3) << 11) | ((p_g >> 2) << 5) | (p_b >> 3);

        m_pColor[i] = tempcolor;

    }

    delete[]ACT_data;

    return true;
}

