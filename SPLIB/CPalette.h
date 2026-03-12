#ifndef __MPALETTE_H__
#define __MPALETTE_H__
#include <windows.h>
#include "SP_PCH.h"

class MPalette 
{
public:
	MPalette();
	~MPalette();
	BYTE	GetSize() const { return m_Count; }
	WORD& operator [] (BYTE n) { return m_pColor[n]; }
	WORD& operator [] (BYTE n) const { return m_pColor[n]; }
	void		operator = (const MPalette& pal);
	void        CopyData(const MPalette& pal);
	void        Release();
	bool LoadFromFile(ifstream& file);
	bool LoadFromManager(WORD* Source, WORD size);
	bool SaveToFile(ofstream& file);
	bool SaveToACTFile(ofstream& file);//为了方便开发;新增一个导出调色板功能,---> 简单放大技能图形后拟合调色板
	bool LoadFromACTFile(ifstream& file);//新增直接读取ACT

protected:
	unsigned short* m_pColor;
	unsigned char m_Count;//the colors count
};








#endif