#include "CIndexSprite.h"

USHORT	CIndexSprite::ColorSet[MAX_COLORSET][MAX_COLORGRADATION];
USHORT	CIndexSprite::GradationValue[MAX_COLORGRADATION];
USHORT	CIndexSprite::ColorSetDarkness[MAX_DARKBIT][MAX_COLORSET][MAX_COLORGRADATION];
BYTE	CIndexSprite::ColorToGradation[MAX_COLOR_TO_GRADATION];

extern WORD	Color(const BYTE& r, const BYTE& g, const BYTE& b);
extern BYTE Red(const WORD& c);
extern BYTE Green(const WORD& c);
extern BYTE Blue(const WORD& c);
extern uint32_t ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
//----------------------------------------------------------------------
// Static member
//----------------------------------------------------------------------
WORD	CIndexSprite::s_Colorkey = 0;
int		CIndexSprite::s_IndexValue[MAX_COLORSET_USE];

//-----------------------------------------------------------------------------
// 在Blt Color中用作默认颜色的Color Set
//-----------------------------------------------------------------------------
const int defaultGradationColor = 384;
void CIndexSprite::Release()
{

    m_Width = 0;
    m_Height = 0;
    m_BodyLength = 0;

    if (m_pData != nullptr)
    {

        delete[] m_Pixels;
        m_Pixels = nullptr;
        delete[] m_indexArray;
        m_indexArray = nullptr;
        delete[] m_indexArray_image;
        m_indexArray_image = nullptr;
		vector<uint32_t>().swap(m_textureMapdate);

    }
	m_bCreate = false;
    m_bInit = false;

}

bool CIndexSprite::LoadFromFile(ifstream& file)
{
	Release();
	//file.read((char*)&m_BodyLength, 4);
	/*
	IndexSprite 类设计的没有alphaSprite好, 主要是它没有预存每行指针 而是存入每行像素长度,这就不能一次性读取所有像素了.
	这边先读取一边文件长度, 并且保存每行指针. 最后跳回文件最初指针.
	这样子就可以用alphaSprite方法读取了.
	 
	*/
	static unsigned long _offset;
	

	file.read((char*)&m_Width, 2);
	file.read((char*)&m_Height, 2);
	if (m_Width == 0 || m_Height == 0)
	{
		m_bInit = true;

		return true;
	}
	m_Pixels = new BYTE * [m_Height];

	m_indexArray_image = new UINT[m_Height];
	_offset = file.tellg();//
	if (GetSpriteSizeFormFile(file))
	{
		file.seekg(_offset, ifstream::beg);//回到file初始指针.
		m_pData = new BYTE[m_BodyLength];
		file.read((char*)m_pData, m_BodyLength);
		BYTE* tempData = m_pData;

		for (int i = 0; i < m_Height; i++)
		{
			tempData = &m_pData[(m_indexArray_image[i])];
			m_Pixels[i] = tempData;
			
		}

		m_bInit = true;
		SetImage();
		return true;
	};
	return false;
}

bool CIndexSprite::GetPalette(WORD& Color, BYTE& colorindex)
{
    return false;
}

bool CIndexSprite::SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height)
{
    return false;
}

bool CIndexSprite::SaveToFile(ofstream& file)
{
    return false;
}

void CIndexSprite::SetColorSet()
{
	int i, j, k, set;
	WORD color;

	static BYTE rgbPoint[MAX_COLORSET_SEED][3] =
	{
		{ 0, 0, 31 },
		{ 0, 31, 0 },
		{ 31, 0, 0 },
		{ 0, 31, 31 },
		{ 31, 0, 31 },
		{ 31, 31, 0 },

		{ 0, 0, 16 },
		{ 0, 16, 0 },
		{ 16, 0, 0 },
		{ 0, 16, 16 },
		{ 16, 0, 16 },
		{ 16, 16, 0 },

		{ 16, 31, 0 },
		{ 16, 0, 31 },
		{ 31, 16, 0 },
		{ 0, 16, 31 },
		{ 31, 0, 16 },
		{ 0, 31, 16 },

		{ 16, 31, 16 },
		{ 16, 16, 31 },
		{ 31, 16, 16 },

		{ 16, 31, 31 },
		{ 31, 16, 31 },
		{ 31, 31, 16 },

		{ 16, 16, 16 }, // 灰色
		{ 24, 24, 24 }, // 亮灰色
		{ 8, 8, 8 }, // 暗灰色

		{ 30, 24, 18 }, // 肉色
		{ 25, 15, 11 },	// 褐色	
		{ 21, 12, 11 },
		{ 19, 15, 13 }, // 古铜色				

		{ 21, 18, 11 }, // 浅色		

		{ 22, 16, 9 } //肉色		
	};



	//----------------------------------------------------------------------
	// ColorIndex Table 
	//----------------------------------------------------------------------
	set = 0;
	int r, g, b;

	for (i = 0; i < MAX_COLORSET_SEED; i++)
	{
		r = rgbPoint[i][0];
		g = rgbPoint[i][1];
		b = rgbPoint[i][2];

		// MAX_COLORGRADATION_HALF ~ 1
		for (j = MAX_COLORGRADATION_HALF; j >= 1; j--)
		{
			// 첫줄만
			if (j == MAX_COLORGRADATION_HALF)
			{
				GetIndexColor(ColorSet[set], j,
					31, 31, 31,
					r, g, b);
			}
			else
			{
				WORD color = ColorSet[i * MAX_COLORSET_SEED_MODIFY][MAX_COLORGRADATION_HALF - j];
				int r0 = Red(color);
				int g0 = Green(color);
				int b0 = Blue(color);

				GetIndexColor(ColorSet[set], j,
					r0, g0, b0,
					r, g, b);
			}

			GetIndexColor(ColorSet[set] + j, MAX_COLORGRADATION - j,
				r, g, b,
				0, 0, 0);

			set++;
		}
	}

	//----------------------------------------------------------------------
	// GradationValue와
	//----------------------------------------------------------------------
	for (j = 0; j < MAX_COLORGRADATION; j++)
	{
		color = ColorSet[0][j];
		GradationValue[j] = Red(color) +Green(color) + Blue(color);
	}

	//----------------------------------------------------------------------
	// Darkness색을 결정한다.
	//----------------------------------------------------------------------
	for (i = 0; i < MAX_COLORSET; i++)
	{
		for (j = 0; j < MAX_COLORGRADATION; j++)
		{
			color = ColorSet[i][j];
			//GradationValue[j] = CDirectDraw::Red(color) + CDirectDraw::Green(color) + CDirectDraw::Blue(color);

			// Darkness를 위한 색값
			for (k = 0; k < MAX_DARKBIT; k++)
			{
				r = ((color >> s_bSHIFT_R) >> k) << s_bSHIFT_R;
				g = (((color >> s_bSHIFT_G) & 0x1F) >> k) << s_bSHIFT_G;
				b = (color & 0x1F) >> k;



				ColorSetDarkness[k][i][j] = r | g | b;
			}
		}
	}

	//----------------------------------------------------------------------
	// Color to Gradation
	//----------------------------------------------------------------------
	for (BYTE cg = 0; cg < MAX_COLOR_TO_GRADATION; cg++)
	{
		ColorToGradation[cg] = GetColorToGradation(cg);
	}
}

void CIndexSprite::GetIndexColor(WORD* pColor, int step, int r0, int g0, int b0, int r1, int g1, int b1)
{
	float r = (float)r0;
	float g = (float)g0;
	float b = (float)b0;

	float step_1 = (float)step - 1.0f;
	float sr = (r1 - r0) / (float)step_1;
	float sg = (g1 - g0) / (float)step_1;
	float sb = (b1 - b0) / (float)step_1;

	BYTE red, green, blue;

	for (int i = 0; i < step; i++)
	{
		red = (BYTE)r;
		green = (BYTE)g;
		blue = (BYTE)b;

		*pColor++ = Color(red, green, blue);

		r += sr;
		g += sg;
		b += sb;
	}

}

BYTE CIndexSprite::GetColorToGradation(BYTE color)
{
	// 5:6:5전용 code
	WORD spriteGradation = (color >> 11) + ((color >> 6) & 0x1F) + (color & 0x1F);
	

	//-------------------------------------------------------
	// spriteGradation값과 가장 가까운 
	// GradationValue를 찾아야 한다.
	//-------------------------------------------------------
	int g = 0;
	for (g = 0; g < MAX_COLORGRADATION; g++)
	{
		if (spriteGradation > GradationValue[g])
		{
			break;
		}
	}

	// 제일 끝의 색깔인 경우
	if (g == 0 || g == MAX_COLORGRADATION - 1)
	{
		return g;
	}

	// 가운데 색깔
	WORD value1 = GradationValue[g - 1] - spriteGradation;
	WORD value2 = spriteGradation - GradationValue[g - 1];

	// 적은 값을 선택한다.
	if (value1 < value2)
	{
		return g - 1;
	}
	else if (value1 > value2)
	{
		return g;
	}

	// 같은 경우는.. ??	
	return g - 1;
	

}

bool CIndexSprite::GetSpriteSizeFormFile(ifstream& file)
{
	UINT len = 0;
	UINT temppoint = 0;
	m_BodyLength = 0;

	for (int i = 0; i < m_Height; i++)
	{
		m_indexArray[i] = temppoint;
		file.read((char*)&len, 2);
		temppoint += 2;

		temppoint += (UINT)(len << 1);
		m_BodyLength += (UINT)(len << 1);

		file.seekg(len << 1, ifstream::cur);
	}
	m_BodyLength += (UINT)(m_Height << 2);
	return true;
}
bool CIndexSprite::SetImage()
{
	
	if (m_bInit && !m_bCreate)
	{
		uint32_t Transparent = ColorRGBA(0, 0, 0, 0);//Transparent color
		//Returns the expanded image array
		int Segmentcount = 0;
		int offset = 0;
		int colorcount = 0;
		int	colorSet, colorGradation;
		USHORT point_color;
		colorSet = 9;//Directly specify the darkest color for color 0
		BYTE p_r, p_g, p_b, p_a;
		if (m_Height > 0)
		{
			m_textureMapdate.resize(static_cast<size_t>(m_Height) * static_cast<size_t>(m_Width), Transparent);

			vector<uint32_t*> textureMap(m_Height);
			for (int J = 0; J < m_Height; ++J)
			{
				textureMap[J] = &m_textureMapdate[J * m_Width];
			}


			for (int i = 0; i < m_Height; i++)
			{
				WORD* DataPoint = reinterpret_cast<WORD*> (m_Pixels[i]);//和客户端不一样的是,我这里每行pixel头 有2个byte是长度. 要跳过再开始读取
				DataPoint++;//跳过本行长度信息
				offset = 0;//继续
				Segmentcount = *DataPoint++;

				for (int Segment = 0; Segment < Segmentcount; Segment++)
				{
					UINT next_offect, _ckpixcount;
					next_offect = *DataPoint++;
					_ckpixcount = *DataPoint++;
					offset += next_offect;
					for (int step_i = 0; step_i < _ckpixcount; step_i++)
					{

						colorGradation = (*DataPoint & 0xFF);

						point_color = ColorSet[colorSet][colorGradation];
						p_r = (point_color & RGB565_MASK_RED) >> 8;//获得555 颜色值
						p_g = (point_color & RGB565_MASK_GREEN) >> 3;
						p_b = (point_color & RGB565_MASK_BLUE) << 3;
						p_a = 255;//索引颜色 alpha通道全部拉满.

						//The index colors are all in 555 format, ranging from 0 to 31,
						//Project it to 0-255; pikachu 2023/11/14
						p_r = (p_r << 3) | (p_r >> 2);

						p_g = (p_g << 3) | (p_g >> 2);
						p_b = (p_b << 3) | (p_b >> 2);
						textureMap[i][offset] = ColorRGBA(p_r, p_g, p_b, p_a);

						offset += 1;
						DataPoint++;

					}
					colorcount = *DataPoint++;
					for (int step_i = 0; step_i < colorcount; step_i++)
					{



						point_color = *DataPoint++;
						p_r = (point_color >> 11) & 0x1F;
						p_g = (point_color >> 5) & 0x3F;
						p_b = point_color & 0x1F;
						p_a = 255;//索引颜色 alpha通道全部拉满.


						//Project it to 0-255; pikachu 2023/11/14
						p_r = (p_r << 3) | (p_r >> 2);
						p_g = (p_g << 2) | (p_g >> 4);
						p_b = (p_b << 3) | (p_b >> 2);
						textureMap[i][offset] = ColorRGBA(p_r, p_g, p_b, p_a);

						offset += 1;


					}
				}

			}
			m_bCreate = true;
			return true;
		}
		else
		{
			m_bCreate = false;
			return false;
		}
		
	}
	
}
uint32_t* CIndexSprite::GetImage()
{
	
	
	return m_textureMapdate.data();

	
}
