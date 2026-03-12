//----------------------------------------------------------------------
// CFrameSetManager.h
//----------------------------------------------------------------------
//
//创建信息，以便只能在框架包中加载特定框架。
//
// <FrameID를 저장할 List사용>
//
// 保存帧包文件的一些帧 ID。
// 
// * 我们需要创建一个在Frame Set中加载时使用的索引文件，
// 设置保存的Frame ID对应的Frame的文件位置
// 在帧包索引文件中查找
// 它应该保存为框架集索引文件。
//
// *提取与给定帧 ID 相关的所有 Sprite ID。
// 不能重复，所以添加到列表的时候要保证不重复。
// 需要创建Sprite Set Index File，但是设置Sprite Pack Index
// 使用保存
//
//
// (!) Thing Frame Set Manager 和 Creature Frame Pack Manager
// 必须继承和使用
//----------------------------------------------------------------------
//
// 设置帧 ID
// 保存 Frame Pack 的文件位置。 --> 框架集索引文件
// 保存 Sprite Pack 的文件位置。 --> 精灵集索引文件
//
//----------------------------------------------------------------------

#ifndef	__CFRAMESETMANAGER_H__
#define	__CFRAMESETMANAGER_H__


#include "DrawTypeDef.h"
#include "CSetManager.h"


class CFrameSetManager : public CSetManager<TYPE_FRAMEID, TYPE_FRAMEID> {
public:
	CFrameSetManager();
	virtual ~CFrameSetManager();

	//--------------------------------------------------------
	// 从框架包索引文件创建框架集索引文件。
	//--------------------------------------------------------
	bool		SaveFrameSetIndex(ofstream& setIndex, ifstream& packIndex);

	//--------------------------------------------------------
// 通过选择与 Frame Set 关联的 Sprite ID
// 从 Sprite Pack 索引文件创建 Sprite Set 索引文件。
	//--------------------------------------------------------
	//virtual bool		SaveSpriteSetIndex(class ofstream& setIndex, class ifstream& packIndex) = 0;


protected:

};


#endif



