#include "TilePatternManager.h"

TilePatternManager::TilePatternManager()
    : m_mapWidth(0)
    , m_mapHeight(0)
    , m_analyzed(false)
{
}

TilePatternManager::~TilePatternManager()
{
    Clear();
}

void TilePatternManager::Clear()
{
    m_patterns.clear();
    m_minimapPatternIndex.clear();
    m_mapWidth = 0;
    m_mapHeight = 0;
    m_analyzed = false;
}

void TilePatternManager::AnalyzeMap(uint32_t mapWidth, uint32_t mapHeight, const std::vector<std::vector<uint16_t>>& mapData)
{
    Clear();

    if (mapWidth == 0 || mapHeight == 0 || mapData.empty())
        return;

    m_mapWidth = mapWidth;
    m_mapHeight = mapHeight;

    // 使用unordered_map加速hash查找
    std::unordered_map<size_t, size_t> patternHashMap;

    // 分析3x3到5x5的组合，步长为自己大小避免重复
    const uint32_t sizes[] = {3, 4, 5};

    for (uint32_t sizeIdx = 0; sizeIdx < 3; sizeIdx++)
    {
        uint32_t pw = sizes[sizeIdx];
        uint32_t ph = sizes[sizeIdx];

        if (pw > mapWidth || ph > mapHeight)
            continue;

        for (uint32_t startY = 0; startY <= mapHeight - ph; startY += pw)
        {
            for (uint32_t startX = 0; startX <= mapWidth - pw; startX += pw)
            {
                // 计算hash
                size_t hash = 0;
                bool hasValidSprite = false;
                for (uint32_t y = 0; y < ph; y++)
                {
                    for (uint32_t x = 0; x < pw; x++)
                    {
                        uint32_t tx = startX + x;
                        uint32_t ty = startY + y;
                        if (ty < mapData.size() && tx < mapData[ty].size())
                        {
                            uint16_t id = mapData[ty][tx];
                            // 过滤无效的spriteID（0和65535），用0代替计算hash
                            uint16_t filteredId = (id > 0 && id != 65535) ? id : 0;
                            if (id > 0 && id != 65535) hasValidSprite = true;
                            hash = hash * 31 + filteredId;
                        }
                    }
                }

                if (!hasValidSprite)
                    continue;

                hash = hash * 17 + pw;
                hash = hash * 19 + ph;

                // 查找或添加
                auto it = patternHashMap.find(hash);
                if (it != patternHashMap.end())
                {
                    m_patterns[it->second].count++;
                    // 保存所有位置用于高亮显示
                    m_patterns[it->second].positions.push_back(startY * mapWidth + startX);
                }
                else
                {
                    // 创建新pattern
                    TilePattern newPattern;
                    newPattern.width = pw;
                    newPattern.height = ph;
                    newPattern.count = 1;
                    newPattern.spriteIDs.reserve(pw * ph);
                    newPattern.positions.reserve(10);
                    newPattern.positions.push_back(startY * mapWidth + startX);

                    for (uint32_t y = 0; y < ph; y++)
                    {
                        for (uint32_t x = 0; x < pw; x++)
                        {
                            uint32_t tx = startX + x;
                            uint32_t ty = startY + y;
                            if (ty < mapData.size() && tx < mapData[ty].size())
                            {
                                // 过滤无效的spriteID（0和65535），用0存储
                                uint16_t id = mapData[ty][tx];
                                newPattern.spriteIDs.push_back((id > 0 && id != 65535) ? id : 0);
                            }
                        }
                    }

                    // 生成默认颜色
                    if (!newPattern.spriteIDs.empty())
                    {
                        uint16_t mainSpriteID = newPattern.spriteIDs[0];
                        uint8_t r = 80 + (mainSpriteID % 4) * 40;
                        uint8_t g = 80 + ((mainSpriteID / 4) % 4) * 40;
                        uint8_t b = 80 + ((mainSpriteID / 16) % 4) * 40;
                        newPattern.color = (0xFF << 24) | (b << 16) | (g << 8) | r;
                    }
                    else
                    {
                        newPattern.color = 0xFF808080;
                    }

                    size_t idx = m_patterns.size();
                    m_patterns.push_back(newPattern);
                    patternHashMap[hash] = idx;
                }
            }
        }
    }

    // 为每个位置分配模式（用于minimap）
    m_minimapPatternIndex.resize(mapWidth * mapHeight, 0);

    // 先建立一个spriteID到pattern索引的映射（只记录每个spriteID对应的最高频pattern）
    std::unordered_map<uint16_t, size_t> spriteToPattern;
    for (size_t i = 0; i < m_patterns.size(); i++)
    {
        const auto& p = m_patterns[i];
        for (uint16_t id : p.spriteIDs)
        {
            if (id == 0) continue;  // 跳过0
            auto it = spriteToPattern.find(id);
            if (it == spriteToPattern.end() || p.count > m_patterns[it->second].count)
            {
                spriteToPattern[id] = i;
            }
        }
    }

    for (uint32_t y = 0; y < mapHeight; y++)
    {
        for (uint32_t x = 0; x < mapWidth; x++)
        {
            uint16_t spriteID = 0;
            if (y < mapData.size() && x < mapData[y].size())
            {
                spriteID = mapData[y][x];
            }

            // 65535或0显示为黑色
            if (spriteID == 0 || spriteID == 65535)
            {
                if (y * mapWidth + x < m_minimapPatternIndex.size())
                {
                    m_minimapPatternIndex[y * mapWidth + x] = SIZE_MAX;  // 特殊标记表示黑色
                }
                continue;
            }

            // 直接查表
            size_t bestPattern = 0;
            auto it = spriteToPattern.find(spriteID);
            if (it != spriteToPattern.end())
            {
                bestPattern = it->second;
            }

            if (y * mapWidth + x < m_minimapPatternIndex.size())
            {
                m_minimapPatternIndex[y * mapWidth + x] = bestPattern;
            }
        }
    }

    // 按出现次数排序
    std::sort(m_patterns.begin(), m_patterns.end(),
        [](const TilePattern& a, const TilePattern& b)
        {
            return a.count > b.count;
        });

    m_analyzed = true;
}

const TilePattern* TilePatternManager::GetPattern(size_t index) const
{
    if (index < m_patterns.size())
        return &m_patterns[index];
    return nullptr;
}

uint32_t TilePatternManager::GetMinimapColor(uint32_t x, uint32_t y) const
{
    if (x >= m_mapWidth || y >= m_mapHeight)
        return 0xFF303040;

    size_t idx = y * m_mapWidth + x;
    if (idx >= m_minimapPatternIndex.size())
        return 0xFF303040;

    size_t patternIdx = m_minimapPatternIndex[idx];

    // SIZE_MAX表示该位置是0或65535，显示为黑色
    if (patternIdx == SIZE_MAX)
        return 0xFF000000;

    if (patternIdx < m_patterns.size())
        return m_patterns[patternIdx].color;

    return 0xFF303040;
}

void TilePatternManager::SetPatternColor(size_t index, uint32_t color)
{
    if (index < m_patterns.size())
    {
        m_patterns[index].color = color;
    }
}

bool TilePatternManager::GetPatternPositions(size_t index, uint32_t& outTileX, uint32_t& outTileY, uint32_t& outWidth, uint32_t& outHeight) const
{
    if (index >= m_patterns.size() || m_patterns[index].positions.empty())
        return false;

    // 返回第一个位置和pattern的宽高
    uint32_t pos = m_patterns[index].positions[0];
    outTileY = pos / m_mapWidth;
    outTileX = pos % m_mapWidth;
    outWidth = m_patterns[index].width;
    outHeight = m_patterns[index].height;
    return true;
}

size_t TilePatternManager::GetPatternPositionCount(size_t index) const
{
    if (index >= m_patterns.size())
        return 0;
    return m_patterns[index].positions.size();
}
