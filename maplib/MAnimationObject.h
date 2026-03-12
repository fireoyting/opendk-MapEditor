#ifndef __MANIMATIONOBJECT_H__
#define __MANIMATIONOBJECT_H__

#include "MImageObject.h"

// ShowTimeChecker - controls animation display during specific time periods
// Simplified version, no external dependencies
class ShowTimeChecker {
public:
    ShowTimeChecker()
        : m_Loop(false)
        , m_MinDelay(0)
        , m_MaxDelay(0)
        , m_StartHour(0)
        , m_EndHour(24)
    {}

    virtual ~ShowTimeChecker() {}

    // File I/O
    virtual void SaveToFile(ofstream& file) const
    {
        file.write((const char*)&m_Loop, 1);
        file.write((const char*)&m_MinDelay, 4);
        file.write((const char*)&m_MaxDelay, 4);
        file.write((const char*)&m_StartHour, 1);
        file.write((const char*)&m_EndHour, 1);
    }

    virtual void LoadFromFile(ifstream& file)
    {
        file.read((char*)&m_Loop, 1);
        file.read((char*)&m_MinDelay, 4);
        file.read((char*)&m_MaxDelay, 4);
        file.read((char*)&m_StartHour, 1);
        file.read((char*)&m_EndHour, 1);
    }

    // Getters
    bool IsLoop() const { return m_Loop; }
    int32_t GetMinDelay() const { return m_MinDelay; }
    int32_t GetMaxDelay() const { return m_MaxDelay; }
    uint8_t GetStartHour() const { return m_StartHour; }
    uint8_t GetEndHour() const { return m_EndHour; }

    // Setters
    void SetLoop(bool loop) { m_Loop = loop; }
    void SetDelayRange(int32_t min, int32_t max) { m_MinDelay = min; m_MaxDelay = max; }
    void SetActiveHours(uint8_t start, uint8_t end) { m_StartHour = start; m_EndHour = end; }

protected:
    bool m_Loop;           // Loop animation
    int32_t m_MinDelay;   // Minimum delay
    int32_t m_MaxDelay;    // Maximum delay
    uint8_t m_StartHour;   // Start hour (0-24)
    uint8_t m_EndHour;     // End hour (0-24)
};

//----------------------------------------------------------------------
//
// MAnimationObject class - Animation object
// Inherits from MImageObject, adds animation-related fields
//
//----------------------------------------------------------------------
class MAnimationObject : public MImageObject, public ShowTimeChecker {
public:
    MAnimationObject();
    MAnimationObject(uint32_t id, uint32_t ImageObjectID, uint32_t nSprite,
                      int32_t pX, int32_t pY, uint16_t viewpoint, uint8_t trans,
                      uint8_t bltType = 0);
    virtual ~MAnimationObject();

    // Frame control
    void NextFrame();
    void SetFrameID(uint16_t frameId, uint8_t maxFrame);

    // Direction
    void SetDirection(uint8_t dir) { m_Direction = dir; }
    uint8_t GetDirection() const { return m_Direction; }

    // Sound
    bool IsSoundFrame() const { return m_SoundFrame == m_CurrentFrame; }
    void SetSoundFrame(uint8_t frame) { m_SoundFrame = frame; }
    uint8_t GetSoundFrame() const { return m_SoundFrame; }
    void SetSoundID(uint32_t id) { m_SoundID = id; }
    uint32_t GetSoundID() const { return m_SoundID; }

    // BltType
    void SetBltType(uint8_t type) { m_BltType = type; }
    uint8_t GetBltType() const { return m_BltType; }

    // Frame info
    uint16_t GetFrameID() const { return m_FrameID; }
    uint8_t GetFrame() const { return m_CurrentFrame; }
    uint8_t GetMaxFrame() const { return m_MaxFrame; }

    //-------------------------------------------------------
    // File I/O
    //-------------------------------------------------------
    virtual void SaveToFile(ofstream& file) const override;
    virtual void LoadFromFile(ifstream& file) override;

protected:
    static uint32_t LoopFrameCount;

    // Animation frame info (from CAnimationFrame)
    uint16_t m_FrameID;
    uint8_t m_CurrentFrame;
    uint8_t m_MaxFrame;
    uint8_t m_BltType;

    // Direction and sound
    uint8_t m_Direction;
    uint8_t m_SoundFrame;
    uint16_t m_SoundID;  // Changed from uint32_t to uint16_t
};

#endif
