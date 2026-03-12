#ifndef SP_PAL_BASE
#define SP_PAL_BASE

#include<windows.h>
#include "SP_PCH.h"
/// <summary>
/// 使用调色板压缩的精灵类基类
/// 源码在VS2022编译会报错
/// 改了下指针赋值
/// 
/// ISPK的精灵类保存时候 2进制存在一个声明区以便调用客户端自定义4xx个颜色,
/// 用于装备颜色外观的快捷变换.
/// 保存时 声明自定义颜色区即可 用纯色声明色键
/// 在编写工具时可以导入客户端的自定义颜色表, 并且切换颜色.
/// </summary>
class CSpritePalBase
{
public:
    CSpritePalBase();
    virtual ~CSpritePalBase();
    virtual void        Release();

    virtual bool LoadFromFile(ifstream& file);// ALPHA 和 无ALPHA通道类 精灵不同， 可以指定一个颜色为透明色  set color key //可以继承重写此类
    virtual bool GetPalette(WORD& Color, BYTE& colorindex);
    virtual bool SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height);//转换图片为压缩格式 ALPHA通道必须重写此方法
    virtual bool SaveToFile(ofstream& file);

    BOOL        Ready() const { return m_bCreate; }
    WORD        GetWidth()      const { return m_Width; }
    WORD        GetHeight()     const { return m_Height; }
    BYTE* GetPixelLine(WORD y)  const { return m_Pixels[y]; }//改为使用BYTE。 然后前向声明老是报错 就不用了


protected:
    WORD            m_Width;
    WORD            m_Height;
    BYTE* m_pData;
    BYTE** m_Pixels;
    unsigned int m_BodyLength;
    bool            m_bInit;
    bool            m_bCreate=false;
    WORD* m_indexArray;
};

#endif