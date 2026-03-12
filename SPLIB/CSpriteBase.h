#ifndef BASE_SPRITE
#define BASE_SPRITE
#include "CSpritePalBase.h"
#include "DrawTypeDef.H"
/*
这个是SPK，LIB中最基础的精灵类， 只保存了宽高和像素数据
*/
//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------






class CBaseSprite : public CSpritePalBase
{
public:

    void Release() override;
    bool LoadFromFile(ifstream& file) override;// 
    bool GetPalette(WORD& Color, BYTE& colorindex) override;// 获取调色板. 在BASESPRITE里面我们不使用
    bool SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height) override;//转换图片为压缩格式 ALPHA通道必须重写此方法
    bool SaveToFile(ofstream& file) override;
    //static	void	SetColorSet();// 预生成颜色表
    //static	void	GetIndexColor(WORD* pColor, int step, int r0, int g0, int b0, int r1, int g1, int b1);//获取两个颜色之间的插值---16色情况
    //static	BYTE	GetColorToGradation(BYTE color);
    bool GetSpriteSizeFormFile(ifstream& file);
    bool SetImage();
    uint32_t* GetImage();
    void Release_Image();

public:

protected:
    
    vector<uint32_t> m_textureMapdate;
    UINT* m_indexArray_image; // alpha精灵的索引只用了uint_16的长度，遇上大型图片无法保存正确的偏移值，
                            // alpha精灵有其它配置文件辅助可以自动拼接组合
                            // 所以image新建了一个uint_32的索引数组，
                            // 以便保存正确的偏移值
};


#endif // !BASE_SPRITE





