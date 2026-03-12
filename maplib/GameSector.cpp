#include "GameSector.h"

GameSector::GameSector(uint16_t spriteID)
    : m_objectID(0)
{
}

GameSector::~GameSector()
{
}

void GameSector::SaveToFile(ofstream& file)
{
	file.write((char*)&m_SpriteID, sizeof(m_SpriteID));
	file.write((char*)&m_Flag, sizeof(m_Flag));
	file.write((char*)&m_light, sizeof(m_light));
}

void GameSector::LoadFromFile(ifstream& file)
{
	file.read((char*)&m_SpriteID, sizeof(m_SpriteID));
	file.read((char*)&m_Flag, sizeof(m_Flag));
	file.read((char*)&m_light, sizeof(m_light));

}
