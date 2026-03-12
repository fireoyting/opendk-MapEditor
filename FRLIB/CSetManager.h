//----------------------------------------------------------------------
// CSetManager.h
//------------------------------------------------ ----------------------
// 模板排序列表
// 在内部，使用了 stl 的LIST
//------------------------------------------------ ----------------------
//
// 仅保存数据类型值。
// 这是一个升序排序。
//
//----------------------------------------------------------------------
#ifndef	__CSETMANAGER_H__
#define	__CSETMANAGER_H__



#include <Windows.h>
#include <list>
#include "DrawTypeDef.h"

#include <fstream>
using std::ifstream;
using std::ofstream;

template <class DataType, class SizeType>
class CSetManager {
	public :		
		typedef std::list<DataType>	DATA_LIST;

	public :
		CSetManager();
		~CSetManager();

		//--------------------------------------------------------
		// Init/Release		
		//--------------------------------------------------------
		void		Release();

		//--------------------------------------------------------
		// add / remove
		//--------------------------------------------------------
		bool		Add(const DataType data);		
		bool		Remove(const DataType data);

		//--------------------------------------------------------
		// file I/O		
		//--------------------------------------------------------		
		bool		SaveToFile(ofstream& file);
		bool		LoadFromFile(ifstream& file);

		//--------------------------------------------------------
		// Get functions
		//--------------------------------------------------------
		SizeType	GetSize() const	{ return m_List.size(); }
		
		// 它通过第一个位置的列表迭代器。
		typename DATA_LIST::const_iterator	GetIterator() const	{ return m_List.begin(); }

	protected :			
		DATA_LIST			m_List;		// Data pointer들을 저장해둔다.

		// sizeof(SizeType) 의 값
		static BYTE			s_SIZEOF_SizeType;
};



//----------------------------------------------------------------------
//
// Initialize static data member
//
//----------------------------------------------------------------------
template <class DataType, class SizeType>
BYTE	CSetManager<DataType, SizeType>::s_SIZEOF_SizeType = sizeof(SizeType);


//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------
template <class DataType, class SizeType> 
CSetManager<DataType, SizeType>::CSetManager()
{
}

template <class DataType, class SizeType> 
CSetManager<DataType, SizeType>::~CSetManager()
{
	Release();
}


//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Release
//----------------------------------------------------------------------
template <class DataType, class SizeType>
void
CSetManager<DataType, SizeType>::Release()
{
	m_List.clear();		
}

//----------------------------------------------------------------------
// Add
//------------------------------------------------ ----------------------
// 将数据添加到列表中。
// 排序后添加，不允许重复。
//
// 如果值已经存在，则返回 false。
//----------------------------------------------------------------------
template <class DataType, class SizeType>
bool	
CSetManager<DataType, SizeType>::Add(const DataType data)
{
	DATA_LIST::iterator iData = m_List.begin();

	while (iData != m_List.end())
	{		
// 如果当前的大于你要添加的，
// 只需将其添加到当前位置。
		if (*iData > data)
		{
			m_List.insert(iData, data);
			return true;
		}
// 如果值已经存在
// 不添加
		if (*iData==data)
		{
			return false;
		}

		iData++;
	}	
// 因为它大于列表中的所有元素
// 追加到列表的末尾
	m_List.push_back( data );

	return true;
}

//----------------------------------------------------------------------
// Remove
//----------------------------------------------------------------------
// 从列表中删除数据。
//
// 如果值不存在，返回false
//----------------------------------------------------------------------
template <class DataType, class SizeType>
bool
CSetManager<DataType, SizeType>::Remove(const DataType data)
{
	DATA_LIST::iterator iData = m_List.begin();

	while (iData != m_List.end())
	{		
		// 같은 값이면 지운다.
		if (*iData==data)
		{
			m_List.erase(iData);
			return true;
		}
// 如果当前位置的值大于数据
// 没有数据值，因为只有大值。
		if (*iData > data)
		{			
			return false;
		}

		iData++;
	}	

	// 없는 경우
	return false;
}


//----------------------------------------------------------------------
// Save To File
//----------------------------------------------------------------------
//
// 保存大小
// 保存所有列表节点。
//
//----------------------------------------------------------------------
template <class DataType, class SizeType>
bool
CSetManager<DataType, SizeType>::SaveToFile(ofstream& file)
{
	// size
	SizeType size = m_List.size();

	// size저장
	file.write((const char *)&size, s_SIZEOF_SizeType);

	// 아무 것도 없으면
	if (size==0)
	{
		return false;
	}

	DataType data;

	// 모든 Data들을 save한다.
	DATA_LIST::iterator iData = m_List.begin();

	int dataSize = sizeof(DataType);

	for (SizeType i=0; i<size; i++)
	{
		data = *iData;

		// file에 저장
		file.write((const char *)&data, dataSize);		

		iData++;
	}
	
	return true;
}

//----------------------------------------------------------------------
// Load from File
//----------------------------------------------------------------------
template <class DataType, class SizeType>
bool
CSetManager<DataType, SizeType>::LoadFromFile(ifstream& file)
{
	// 이전에 있던 list를 지운다.
	Release();

	SizeType size;

// 从文件中读取大小
	file.read((char*)&size, s_SIZEOF_SizeType);

// 如果没有保存
	if (size==0)
	{
		return false;
	}

	DataType	data;

	int dataSize = sizeof(DataType);

	// size개 만큼을 load한다.
	for (SizeType i=0; i<size; i++)
	{
		// file에서 load한다.
		file.read((char*)&data, dataSize);

		// list에 추가한다.
		Add( data );
	}
	
	return true;
}


#endif

