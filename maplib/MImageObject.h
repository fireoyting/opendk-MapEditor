#ifndef	__MIMAGEOBJECT_H__
#define	__MIMAGEOBJECT_H__
#include "MAP_PCH.h"
#include "MObject.h"



class MImageObject : public MObject {
public:

	enum WALL_DIRECTION
	{
		WALL_RIGHTDOWN = 1,
		WALL_RIGHTUP,

		WALL_NULL = 0xffff,
	};

public:
	MImageObject();
	MImageObject(uint32_t id, uint32_t ImageObjectID, uint32_t nSprite, int32_t pX, int32_t pY, uint16_t viewpoint, uint8_t trans);
	~MImageObject();

	// Getters
	int32_t GetPixelX() const { return m_PixelX; }
	int32_t GetPixelY() const { return m_PixelY; }
	uint32_t GetSpriteID() const { return m_SpriteID; }
	uint32_t GetImageObjectID() const { return m_ImageObjectID; }
	uint16_t GetViewpoint() const { return m_Viewpoint; }

	virtual void	SaveToFile(ofstream& file) const;
	virtual void	LoadFromFile(ifstream& file);
protected:


	uint32_t m_ImageObjectID;
	uint32_t m_SpriteID;
	uint8_t	m_bAnimation;
	int32_t	m_PixelX;
	int32_t	m_PixelY;
	uint16_t m_Viewpoint;
	uint8_t	m_bTrans;


};

#endif// __MIMAGEOBJECT_H__