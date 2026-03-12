
#ifndef	__MOBJECT_H__
#define	__MOBJECT_H__
//darkeden v2 The map is  semi-finished product, and many functions not been implemented.
// To complete in v6, it is necessary to complete these functions.
// Map animations, map events, etc. Interactive elements like doors, treasure chests,
// reserved door opening functions without server broadcast implementation, so they are invalid.
// This class defines static map objects. In gamesector, it specifies tile sprite objects,
// and can also have multiple imageobjects and effects.
/***
*The Animation OBject class definition of dynamic Animation objects on a map, such as flames and rivers.
+------------------- Zone -------------------+
| ZoneHeader                                  |
| Width (2 bytes)                             |
| Height (2 bytes)                            |
|                                             |
| +----------- m_Sectors (2D array) ---------+ |
| | [0,0] GameSector                         | |
| |   - mspriteID (2 bytes)                  | |
| |   - Static image object list (vector)    | |
| |       +-- MImageObject                    | |
| |           +-- count                       | |
| |           +-- position (2 bytes*2)        | |
| |   - Character object list (vector)       | |
| |       +-- CharacterObject                 | |
| |   - Animation object list (vector)       | |
| |       +-- AnimationObject                 | |
| |   - Interaction object list (vector)     | |
| |       +-- InterActionObject               | |
| | [0,1] GameSector ...                     | |
| | ...                                      | |
| +------------------------------------------+ |
+-----------------------------------------
pikachu 202500907
Currently only need to display map, read tile, object, animationobject.
***/

#include "MAP_PCH.h"
typedef struct tagMRECT
{
	int32_t    left;
	int32_t    top;
	int32_t    right;
	int32_t    bottom;
} M_RECT;

class MObject {
public:
	MObject();
	~MObject() {}

	uint8_t	GetObjectType()	const { return m_ObjectType; }

	//--------------------------------------------------------
	// id
	//--------------------------------------------------------
	void			SetID(uint32_t id) { m_ID = id; }
	uint32_t	GetID()	const { return m_ID; }

	//--------------------------------------------------------
	//	sprite position
	//--------------------------------------------------------
	void		SetPosition(uint16_t x, uint16_t y) { m_X = x; m_Y = y; }
	void		SetX(uint16_t x) { m_X = x; }
	void		SetY(uint16_t y) { m_Y = y; }
	uint16_t		GetX()	const { return m_X; }
	uint16_t		GetY()	const { return m_Y; }


	void				ClearScreenRect()
	{
		m_ScreenRect.left = 800;	//
		m_ScreenRect.top = 600;		// temp values
		m_ScreenRect.right = 0;
		m_ScreenRect.bottom = 0;
	}
	void	AddScreenRect(M_RECT* pRect);
	void	SetScreenRect(M_RECT* pRect) { m_ScreenRect = *pRect; }
	const M_RECT& GetScreenRect() const { return m_ScreenRect; }
	bool				IsPointInScreenRect(int x, int y) const
	{
		if (x >= m_ScreenRect.left && x < m_ScreenRect.right
			&& y >= m_ScreenRect.top && y < m_ScreenRect.bottom)
		{
			return TRUE;
		}

		return FALSE;
	}

	//--------------------------------------------------------
	// file I/O
	//--------------------------------------------------------
	virtual void	SaveToFile(ofstream& file) const;
	virtual void	LoadFromFile(ifstream& file);

	//--------------------------------------------------------
	// Object type definition: class ID
	//--------------------------------------------------------
	enum OBJECT_TYPE
	{
		TYPE_OBJECT = 0,
		TYPE_CREATURE,
		TYPE_ITEM,
		TYPE_IMAGEOBJECT,
		TYPE_SHADOWOBJECT,
		TYPE_ANIMATIONOBJECT,
		TYPE_SHADOWANIMATIONOBJECT,
		TYPE_INTERACTIONOBJECT,
		TYPE_PORTAL,
		TYPE_EFFECT
	};

protected:
	uint8_t				m_ObjectType;	// Object type
	uint32_t		m_ID;			// Object instance ID

	// Position
	uint16_t				m_X, m_Y;		// Sector coordinates (not in pixels!)

	// Sprite collision rectangle on screen
	M_RECT					m_ScreenRect;
};

#endif
