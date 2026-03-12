//----------------------------------------------------------------------
// CAnimationFrameSetManager.cpp
//----------------------------------------------------------------------
#include "FR_PCH.h"
#include "CFrame.h"
#include "CAnimationFrameSetManager.h"
#include "CFramePack.h"
#include "CSpriteSetManager.h"

//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------

CAnimationFrameSetManager::CAnimationFrameSetManager()
{
}

CAnimationFrameSetManager::~CAnimationFrameSetManager()
{
}

//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// 保存 Sprite 集索引
//------------------------------------------------ ----------------------
// 通过选择与动画帧集相关的 Sprite ID
// 从 Sprite Pack 索引文件创建 Sprite Set 索引文件。
// 
//				如 动画EFF01 ---> ↑ ---> SpriteID01
//										  SpriteID02
//										  SpriteID03
//										 
//          然后从包索引 取出ID对应的文件tepll位置 写入集索引
// 
//				//				如 动画EFF01 ---> ↑ ---> 0XXXXXXXXX
//														  ......
//														  0XXXXXXXXX
// 
// 
//------------------------------------------------ ----------------------
bool
CAnimationFrameSetManager::SaveSpriteSetIndex(CAnimationFramePack* pAnimationFramePack,
	ofstream& setIndex, ifstream& packIndex)
{
	CSpriteSetManager ssm;


	FRAME_ARRAY* pFrameArray;

	DATA_LIST::iterator iData = m_List.begin();

	//------------------------------------------------------------------
	// 我们需要找出与所选帧 ID 相关的所有 Sprite ID。
	//------------------------------------------------------------------
	while (iData != m_List.end())
	{
		// FRAME_ARRAY를 읽어온다.
		pFrameArray = &((*pAnimationFramePack)[*iData]);

		//--------------------------------------------------------------
		// 属于每个 FRAME ARRAY 的 Frame 的 Sprite ID
		// 保存到 CSprite 集管理器。
		//--------------------------------------------------------------
		for (int i = 0; i < pFrameArray->GetSize(); i++)
		{
			ssm.Add((*pFrameArray)[i].GetSpriteID());
		}

		iData++;
	}

	//------------------------------------------------------------------
	// AnimationFrameSet과 관련된 모든 SpriteID를 
	// SpriteSetManager에 저장했으므로 
	// SpriteSetManager를 이용해 SpriteSetIndex를 생성하면된다.
	//------------------------------------------------------------------
	return ssm.SaveSpriteSetIndex(setIndex, packIndex);
}

