
#ifndef	__DRAWTYPEDEF_H__
#define	__DRAWTYPEDEF_H__
#include <minwindef.h>
#include <cstdint>
// sizeof( TYPE_... )
constexpr int SIZE_SPRITEID = 2;
constexpr	int SIZE_FRAMEID = 2;
constexpr	int SIZE_FILTERID = 2;

constexpr	unsigned int SPRITEID_NULL = 0xFFFF;
constexpr	unsigned int FRAMEID_NULL = 0xFFFF;
constexpr	unsigned int FILTERID_NULL = 0xFFFF;


typedef	unsigned short		TYPE_SPRITEID;
typedef unsigned short		TYPE_FRAMEID;
typedef unsigned short		TYPE_FILTERID;
constexpr auto MAX_COLORSET_SEED = 33;	// 默认颜色的数量；
constexpr auto MAX_COLORSET_SEED_MODIFY = 15;		// 每个默认颜色的变形颜色数;
constexpr auto MAX_COLORSET = 495;	 // MAX_COLORSET_SEED * MAX_COLORSET_SEED_MODIFY;
constexpr auto MAX_COLORGRADATION = 30;
constexpr auto MAX_COLOR_TO_GRADATION = 93;		// R+G+B;
constexpr auto MAX_COLORGRADATION_HALF = 15;		// == MAX_COLORSET_SEED_MODIFY;
constexpr auto MAX_DARKBIT = 5;
constexpr unsigned int RGB565_MASK_RED = 0xF800  ;
constexpr unsigned int RGB565_MASK_GREEN = 0x07E0  ;
constexpr unsigned int RGB565_MASK_BLUE = 0x001F ;
enum BLT_TYPE 
{
	BLT_NORMAL = 0,//普通
	BLT_EFFECT,//魔法效果
	BLT_SHADOW,//影子
	BLT_SCREEN// 屏保
};
constexpr auto s_bSHIFT_R = 11;
constexpr auto s_bSHIFT_G = 6;
constexpr auto s_bSHIFT_B = 0;
static WORD	Color(const BYTE& r, const BYTE& g, const BYTE& b)
{
	return (r << s_bSHIFT_R) | (g << s_bSHIFT_G) | b;
}

// Red蔼
static  BYTE Red(const WORD& c)
{
	return (c >> s_bSHIFT_R);
}


static  BYTE Green(const WORD& c)
{


	BYTE g;
	g = c >> s_bSHIFT_G;
	g &= RGB565_MASK_BLUE;
	return g;
}

static  BYTE	Blue(const WORD& c)
{
	return (c & RGB565_MASK_BLUE);
}
static uint32_t ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (r | (g << 8) | (b << 16) | (a << 24));
}
#endif