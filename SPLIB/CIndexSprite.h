#ifndef INDEX_SPRITE
#define INDEX_SPRITE
#include "CSpritePalBase.h"
#include "DrawTypeDef.H"
/*
这里说明下结构, indexsprite 本身有一个[495][30]的颜色表
存图片时, 只用了8bit来存颜色索引 实际上8bit都多了,因为大部分都是通过装备来指定变化颜色, 图片本身只存入0 或者1  , 由装备属性写入0-495的数值, 第二个8bit 存颜色的层次

*/
//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
// Maximum value설정


// 一个Sprite使用的最大颜色集数
#define	MAX_COLORSET_USE			256

// 设置index值
#define	INDEX_NULL					0xFF
#define	INDEX_SELECT				0xFE
#define	INDEX_TRANS					0xFD
#define	INDEXSET_NULL				0xFF



class CIndexSprite : public CSpritePalBase
{
public:

    void Release() override;
    bool LoadFromFile(ifstream& file) override;// 
    bool GetPalette(WORD& Color, BYTE& colorindex) override;// 获取调色板. 在INDEXSPRITE里面我们不使用
    bool SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height) override;//转换图片为压缩格式 ALPHA通道必须重写此方法
    bool SaveToFile(ofstream& file) override;
    static	void	SetColorSet();// 预生成颜色表
    static	void	GetIndexColor(WORD* pColor, int step, int r0, int g0, int b0, int r1, int g1, int b1);//获取两个颜色之间的插值---16色情况
    static	BYTE	GetColorToGradation(BYTE color);
    bool GetSpriteSizeFormFile(ifstream& file);
    bool SetImage();
    uint32_t* GetImage();
    void Release_Image();

public:
	//--------------------------------------------------------
// ColorSet Table
//--------------------------------------------------------
// 495 Set ,  30 Gradation
	static USHORT		ColorSet[MAX_COLORSET][MAX_COLORGRADATION];			// 实际颜色
	static USHORT		GradationValue[MAX_COLORGRADATION];					// 每个gradation的值-用于在Index Editor中计算的值
	static USHORT		ColorSetDarkness[MAX_DARKBIT][MAX_COLORSET][MAX_COLORGRADATION];	// 变暗时的颜色值
	static BYTE		ColorToGradation[MAX_COLOR_TO_GRADATION];			//根据R+G+B的值得出Gradation值。
protected:
    static WORD		s_Colorkey;

    // Blt Value (parameter 대용)
    static int		s_IndexValue[MAX_COLORSET_USE];
    vector<uint32_t> m_textureMapdate;
    UINT* m_indexArray_image; // alpha精灵的索引只用了uint_16的长度，遇上大型图片无法保存正确的偏移值，
                            // 所以新建了一个uint_32的索引数组，以便保存正确的偏移值

};


#endif // !INDEX_SPRITE





