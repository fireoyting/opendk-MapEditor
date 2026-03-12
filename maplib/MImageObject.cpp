#include "MImageObject.h"

MImageObject::MImageObject()
{
}

MImageObject::MImageObject(uint32_t id, uint32_t ImageObjectID, uint32_t nSprite, int32_t pX, int32_t pY, uint16_t viewpoint, uint8_t trans)
{
	m_ObjectType = TYPE_IMAGEOBJECT;
	m_ImageObjectID = ImageObjectID;

	m_bAnimation = false;

	// data
	m_SpriteID = nSprite;
	m_PixelX = pX;
	m_PixelY = pY;
	m_Viewpoint = viewpoint;
	m_bTrans = trans;
}

MImageObject::~MImageObject()
{
}

void MImageObject::LoadFromFile(ifstream& file)
{
	// Read base class MObject fields first
	MObject::LoadFromFile(file);

	// Then read ImageObject-specific fields
	file.read((char*)&m_ImageObjectID, 4);
	file.read((char*)&m_SpriteID, 2);  // Changed from 4 to 2 bytes
	file.read((char*)&m_PixelX, 4);
	file.read((char*)&m_PixelY, 4);
	file.read((char*)&m_Viewpoint, 2);
	file.read((char*)&m_bAnimation, 1);
	file.read((char*)&m_bTrans, 1);
}

void MImageObject::SaveToFile(ofstream& file) const
{
	// Save base class MObject fields first
	MObject::SaveToFile(file);

	// Then save ImageObject-specific fields
	file.write((const char*)&m_ImageObjectID, 4);
	file.write((const char*)&m_SpriteID, 2);  // Changed from 4 to 2 bytes
	file.write((const char*)&m_PixelX, 4);
	file.write((const char*)&m_PixelY, 4);
	file.write((const char*)&m_Viewpoint, 2);
	file.write((const char*)&m_bAnimation, 1);
	file.write((const char*)&m_bTrans, 1);
}
