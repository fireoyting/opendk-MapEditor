#include "MImageObject.h"
#include "Zone.h"
#include <iostream>

Zone::Zone()
    : m_Width(0)
    , m_Height(0)
    , m_hasUnknownObjectType(false)
{
}

Zone::~Zone()
{
    Release();
}

void Zone::Init(unsigned int width, unsigned int height)
{
    m_Width = width;
    m_Height = height;
    m_Sectors.resize(m_Height, std::vector<GameSector>(m_Width));
    m_imageObjectFlyweights.clear();
    m_animationObjectFlyweights.clear();
    m_shadowObjectFlyweights.clear();
    m_animationObjectInstances.clear();
    m_interactionObjectInstances.clear();
    m_imageObjectsByViewpoint.clear();
    m_hasUnknownObjectType = false;
    m_unknownObjectTypes.clear();
}

void Zone::Release()
{
    m_Sectors.clear();
    m_imageObjectFlyweights.clear();
    m_animationObjectFlyweights.clear();
    m_shadowObjectFlyweights.clear();
    m_animationObjectInstances.clear();
    m_interactionObjectInstances.clear();
    m_imageObjectsByViewpoint.clear();
    m_unknownObjectTypes.clear();
    m_hasUnknownObjectType = false;
    m_Width = 0;
    m_Height = 0;
}

bool Zone::LoadFromFile(ifstream& file)
{
    m_ZoneHeader.LoadFromFile(file);

    // Read Tile SPI offset
    uint32_t tileSPIOffset = 0;
    file.read((char*)&tileSPIOffset, 4);
    std::cout << "DEBUG: tileSPIOffset = " << tileSPIOffset << std::endl;

    file.seekg(4, std::ios::cur); // skip Object Image SPI offset
    file.read((char*)&m_Width, 2);
    file.read((char*)&m_Height, 2);
    std::cout << "DEBUG: width=" << m_Width << " height=" << m_Height << std::endl;

    if (m_Width == 0 || m_Height == 0)
    {
        return false;
    }

    m_Sectors.resize(m_Height, vector<GameSector>(m_Width));

    // Read all Tiles (Sectors)
    for (auto& row : m_Sectors) {
        for (auto& sector : row) {
            sector.LoadFromFile(file);
        }
    }

    // ======== Read Object instances ========
    // Format: objectCount(4) + objects
    // Each object: objectType(1) + MObject fields (m_ObjectType+ID+X+Y=9) + instanceCount(2) + positions
    uint32_t objectCount = 0;
    file.read((char*)&objectCount, 4);
    std::streampos posAfterCount = file.tellg();
    std::cout << "DEBUG: objectCount=" << objectCount << " at posAfterCount=" << posAfterCount << std::endl;

    std::cout << "DEBUG: Will read first objectType at pos=" << posAfterCount << std::endl;

    m_imageObjectFlyweights.clear();
    m_animationObjectFlyweights.clear();
    m_shadowObjectFlyweights.clear();
    m_animationObjectInstances.clear();
    m_interactionObjectInstances.clear();
    m_imageObjectsByViewpoint.clear();
    m_hasUnknownObjectType = false;
    m_unknownObjectTypes.clear();

    for (uint32_t i = 0; i < objectCount; i++)
    {
        std::streampos posBeforeType = file.tellg();
        uint8_t objectType = 0;
        file.read((char*)&objectType, 1);  // read object type
        std::cout << "DEBUG: object[" << i << "] type=" << (int)objectType << " at pos=" << posBeforeType << std::endl;

        // Only process implemented types - stop if unknown
        if (objectType != MObject::TYPE_OBJECT &&
            objectType != MObject::TYPE_IMAGEOBJECT &&
            objectType != MObject::TYPE_SHADOWOBJECT &&
            objectType != MObject::TYPE_ANIMATIONOBJECT &&
            objectType != MObject::TYPE_SHADOWANIMATIONOBJECT &&
            objectType != MObject::TYPE_INTERACTIONOBJECT)
        {
            std::streampos pos = file.tellg();
            printf("Warning: Unknown object type %u at index %u (pos=%u). Stopping.\n",
                   objectType, i, (uint32_t)pos);
            m_hasUnknownObjectType = true;
            if (std::find(m_unknownObjectTypes.begin(), m_unknownObjectTypes.end(), objectType) == m_unknownObjectTypes.end()) {
                m_unknownObjectTypes.push_back(objectType);
            }
            break;
        }

        // TYPE_OBJECT - base object type
        // Format: objectType(1) already read
        // MObject::LoadFromFile reads: m_ObjectType(1) + ID(4) + X(2) + Y(2) = 9 bytes
        // Total header: objectType(1) + MObject(9) + instanceCount(2) = 12 bytes
        if (objectType == MObject::TYPE_OBJECT) {
            MObject baseObj;
            baseObj.LoadFromFile(file); // reads 9 bytes: m_ObjectType + ID + X + Y
            uint32_t objID = baseObj.GetID();

            // Save to flyweight pool (one objID can have multiple MImageObject)
            MImageObject imgObj;
            imgObj.SetID(objID);
            m_imageObjectFlyweights[objID].push_back(imgObj);

            // Read instance count (2 bytes)
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);

            // Assign objectID to sectors (for occlusion)
            for (uint32_t j = 0; j < instanceCount; j++)
            {
                uint16_t x = 0, y = 0;
                file.read((char*)&x, 2);
                file.read((char*)&y, 2);

                GameSector* sector = GetSector(x, y);
                if (sector) {
                    // TYPE_OBJECT 没有 spriteID，使用 0
                    sector->SetObjectID(objID, 0);
                }
            }
        }
        else if (objectType == MObject::TYPE_IMAGEOBJECT) {
            MImageObject imgObj;
            imgObj.LoadFromFile(file);
            uint32_t imgID = imgObj.GetID();
            uint32_t spriteID = imgObj.GetSpriteID();  // 获取 spriteID

            // Save to flyweight pool (one imgID can have multiple MImageObject)
            m_imageObjectFlyweights[imgID].push_back(imgObj);

            // Read instance count
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);

            // Assign imageID to sectors (for occlusion)
            for (uint32_t j = 0; j < instanceCount; j++)
            {
                uint16_t x = 0, y = 0;
                file.read((char*)&x, 2);
                file.read((char*)&y, 2);

                GameSector* sector = GetSector(x, y);
                if (sector) {
                    sector->SetObjectID(imgID, spriteID);  // 同时保存 spriteID
                }
            }
        }
        else if (objectType == MObject::TYPE_SHADOWOBJECT) {
            // ShadowObject - stores offset relative to ImageObject (not world position)
            MImageObject imgObj;
            imgObj.LoadFromFile(file);
            uint32_t imgID = imgObj.GetID();

            // Save to shadow flyweight pool (offset data for renderer)
            ShadowObjectData shadowData;
            shadowData.imageID = imgID;
            shadowData.offsetX = (int16_t)imgObj.GetPixelX();  // offset from ImageObject
            shadowData.offsetY = (int16_t)imgObj.GetPixelY();
            m_shadowObjectFlyweights[imgID] = shadowData;

            // Also store instance positions (for reference/mapping)
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);

            // Read instance positions - maps ImageObject ID to shadow instance positions
            for (uint32_t j = 0; j < instanceCount; j++)
            {
                uint16_t x = 0, y = 0;
                file.read((char*)&x, 2);
                file.read((char*)&y, 2);

                // Store as MapObjectInstance with ImageObject directly
                MapObjectInstance inst;
                inst.imageObject = imgObj;
                inst.x = x;
                inst.y = y;
                // Note: Not adding to m_imageObjectInstances - separate management
            }
        }
        else if (objectType == MObject::TYPE_ANIMATIONOBJECT) {
            MAnimationObject animObj;
            animObj.LoadFromFile(file);
            uint32_t animID = animObj.GetID();

            // Save to flyweight pool
            m_animationObjectFlyweights[animID] = animObj;

            // Read instance count
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);

            // Read all instance positions
            for (uint32_t j = 0; j < instanceCount; j++)
            {
                uint16_t x = 0, y = 0;
                file.read((char*)&x, 2);
                file.read((char*)&y, 2);

                AnimationObjectInstance inst;
                inst.animationID = animID;
                inst.x = x;
                inst.y = y;
                m_animationObjectInstances.push_back(inst);
            }
        }
        else if (objectType == MObject::TYPE_SHADOWANIMATIONOBJECT) {
            // ShadowAnimationObject has same format as AnimationObject
            MAnimationObject animObj;
            animObj.LoadFromFile(file);
            uint32_t animID = animObj.GetID();

            // Save to flyweight pool
            m_animationObjectFlyweights[animID] = animObj;

            // Read instance count
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);

            // Read all instance positions
            for (uint32_t j = 0; j < instanceCount; j++)
            {
                uint16_t x = 0, y = 0;
                file.read((char*)&x, 2);
                file.read((char*)&y, 2);

                AnimationObjectInstance inst;
                inst.animationID = animID;
                inst.x = x;
                inst.y = y;
                m_animationObjectInstances.push_back(inst);
            }
        }
        else if (objectType == MObject::TYPE_INTERACTIONOBJECT) {
            // InteractionObject - similar to AnimationObject but with extra 2-byte type field
            MAnimationObject animObj;
            animObj.LoadFromFile(file);

            // Read additional 2-byte interaction type
            uint16_t interactionType = 0;
            file.read((char*)&interactionType, 2);

            uint32_t interactID = animObj.GetID();

            // Save to flyweight pool (reusing animation pool for simplicity)
            m_animationObjectFlyweights[interactID] = animObj;

            // Read instance count
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);

            // Read all instance positions
            for (uint32_t j = 0; j < instanceCount; j++)
            {
                uint16_t x = 0, y = 0;
                file.read((char*)&x, 2);
                file.read((char*)&y, 2);

                InteractionObjectInstance inst;
                inst.interactionID = interactID;
                inst.x = x;
                inst.y = y;
                m_interactionObjectInstances.push_back(inst);
            }
        }
        else {
            // Unknown type, skip
            file.seekg(29, std::ios::cur);
            uint16_t instanceCount = 0;
            file.read((char*)&instanceCount, 2);
            file.seekg(instanceCount * 4, std::ios::cur);
        }
    }

    return true;
}

bool Zone::SaveToFile(ofstream& file)
{
    // Save Zone header
    m_ZoneHeader.SaveToFile(file);

    // Skip reserved bytes
    char reserved[8] = { 0 };
    file.write(reserved, 8);

    // Save width and height
    file.write((char*)&m_Width, 2);
    file.write((char*)&m_Height, 2);

    // Save all Sectors
    for (auto& row : m_Sectors) {
        for (auto& sector : row) {
            sector.SaveToFile(file);
        }
    }

    return true;
}

GameSector* Zone::GetSector(uint32_t x, uint32_t y)
{
    if (y < m_Height && x < m_Width)
        return &m_Sectors[y][x];
    return nullptr;
}

const GameSector* Zone::GetSector(uint32_t x, uint32_t y) const
{
    if (y < m_Height && x < m_Width)
        return &m_Sectors[y][x];
    return nullptr;
}

const MImageObject* Zone::GetImageObject(uint32_t imageID) const
{
    auto it = m_imageObjectFlyweights.find(imageID);
    if (it != m_imageObjectFlyweights.end() && !it->second.empty())
    {
        return &it->second[0];
    }
    return nullptr;
}

const MAnimationObject* Zone::GetAnimationObject(uint32_t animationID) const
{
    auto it = m_animationObjectFlyweights.find(animationID);
    if (it != m_animationObjectFlyweights.end())
    {
        return &it->second;
    }
    return nullptr;
}

const ShadowObjectData* Zone::GetShadowObject(uint32_t imageID) const
{
    auto it = m_shadowObjectFlyweights.find(imageID);
    if (it != m_shadowObjectFlyweights.end())
    {
        return &it->second;
    }
    return nullptr;
}

void Zone::BuildCompositeObjects()
{
    m_compositeObjects.clear();

    // Build composite objects from flyweight pool
    for (const auto& pair : m_imageObjectFlyweights)
    {
        uint32_t objID = pair.first;
        const std::vector<MImageObject>& imgObjList = pair.second;

        CompositeObject comp;
        comp.objectID = objID;
        comp.objectCount = imgObjList.size();

        // Collect all spriteIDs
        for (const MImageObject& imgObj : imgObjList)
        {
            comp.spriteIDs.push_back(imgObj.GetSpriteID());
        }

        // Debug output: show spriteID list for each imgID
        char buf[512];
        sprintf(buf, "[Debug] imgID=%u, count=%zu, spriteIDs:", objID, comp.objectCount);
        std::string debugMsg = buf;
        for (size_t i = 0; i < comp.spriteIDs.size() && i < 10; i++) {
            sprintf(buf, " %u", comp.spriteIDs[i]);
            debugMsg += buf;
        }
        if (comp.spriteIDs.size() > 10) {
            debugMsg += " ...";
        }
        debugMsg += "\n";
        OutputDebugStringA(debugMsg.c_str());

        m_compositeObjects[objID] = comp;
    }
}

const CompositeObject* Zone::GetCompositeObject(uint32_t objectID) const
{
    auto it = m_compositeObjects.find(objectID);
    if (it != m_compositeObjects.end())
    {
        return &it->second;
    }
    return nullptr;
}

void Zone::BuildImageObjectViewpointIndex()
{
    m_imageObjectsByViewpoint.clear();

    // Iterate over flyweight pool: map<id, vector<MImageObject>>
    for (const auto& pair : m_imageObjectFlyweights)
    {
        const std::vector<MImageObject>& imgObjList = pair.second;
        // Traverse each MImageObject instance
        for (const MImageObject& imgObj : imgObjList)
        {
            uint16_t viewpoint = imgObj.GetViewpoint();
            m_imageObjectsByViewpoint[viewpoint].push_back(&imgObj);
        }
    }
}
