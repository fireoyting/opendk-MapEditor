// CThingFrameSetManager.h
//----------------------------------------------------------------------
//
// 创建动画帧集的类
//
//
//----------------------------------------------------------------------

#ifndef	__CANIMATIONFRAMESETMANAGER_H__
#define	__CANIMATIONFRAMESETMANAGER_H__

#include "CFrameSetManager.h"
#include "CFramePack.h"

class CAnimationFrameSetManager : public CFrameSetManager {
public:
	CAnimationFrameSetManager();
	~CAnimationFrameSetManager();

protected:
	//--------------------------------------------------------
// 通过选择与动画帧集相关的 Sprite ID
// 从 Sprite Pack 索引文件创建 Sprite Set 索引文件。
	//--------------------------------------------------------
	bool	SaveSpriteSetIndex(CAnimationFramePack* pAnimationFramePack, ofstream& setIndex, ifstream& packIndex);
};

#endif
