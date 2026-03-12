//----------------------------------------------------------------------
// CFileIndexTable.h
//----------------------------------------------------------------------
//
// Index File을 Load한다.
// 该类专门读取以4个字节存放的文件指针二进制文件, 实现了一个二进制文件打包的快速索引.
//----------------------------------------------------------------------

#ifndef	__CFILEINDEXTABLE_H__
#define	__CFILEINDEXTABLE_H__

#include <Windows.h>
#include <fstream>
using std::ifstream;


class CFileIndexTable {
public:
	CFileIndexTable();
	~CFileIndexTable();

	//--------------------------------------------------------
	// file I/O		
	//--------------------------------------------------------		
	bool			LoadFromFile(ifstream& indexFile);

	WORD				GetSize() { return m_Size; }

	//--------------------------------------------------------
	// operator //重载操作符号, CFIT[]对象会返回m_pindex[]
	//--------------------------------------------------------
	const long& operator [] (WORD n) { return m_pIndex[n]; }

	//--------------------------------------------------------
	// Release
	//--------------------------------------------------------
	void			Release();


protected:
	//--------------------------------------------------------
	// Init/Release
	//--------------------------------------------------------
	void			Init(WORD count);


protected:
	WORD			m_Size;				// 개수
	long* m_pIndex;			// File position
};

#endif
