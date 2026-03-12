//----------------------------------------------------------------------
// CAnimationFrame.h
//----------------------------------------------------------------------
// 一个随着 Frame 变化而成为 Animation 的类
//----------------------------------------------------------------------
//
// Frame ID 在哪个 Frame Pack 和
// 它有当前帧/最大帧的信息。
// 例子:  Frame/动作,  8/24
//----------------------------------------------------------------------
//
// [ File I/O ]
// 该方法用于实现effect多种魔法的循环播放 使得成为动画
// 仅保存帧 ID。 Max Frame 可以更改，因此该值是在运行时确定的。
// 当前帧总是从 0 开始。
// 只有Frame ID，就可以知道Max Frame的信息。
// 但是，无法知道某个 CAnimation Frame 是什么帧包。
// 无论如何.. 假设信息是由外部确定的.. - -;;
// 
//
//----------------------------------------------------------------------

#ifndef	__CANIMATIONFRAME_H__
#define	__CANIMATIONFRAME_H__

#include <Windows.h>
#include "DrawTypeDef.h"
#include <fstream>
using std::ifstream;
using std::ofstream;


class CAnimationFrame {
public:
	CAnimationFrame(BYTE bltType = BLT_NORMAL);
	~CAnimationFrame();

	//--------------------------------------------------------
	// 기본 Frame
	//--------------------------------------------------------
	void			SetFrameID(TYPE_FRAMEID FrameID, BYTE max) { m_FrameID = FrameID; m_MaxFrame = max; m_CurrentFrame = 0; }
	TYPE_FRAMEID	GetFrameID() const { return m_FrameID; }
	BYTE			GetFrame() const { return m_CurrentFrame; }
	BYTE			GetMaxFrame() const { return m_MaxFrame; }

	// 
	void			NextFrame() { if (++m_CurrentFrame == m_MaxFrame) m_CurrentFrame = 0; }

	//--------------------------------------------------------
	// file I/O
	//--------------------------------------------------------
	void	SaveToFile(ofstream& file);
	void	LoadFromFile(ifstream& file);

	//-------------------------------------------------------
	// 출력 방식
	//-------------------------------------------------------
	void	SetBltType(BYTE bltType) { m_BltType = bltType; }
	BYTE	GetBltType() const { return m_BltType; }
	bool	IsBltTypeNormal() const { return m_BltType == BLT_NORMAL; }
	bool	IsBltTypeEffect() const { return m_BltType == BLT_EFFECT; }
	bool	IsBltTypeShadow() const { return m_BltType == BLT_SHADOW; }
	bool	IsBltTypeScreen() const { return m_BltType == BLT_SCREEN; }


protected:
	// 当前框架信息：关于 CThing Frame Pack。
	TYPE_FRAMEID		m_FrameID;

	// 动画帧信息
	BYTE				m_CurrentFrame;	//当前的 Frame
	BYTE				m_MaxFrame;		// 最大的Frame

	//输出方式
	BYTE				m_BltType;
};

#endif

