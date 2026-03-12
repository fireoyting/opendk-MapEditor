#include<CSpritePalBase.h>

/// <summary>
//影子
/// </summary>
class CSpriteShadows : CSpritePalBase
{
public:

    virtual void        Release();
    virtual bool LoadFromFile(ifstream& file);// ALPHA 和 无ALPHA通道类 精灵不同， 可以指定一个颜色为透明色  set color key //可以继承重写此类
    virtual bool GetPalette(WORD& Color, BYTE& colorindex);
    virtual bool SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height);//转换图片为压缩格式 ALPHA通道必须重写此方法
    virtual bool SaveToFile(ofstream& file);
   
protected:

};

