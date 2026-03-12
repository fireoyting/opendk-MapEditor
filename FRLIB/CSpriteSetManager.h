//----------------------------------------------------------------------
// CSpriteSetManager.h
//----------------------------------------------------------------------
//
// 创建信息，以便只能从 Sprite Pack 加载特定的 Sprite。
//
// <使用List存储Sprite ID>
//
// 保存一些 Sprite Pack 文件的 Sprite ID。
//
// 我们需要创建一个索引文件，以便在 Sprite Set 中加载时使用，
// 设置与保存的 Sprite ID 对应的 Sprite 的 File Position
// 在 Sprite Pack 索引文件中查找
// 它应该保存为 Sprite Set 索引文件。
//
//----------------------------------------------------------------------
//
// 设置精灵 ID
// 保存文件位置。
//
//----------------------------------------------------------------------

#ifndef	__CSPRITESETMANAGER_H__
#define	__CSPRITESETMANAGER_H__


#include "DrawTypeDef.h"
#include "CSetManager.h"


class CSpriteSetManager : public CSetManager<TYPE_SPRITEID, TYPE_SPRITEID> {
public:
	CSpriteSetManager();
	~CSpriteSetManager();

	//--------------------------------------------------------
	// 从 Sprite Pack 索引文件创建 Sprite Set 索引文件。
	//--------------------------------------------------------
	bool		SaveSpriteSetIndex(ofstream& setIndex,ifstream& spkIndex);


protected:

};


#endif
