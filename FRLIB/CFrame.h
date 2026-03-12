//----------------------------------------------------------------------
// CFrame.h
//----------------------------------------------------------------------
//
// CFrame은 하나의 Sprite에 대한 정보를 가진다.
//
//----------------------------------------------------------------------
// // CF是生物 动作sprite pack frame描叙最常见的包。
// mID 是  Sprite ID。
//
// mcX, mcY 为坐标校正值，输出精灵时，
// 这是一个变量值。
// （示例）当给角色戴上眼镜时，
// 如果角色精灵是 m c X=0, m c Y=0
// 眼镜精灵可以是 m c X=20, m c Y=10。这个坐标校正值需要开发出来界面 每个图片调整， 
// 因为智能切割后的每张图都偏移了。如果不智能切割，其实对客户端性能也没有
// 影响---》我很怀疑显卡玩这个游戏风扇会转
// 新版本导入素材可以不切割素材
//
// mfEffect 是输出方法的定义。
// 镜像输出可用。
//----------------------------------------------------------------------

#ifndef	__CFRAME_H__
#define	__CFRAME_H__

#include <Windows.h>
#include "DrawTypeDef.h"
#include "TArray.h"
//----------------------------------------------------------------------
// Frame을 출력하는데 있어서 효과를 주는 FLAG
//----------------------------------------------------------------------
//#define	FLAG_FRAME_MIRROR		0x01



class CFrame {
public:
	CFrame(TYPE_SPRITEID spriteID = 0, short cx = 0, short cy = 0)
	{
		Set(spriteID, cx, cy);
	}

	virtual ~CFrame() {}
	
	virtual void	Set(TYPE_SPRITEID spriteID, short cx, short cy);//继承类可以重写该方法


	virtual void	SaveToFile(ofstream& file);//继承类可以重写该方法
	virtual void	LoadFromFile(ifstream& file);//继承类可以重写该方法


	TYPE_SPRITEID	GetSpriteID()	const { return m_SpriteID; }
	short	GetCX()		const { return m_cX; }
	short	GetCY()		const { return m_cY; }


	virtual void	operator = (const CFrame& frame); //继承类可以重写该方法




protected:
	TYPE_SPRITEID	m_SpriteID;		// SpriteSurface의 번호(0~65535)		


	short	m_cX;
	short	m_cY;

};


//----------------------------------------------------------------------
// Effect를 위한 Frame
//----------------------------------------------------------------------
class CEffectFrame : public CFrame {
public:
	CEffectFrame(TYPE_SPRITEID spriteID = 0, short cx = 0, short cy = 0, char light = 0, bool bBack = false)
	{
		//			m_bBackground = bBack;
		Set(spriteID, cx, cy, light, bBack);
	};

	void	Set(TYPE_SPRITEID spriteID, short cx, short cy, char light, bool bBack)
	{
		CFrame::Set(spriteID, cx, cy);
		m_Light = light;
		m_bBackground = bBack;
	}

	void	SetBackground() { m_bBackground = true; }
	void	UnSetBackground() { m_bBackground = false; }

	//---------------------------------------------------------------
	// File I/O
	//---------------------------------------------------------------
	void	SaveToFile(ofstream& file);
	void	LoadFromFile(ifstream& file);

	// Get
	char	GetLight() const { return m_Light; }
	bool	IsBackground() const { return m_bBackground; }

	//---------------------------------------------------------------
	// assign
	//---------------------------------------------------------------
	void	operator = (const CEffectFrame& frame);



protected:
	char		m_Light;		// 시야의 크기(빛의 밝기)
	bool		m_bBackground;
};


//----------------------------------------------------------------------
//
// Frame Array  data type 정의

//----------------------------------------------------------------------
// 生物类 大肠包小肠

// CreatureLIST{
//				ACTIONArray{
//							DIRECTION{
//										Frame{Cframe,....Cframe}
//									  }
//							}
//				}
// 
// 
typedef	TArray<CFrame, WORD>					FRAME_ARRAY;

// Direction FrameArray	
typedef	TArray<FRAME_ARRAY, BYTE>				DIRECTION_FRAME_ARRAY;

// Action FrameArray
typedef	TArray<DIRECTION_FRAME_ARRAY, BYTE>		ACTION_FRAME_ARRAY;


//----------------------------------------------------------------------
// Effect Frame
// 
// EffectFrame简单一些， 注意，工具类会额外写一个index方法里面保存精灵id和文件地址偏移。方便客户端读取
// 
// EFFECTLIST{
//            DIRECTION{8个方向    
//						EFFECTFRAME{}
// 
// 
//					   }
//            }
// 
// 
//----------------------------------------------------------------------
// FrameArray
typedef	TArray<CEffectFrame, WORD>					EFFECTFRAME_ARRAY;

// Direction FrameArray	
typedef	TArray<EFFECTFRAME_ARRAY, BYTE>				DIRECTION_EFFECTFRAME_ARRAY;




#endif