//----------------------------------------------------------------------
// CFrameSetManager.cpp
//----------------------------------------------------------------------
#include "FR_PCH.h"
#include "CFrameSetManager.h"
//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------

CFrameSetManager::CFrameSetManager()
{
}

CFrameSetManager::~CFrameSetManager()
{
}

//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Save FrameSet Index
//----------------------------------------------------------------------
// 从框架包索引文件创建框架集索引文件。 说实话我还不明白为什么要这样do
// 有什么区别吗, 看保存的数据是一模一样的
//----------------------------------------------------------------------
bool
CFrameSetManager::SaveFrameSetIndex(ofstream& setIndex, ifstream& packIndex)
{
	// m_List에 아무것도 없으면..
	if (m_List.size() == 0)
		return false;


	TYPE_FRAMEID	count;

	//---------------------------------------------------------------
	//读取帧包索引文件中的FRAME数。
	//---------------------------------------------------------------
	packIndex.read((char*)&count, SIZE_FRAMEID);

	// FramePack Index를 저장해둘 memory잡기
	long* pIndex = new long[count];

	//---------------------------------------------------------------
	// 加载所有帧包索引。
	//---------------------------------------------------------------
	for (TYPE_FRAMEID i = 0; i < count; i++)
	{
		packIndex.read((char*)&pIndex[i], 4);
	}

	//---------------------------------------------------------------
// 在 Frame Pack Index 中按 m 顺序排列
// 读取并保存对应Frame ID的File Position。
// 建立了 按顺序排序的 1: 0XXXX
//					   2: 0C222
// 	   
// 	的一个简易索引, 然后根据对应指针去读取数据
// 	   最后生成类似这样的           动画包a : 233 : 0xxx
//											  234 : 0xxa
//										      235 : 0xxb
	//---------------------------------------------------------------
	DATA_LIST::iterator iData = m_List.begin();

	// FrameSet의 Frame개수 저장
	count = m_List.size();
	setIndex.write((const char*)&count, SIZE_FRAMEID);

	// List의 모든 node에 대해서..
	while (iData != m_List.end())
	{
		// Frame ID에 대한 FramePack File에서의 File Position
		setIndex.write((const char*)&pIndex[(*iData)], 4);

		iData++;
	}

	delete[] pIndex;

	return true;
}

