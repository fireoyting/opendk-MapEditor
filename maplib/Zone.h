#ifndef GAME_ZONE__H
#define GAME_ZONE__H

/***
Consistency isn't required between the game and our map editor.
Here, we just need to implement the essential features.
Additional functionality can be added to the map editor as needed.
We'll consider adding an undo feature if the situation calls for it.
pikachu 2025/10/9
        ***/

#include "MAP_PCH.h"
#include "ZoneFileHeader.h"
#include "GameSector.h"
#include "MImageObject.h"
#include "MAnimationObject.h"

// Map object instance - stores ImageObject directly (not flyweight) to preserve spriteID
struct MapObjectInstance {
    MImageObject imageObject;  // ImageObject with spriteID
    uint16_t x;               // Sector coordinate X
    uint16_t y;               // Sector coordinate Y
};

// Animation object instance
struct AnimationObjectInstance {
    uint32_t animationID;  // Reference to AnimationObject ID
    uint16_t x;            // Sector coordinate X
    uint16_t y;            // Sector coordinate Y
};

// Interaction object instance
struct InteractionObjectInstance {
    uint32_t interactionID;  // Reference to InteractionObject ID
    uint16_t x;              // Sector coordinate X
    uint16_t y;              // Sector coordinate Y
};

// ShadowObject - stores offset relative to ImageObject (not a world position)
struct ShadowObjectData {
    uint32_t imageID;    // Reference to ImageObject ID
    int16_t offsetX;     // Shadow offset X
    int16_t offsetY;     // Shadow offset Y
};

// CompositeObject - group of ImageObject instances with same ID (e.g., houses, trees)
struct CompositeObject {
    uint32_t objectID;                      // Composite object ID (same as ImageObject ID)
    size_t objectCount;                      // Number of instances for this imgID
    std::vector<uint32_t> spriteIDs;         // List of all spriteIDs for this imgID

    // Legacy fields (kept for compatibility)
    std::vector<MapObjectInstance> parts;
    uint16_t centerY;                        // Center Y = (minY + maxY) / 2

    // Sector containing the center point
    uint16_t sectorX;
    uint16_t sectorY;
};

class Zone
{
public:
    Zone();
    ~Zone();
    void        Init(unsigned int width, unsigned int height);
    void        Release();
    bool LoadFromFile(ifstream& file);
    bool SaveToFile(ofstream& file);

    // Getters
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    GameSector* GetSector(uint32_t x, uint32_t y);
    const GameSector* GetSector(uint32_t x, uint32_t y) const;

    // Flyweight pool - get complete object data
    const MImageObject* GetImageObject(uint32_t imageID) const;
    const std::map<uint32_t, std::vector<MImageObject>>& GetAllImageObjects() const { return m_imageObjectFlyweights; }
    const MAnimationObject* GetAnimationObject(uint32_t animationID) const;
    const ShadowObjectData* GetShadowObject(uint32_t imageID) const;

    // Composite objects - groups instances by objectID
    void BuildCompositeObjects();  // Call after loading to build composites
    const CompositeObject* GetCompositeObject(uint32_t objectID) const;
    const std::map<uint32_t, CompositeObject>& GetAllCompositeObjects() const { return m_compositeObjects; }

    // 按viewpoint分组的ImageObject（用于高效渲染）
    const std::map<uint16_t, std::vector<const MImageObject*>>& GetImageObjectsByViewpoint() const { return m_imageObjectsByViewpoint; }
    void BuildImageObjectViewpointIndex();

    // Debug
    size_t GetImageObjectCount() const { return m_imageObjectsByViewpoint.empty() ? 0 : 1; }  // Simplified
    size_t GetShadowObjectCount() const { return m_shadowObjectFlyweights.size(); }
    size_t GetCompositeObjectCount() const { return m_compositeObjects.size(); }

    // Unknown object type tracking
    bool HasUnknownObjectType() const { return m_hasUnknownObjectType; }
    const std::vector<uint8_t>& GetUnknownObjectTypes() const { return m_unknownObjectTypes; }

private:
    FILEINFO_ZONE_HEADER    m_ZoneHeader;
    uint32_t        m_Width;
    uint32_t        m_Height;
    vector<vector<GameSector>>    m_Sectors;

    // Flyweight pool - for occlusion lookup (one imgID can have multiple MImageObject)
    std::map<uint32_t, std::vector<MImageObject>> m_imageObjectFlyweights;
    std::map<uint32_t, MAnimationObject> m_animationObjectFlyweights;
    std::map<uint32_t, ShadowObjectData> m_shadowObjectFlyweights;  // Shadow offset per ImageObject ID
    std::map<uint32_t, CompositeObject> m_compositeObjects;        // Composite objects grouped by ID

    // Legacy instance lists (for other object types)
    std::vector<AnimationObjectInstance> m_animationObjectInstances;
    std::vector<InteractionObjectInstance> m_interactionObjectInstances;

    // 按viewpoint分组的ImageObject实例（用于高效渲染）
    std::map<uint16_t, std::vector<const MImageObject*>> m_imageObjectsByViewpoint;

    // Unknown object type tracking
    bool m_hasUnknownObjectType;
    std::vector<uint8_t> m_unknownObjectTypes;
};

#endif // !ZONE
