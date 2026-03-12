#ifndef GAMESECTOR_H
#define GAMESECTOR_H

#include "MAP_PCH.h"
//----------------------------------------------------------------------
// Flag
//----------------------------------------------------------------------
#define FLAG_SECTOR_BLOCK_UNDERGROUND        0x01    //  Block underground?
#define FLAG_SECTOR_BLOCK_GROUND            0x02    //  Block  ground?
#define FLAG_SECTOR_BLOCK_FLYING            0x04    //  Block  air?
#define FLAG_SECTOR_BLOCK_ALL                0x07    // Block ALL
#define FLAG_SECTOR_ITEM                    0x08    // Item exists
#define FLAG_SECTOR_UNDERGROUNDCREATURE        0x10    // Underground creature exists
#define FLAG_SECTOR_GROUNDCREATURE            0x20    // Ground creature exists
#define FLAG_SECTOR_FLYINGCREATURE            0x40    // Flying creature exists
#define FLAG_SECTOR_PORTAL                    0x80    // Portal exists

//----------------------------------------------------------------------
// Flag2
//----------------------------------------------------------------------
#define FLAG_SECTOR_SAFE_COMMON                0x01    // All safe
#define FLAG_SECTOR_SAFE_SLAYER                0x02    // Slayer safe
#define FLAG_SECTOR_SAFE_VAMPIRE            0x04    // Vampire safe
#define FLAG_SECTOR_SAFE_NO_PK_ZONE            0x08    // No PK zone
#define FLAG_SECTOR_SAFE_OUSTERS            0x10    // Ousters safe
#define FLAG_SECTOR_SAFE_ZONE                0x17
// blocked by server
#define FLAG_SECTOR_BLOCK_SERVER_UNDERGROUND    0x10
#define FLAG_SECTOR_BLOCK_SERVER_GROUND        0x20
#define FLAG_SECTOR_BLOCK_SERVER_FLYING        0x40


class GameSector
{
public:
    GameSector(uint16_t spriteID = 0);
    ~GameSector();

    //------------------------------------------------
    //
    // file I/O
    //
    //------------------------------------------------
    void    SaveToFile(ofstream& file);
    void    LoadFromFile(ifstream& file);

    // Getters
    uint16_t GetSpriteID() const { return m_SpriteID; }
    uint8_t GetFlag() const { return m_Flag; }
    uint8_t GetLight() const { return m_light; }
    uint32_t GetObjectID() const { return m_objectID; }
    uint32_t GetObjectSpriteID() const { return m_objectSpriteID; }  // ImageObject 对应的 spriteID

    // Setters
    void SetSpriteID(uint16_t spriteID) { m_SpriteID = spriteID; }
    void SetFlag(uint8_t flag) { m_Flag = flag; }
    void SetLight(uint8_t light) { m_light = light; }
    void SetObjectID(uint32_t objectID, uint32_t objectSpriteID = 0) {
        m_objectID = objectID;
        m_objectSpriteID = objectSpriteID;
    }

protected:
    uint16_t m_SpriteID;
    uint8_t m_Flag;        // Sector Flag
    uint8_t m_light;       // Sector Light
    uint32_t m_objectID;   // ImageObject ID (0 = no object)
    uint32_t m_objectSpriteID;  // ImageObject 对应的 spriteID

};

#endif //!GAMESECTOR_H
