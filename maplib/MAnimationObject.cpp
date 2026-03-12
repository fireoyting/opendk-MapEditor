#include "MAnimationObject.h"

MAnimationObject::MAnimationObject()
    : m_FrameID(0)
    , m_CurrentFrame(0)
    , m_MaxFrame(0)
    , m_BltType(0)
    , m_Direction(0)
    , m_SoundFrame(0)
    , m_SoundID(0)
{
    m_ObjectType = TYPE_ANIMATIONOBJECT;
    m_bAnimation = true;
}

MAnimationObject::MAnimationObject(uint32_t id, uint32_t ImageObjectID, uint32_t nSprite,
                                   int32_t pX, int32_t pY, uint16_t viewpoint, uint8_t trans,
                                   uint8_t bltType)
    : m_FrameID(0)
    , m_CurrentFrame(0)
    , m_MaxFrame(0)
    , m_BltType(bltType)
    , m_Direction(0)
    , m_SoundFrame(0)
    , m_SoundID(0)
{
    m_ID = id;
    m_ObjectType = TYPE_ANIMATIONOBJECT;
    m_ImageObjectID = ImageObjectID;
    m_bAnimation = true;
    m_SpriteID = nSprite;
    m_PixelX = pX;
    m_PixelY = pY;
    m_Viewpoint = viewpoint;
    m_bTrans = trans;
}

MAnimationObject::~MAnimationObject()
{
}

void MAnimationObject::SetFrameID(uint16_t frameId, uint8_t maxFrame)
{
    m_FrameID = frameId;
    m_MaxFrame = maxFrame;
    m_CurrentFrame = 0;
}

void MAnimationObject::NextFrame()
{
    if (m_Loop)
    {
        if (m_MaxFrame > 0)
            m_CurrentFrame = (uint8_t)(LoopFrameCount % m_MaxFrame);
    }
    else
    {
        if (++m_CurrentFrame >= m_MaxFrame)
        {
            m_CurrentFrame = 0;
        }
    }
}

void MAnimationObject::SaveToFile(ofstream& file) const
{
    // 1. Save MImageObject base
    MImageObject::SaveToFile(file);

    // 2. Save animation frame info (CAnimationFrame fields)
    file.write((const char*)&m_FrameID, 2);
    file.write((const char*)&m_MaxFrame, 1);
    // m_CurrentFrame not saved

    // 3. Save MAnimationObject specific fields
    file.write((const char*)&m_BltType, 1);
    file.write((const char*)&m_Direction, 1);
    file.write((const char*)&m_SoundFrame, 1);
    file.write((const char*)&m_SoundID, 2);  // Changed from 4 to 2

    // 4. Save ShowTimeChecker fields
    ShowTimeChecker::SaveToFile(file);
}

void MAnimationObject::LoadFromFile(ifstream& file)
{
    // 1. Load MImageObject base
    MImageObject::LoadFromFile(file);

    // 2. Load animation frame info (CAnimationFrame fields)
    file.read((char*)&m_FrameID, 2);
    file.read((char*)&m_MaxFrame, 1);
    m_CurrentFrame = 0;

    // 3. Load MAnimationObject specific fields
    file.read((char*)&m_BltType, 1);
    file.read((char*)&m_Direction, 1);
    file.read((char*)&m_SoundFrame, 1);
    file.read((char*)&m_SoundID, 2);  // Changed from 4 to 2

    // 4. Load ShowTimeChecker fields
    ShowTimeChecker::LoadFromFile(file);
}

uint32_t MAnimationObject::LoopFrameCount = 0;
