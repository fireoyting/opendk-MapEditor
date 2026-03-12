//----------------------------------------------------------------------
// CCreatureFrameSetManager.h
//----------------------------------------------------------------------
//
// CreatureFrameSet을 생성하는 class
//
//
//----------------------------------------------------------------------

#ifndef	__CCREATUREFRAMESETMANAGER_H__
#define	__CCREATUREFRAMESETMANAGER_H__

#include "CFrameSetManager.h"
#include "CFramePack.h"

class CCreatureFrameSetManager : public CFrameSetManager {
public:
	CCreatureFrameSetManager();
	~CCreatureFrameSetManager();

protected:
	//--------------------------------------------------------
// 通过选择与 Creature Frame Set 关联的 Sprite ID
// 从 Sprite Pack 索引文件创建 Sprite Set 索引文件。

	//--------------------------------------------------------
	bool	SaveSpriteSetIndex(CCreatureFramePack* pCreatureFramePack, ofstream& setIndex, ifstream& packIndex);
};

#endif

