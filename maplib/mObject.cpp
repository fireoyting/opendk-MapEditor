#include "MObject.h"

//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------
MObject::MObject()
{
	m_ObjectType = TYPE_OBJECT;
	m_ID = OBJECTID_NULL;
	m_X = SECTORPOSITION_NULL;
	m_Y = SECTORPOSITION_NULL;
	ClearScreenRect();
}


void MObject::AddScreenRect(M_RECT* pRect)
{
	if (pRect->left < m_ScreenRect.left)		m_ScreenRect.left = pRect->left;
	if (pRect->top < m_ScreenRect.top)			m_ScreenRect.top = pRect->top;
	if (pRect->right > m_ScreenRect.right)		m_ScreenRect.right = pRect->right;
	if (pRect->bottom > m_ScreenRect.bottom)	m_ScreenRect.bottom = pRect->bottom;
}

//----------------------------------------------------------------------
// Save to File
//----------------------------------------------------------------------
void MObject::SaveToFile(ofstream& file) const
{
	file.write((const char*)&m_ObjectType, 1);
	file.write((const char*)&m_ID, sizeof(m_ID));
	file.write((const char*)&m_X, sizeof(m_X));
	file.write((const char*)&m_Y, sizeof(m_Y));
}

//----------------------------------------------------------------------
// Load from File
//----------------------------------------------------------------------
void
MObject::LoadFromFile(ifstream& file)
{
	file.read((char*)&m_ObjectType, 1);
	file.read((char*)&m_ID, sizeof(m_ID));
	file.read((char*)&m_X, sizeof(m_X));
	file.read((char*)&m_Y, sizeof(m_Y));
}
