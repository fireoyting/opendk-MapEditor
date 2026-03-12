#include "CSpriteBase.h"




void CBaseSprite::Release()
{

    m_Width = 0;
    m_Height = 0;
    m_BodyLength = 0;

    if (m_pData != nullptr)
    {
        
        delete[] m_Pixels;
        m_Pixels = nullptr;
        delete[] m_indexArray_image;
        m_indexArray_image = nullptr;
		vector<uint32_t>().swap(m_textureMapdate);

    }
	m_bCreate = false;
    m_bInit = false;

}

bool CBaseSprite::LoadFromFile(ifstream& file)
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

bool CBaseSprite::GetPalette(WORD& Color, BYTE& colorindex)
{
    return false;
}

bool CBaseSprite::SetPixels(BYTE* pSource, WORD pitch, WORD width, WORD height)
{
    return false;
}

bool CBaseSprite::SaveToFile(ofstream& file)
{
    return false;
}

//void CBaseSprite::SetColorSet()
//{
//
//}
//
//void CBaseSprite::GetIndexColor(WORD* pColor, int step, int r0, int g0, int b0, int r1, int g1, int b1)
//{
//	
//
//}

//BYTE CBaseSprite::GetColorToGradation(BYTE color)
//{
//	
//}

bool CBaseSprite::GetSpriteSizeFormFile(ifstream& file)
{
	uint32_t len = 0;
	uint32_t temppoint = 0;
	m_BodyLength = 0;

	// 调试输出
	


	for (int i = 0; i < m_Height; i++)
	{
        m_indexArray_image[i] = temppoint;
		file.read((char*)&len, 2);
		temppoint += 2;

		temppoint += (uint32_t)(len << 1);
		m_BodyLength += (uint32_t)(len << 1);

		file.seekg(len << 1, ifstream::cur);
	}
	m_BodyLength += (uint32_t)(m_Height << 1);
	return true;
}
bool CBaseSprite::SetImage()
{
	Release_Image();
	if (m_bInit)
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
				WORD* DataPoint = reinterpret_cast<WORD*> (m_Pixels[i]);//
				WORD lineLength = *DataPoint++;//本行像素数据长度（单位为WORD，0表示本行无像素数据）
				if (lineLength == 0) {
					// 本行长度为0，没有像素数据，跳过此行
					continue;
				}
				offset = 0;//继续
				Segmentcount = *DataPoint++;
                if (Segmentcount> m_Width) {
					// Invalid segment count, skip this line
					m_bCreate = false;
					return false;
                }
				for (int Segment = 0; Segment < Segmentcount; Segment++)
				{
					UINT next_offect, _ckpixcount;
					next_offect = *DataPoint++;
					colorcount = *DataPoint++;
                  
					offset += next_offect;
					
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
uint32_t* CBaseSprite::GetImage()
{
	
	
	return m_textureMapdate.data();

	
}

void CBaseSprite::Release_Image()
{
	m_textureMapdate.clear();
}
