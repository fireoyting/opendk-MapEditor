#include "CAnimationManager.h"
#include <unordered_set>

namespace
{
    // TextureManager单例
    CAnimationManager* s_pInstance = nullptr;
}
CAnimationManager::CAnimationManager()
{
    if (s_pInstance)
        throw std::exception("TextureManager is a singleton!");
    s_pInstance = this;
}
CAnimationManager& CAnimationManager::Get()
{
    if (!s_pInstance)
        throw std::exception("CAnimationManager needs an instance!");
    return *s_pInstance;
}

void CAnimationManager::Init()
{
    //创建一个初始序号为0的空白animation
    std::shared_ptr<CFrame> emptyFrame = std::make_shared<CFrame>(0,0,0);
    std::shared_ptr<CAnimation> emptyAnimat  = std::make_shared<CAnimation>(emptyFrame);
    AddAnimation("empty",emptyAnimat);
}

UINT32 CAnimationManager::GetMinSize()
{
    // 使用 std::min_element 找到最小的 vector 大小
    // 注释掉, 使用冒泡排序排除掉指定的key.
    /*auto min_size_iter = std::min_element(animation_map.begin(), animation_map.end(),
        [](const auto& a, const auto& b) {

            return a.second.size() < b.second.size();
        });

    size_t min_size = min_size_iter->second.size();*/
    // 注释结束
    size_t min_size = 999999;
    std::unordered_set<size_t> keys_to_exclude = { std::hash<std::string>{}("empty"),  std::hash<std::string>{}("Frame")};
    for (auto& entry : animation_map) 
    {
        if (keys_to_exclude.find(entry.first) == keys_to_exclude.end()) 
        {
            min_size = min(min_size, entry.second->getSize());
        }
    }
    return min_size;
}

CAnimation::CAnimation()
{
}

CAnimation::CAnimation(std::shared_ptr<CFrame> frame)
{
    AddFrame(frame);
}

CAnimation::~CAnimation()
{
}
