#include "CSpritePalBase.h"
#include "PaleManager.h"
#include <map>
using std::map;
using std::unordered_map;


CSpritePalBase::CSpritePalBase()
{
    m_BodyLength = 0;
    m_Width = 0;
    m_Height = 0;
    m_pData = nullptr;
    m_Pixels = nullptr;
    m_indexArray = nullptr;
    m_bInit = false;
}

CSpritePalBase::~CSpritePalBase()
{
    Release();
}

void CSpritePalBase::Release()
{
    m_Width = 0;
    m_Height = 0;
    m_BodyLength = 0;

    if (m_pData != nullptr)
    {
        delete[] m_pData;
        m_pData = nullptr;
        delete[] m_Pixels;
        m_Pixels = nullptr;
        delete[]m_indexArray;
        m_indexArray = nullptr;

    }

    m_bInit = false;

}

bool CSpritePalBase::LoadFromFile(ifstream& file)
{

    Release();
    file.read((char*)&m_BodyLength, 4);
    file.read((char*)&m_Width, 2);
    file.read((char*)&m_Height, 2);
    m_pData = new BYTE[m_BodyLength];

    file.read((char*)m_pData, m_BodyLength);

    m_Pixels = new BYTE * [m_Height];

    m_indexArray = new WORD[m_Height];

    file.read((char*)m_indexArray, m_Height << 1);

    BYTE* tempData = m_pData;

    for (int i = 0; i < m_Height; i++)
    {
        m_Pixels[i] = tempData;
        tempData += m_indexArray[i];
    }





    m_bInit = true;
    return true;

}
bool CSpritePalBase::GetPalette(WORD& Color, BYTE& colorindex)
{
    PaleManager& m_pale = PaleManager::Get();
    BYTE p_r;
    BYTE p_g;
    BYTE p_b;
    BYTE x_r;
    BYTE x_g;
    BYTE x_b;
    WORD temp_color;
    WORD range = 0;
    map<WORD, WORD> ColorList;

    x_r = (Color & RGB565_MASK_RED) >> 8;
    x_g = (Color & RGB565_MASK_GREEN) >> 3;
    x_b = (Color & RGB565_MASK_BLUE) << 3;
    int index;

    index = m_pale.size();

    if (m_pale.CanGetAPXPalette(Color)) {
        Color = m_pale.getcolorapproximate(&Color);
        colorindex = m_pale.GetPaletteIndex(&Color);
        return true;
    }

    if (!m_pale.CanGetPalette(Color)) {
        if (index < 255) {
            m_pale.AddPalette(&Color, index++);
            colorindex = index;
            return true;

        }
        else
        {


            for (int k = 0; k < index; k++) {
                p_r = x_r - ((m_pale[k] & RGB565_MASK_RED) >> 8);
                p_g = x_g - ((m_pale[k] & RGB565_MASK_GREEN) >> 3);
                p_b = x_b - ((m_pale[k] & RGB565_MASK_BLUE) << 3);
                range = sqrt(pow(p_r, 2) + pow(p_g, 2) + pow(p_b, 2));//拟合接近黑色时,排序会失败吗?

                WORD shit_color = m_pale[k];
                ColorList.insert(std::pair<WORD, WORD>(range, shit_color));
            }

            temp_color = Color;
            Color = ColorList.begin()->second;
            m_pale.addcolorapproximate(&temp_color, &Color);
            ColorList.clear();
            colorindex = m_pale.GetPaletteIndex(&Color);
            return true;

        }
    }


    if (m_pale.CanGetPalette(Color)) {
        colorindex = m_pale.GetPaletteIndex(&Color);
        return true;
    }
    return false;
}










bool CSpritePalBase::SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height)
{

    Release();

    m_Width = width;
    m_Height = height;

    int temp_bodysize = 0;
    int Segmentcount = 0;
    int offset = 0;
    int colorcount = 0;
    int y = 0;
    int x = 0;
    int temp_point = 0;
    BYTE s_r = 0, s_b = 0, s_g = 0, s_a = 0;
    BYTE temp_data[4024]{};
    BYTE temp_data_0[4024]{};//申请stack内存 方便复写
    m_Pixels = new BYTE * [height];
    BYTE** Pixels = new BYTE * [height];
    BYTE* tempSource;
    m_indexArray = new WORD[height];

    for (y = 0; y < m_Height; y++)
    {
        Segmentcount = 0;

        memcpy(temp_data, temp_data_0, 4024);
        tempSource = pSource;
        temp_point = 1;

        for (x = 0; x < m_Width;)
        {
            offset = 0;
            colorcount = 0;//这个指针飘的我头很晕. 调试下应该是可以了. 怎么优化我还没想好...
            while (*(pSource + 3 + (x << 2)) == 0)
            {
                if (x >= m_Width|| offset>=255) {//透明色大于255 分段
                    break;
                }
                offset++;
                x++;
            }

            if (x >= m_Width) {
                break;
            }
            temp_data[temp_point++] = offset;
            Segmentcount++;
            tempSource = pSource + (x << 2);
            while (*(pSource + 3 + (x << 2)) != 0)
            {
                if (x >= m_Width|| colorcount >=255) {
                    break;
                }
                colorcount++;
                x++;
            }
            BYTE temp_color_index;
            temp_data[temp_point++] = colorcount;
            if (colorcount > 0)
            {
                for (int t = 0; t < colorcount; t++)
                {

                    WORD temp_color;
                    temp_color_index = 25;

                    s_a = (*(tempSource + 3 + (t << 2))) >> 3;
                    temp_data[temp_point++] = s_a;

                    s_r = *(tempSource + 0 + (t << 2));
                    s_g = *(tempSource + 1 + (t << 2));
                    s_b = *(tempSource + 2 + (t << 2));

                    temp_color = ((s_r >> 3) << 11) | ((s_g >> 2) << 5) | (s_b >> 3);
                    if (!GetPalette(temp_color, temp_color_index)) {
                        throw std::exception("can't set PaletteIndex -256");
                    }
                    temp_data[temp_point++] = temp_color_index;
                }
                tempSource += colorcount << 2;

            }
        }

        pSource += pitch;
        m_BodyLength += (temp_point);

        Pixels[y] = new BYTE[temp_point];


        temp_data[0] = Segmentcount;
        memcpy(Pixels[y], temp_data, temp_point);
        m_indexArray[y] = temp_point;
    }

    m_pData = new BYTE[m_BodyLength];
    BYTE* F_tempData = m_pData;

    for (x = 0; x < m_Height; x++)
    {
        memcpy(F_tempData, Pixels[x], m_indexArray[x]);
        m_Pixels[x] = F_tempData;
        F_tempData += m_indexArray[x];
        delete[] Pixels[x];

    }



    delete[] Pixels;

    return true;
}

bool CSpritePalBase::SaveToFile(ofstream& file)
{

    file.write((char*)&m_BodyLength, 4);
    file.write((char*)&m_Width, 2);
    file.write((char*)&m_Height, 2);
    file.write((char*)m_pData, m_BodyLength);
    file.write((char*)m_indexArray, m_Height << 1);
    return true;
}

