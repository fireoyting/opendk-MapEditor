#ifndef TILE_PATTERN_MANAGER_H
#define TILE_PATTERN_MANAGER_H

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <unordered_map>

struct TilePattern
{
    std::vector<uint16_t> spriteIDs;  // 扁平化的spriteID列表
    uint32_t width;                    // 模式宽度（tile数）
    uint32_t height;                   // 模式高度（tile数）
    uint32_t count;                    // 出现次数
    uint32_t color;                    // 用户选择的颜色（ABGR）
    std::vector<uint32_t> positions;  // 出现位置的tile坐标列表

    // 计算哈希值用于比较
    size_t GetHash() const;

    // 比较两个模式是否相同
    bool operator==(const TilePattern& other) const;
};

class TilePatternManager
{
public:
    TilePatternManager();
    ~TilePatternManager();

    // 分析地图，找出重复的tile组合
    void AnalyzeMap(uint32_t mapWidth, uint32_t mapHeight, const std::vector<std::vector<uint16_t>>& mapData);

    // 获取模式数量
    size_t GetPatternCount() const { return m_patterns.size(); }

    // 获取模式（按出现次数排序）
    const TilePattern* GetPattern(size_t index) const;

    // 获取minimap颜色（根据位置）
    uint32_t GetMinimapColor(uint32_t x, uint32_t y) const;

    // 设置模式颜色
    void SetPatternColor(size_t index, uint32_t color);

    // 获取pattern的所有出现位置
    bool GetPatternPositions(size_t index, uint32_t& outTileX, uint32_t& outTileY, uint32_t& outWidth, uint32_t& outHeight) const;

    // 获取pattern数量
    size_t GetPatternPositionCount(size_t index) const;

    // 清除所有数据
    void Clear();

    // 是否已分析
    bool IsAnalyzed() const { return m_analyzed; }

private:
    // 查找或添加模式
    size_t FindOrAddPattern(const TilePattern& pattern);

    std::vector<TilePattern> m_patterns;           // 所有发现模式（按出现次数排序）
    std::vector<size_t> m_minimapPatternIndex;     // 每个位置的模式索引
    uint32_t m_mapWidth;
    uint32_t m_mapHeight;
    bool m_analyzed;
};

// TilePattern inline 实现
inline size_t TilePattern::GetHash() const
{
    size_t hash = 0;
    for (uint16_t id : spriteIDs)
    {
        hash = hash * 31 + id;
    }
    hash = hash * 17 + width;
    hash = hash * 19 + height;
    return hash;
}

inline bool TilePattern::operator==(const TilePattern& other) const
{
    if (width != other.width || height != other.height)
        return false;
    return spriteIDs == other.spriteIDs;
}

#endif
