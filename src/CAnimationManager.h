#ifndef CANIMAM
#define CANIMAM
#include <map>
#include <memory>
#include <d2d1_1.h>
#include <wrl/client.h>
#include <unordered_map>
#include <vector>
#include "FR.h"
#include <string>

//使用一个一维的 vector 来存储所有的帧，然后用一个函数来计算每个帧的索引，这样就可以用一个总的编号来访问帧了。
// 比如 get索引
class CAnimation {


public:
    CAnimation();
    CAnimation(std::shared_ptr<CFrame> frame);
    ~CAnimation();

    void AddFrame(std::shared_ptr<CFrame> frame) {
        animations.push_back(frame);
    }

    std::shared_ptr<CFrame> GetFrame(size_t frameIndex) {
      
        return animations[frameIndex];
    }
    int getSize(){
        return animations.size();
    }
    void ClearAnimations() {
        animations.clear();
    }
    // ... 其他管理动画的方法
private:
    std::vector<std::shared_ptr<CFrame>> animations;
};
class CAnimationManager {
public:
    CAnimationManager();
    static CAnimationManager& Get();
    void AddAnimation(const std::string& name, std::shared_ptr<CAnimation> animation) {
        size_t hash = std::hash<std::string>{}(name);
        animation_map[hash] = animation;
    }
    void Init();
    // 获取动画
    std::shared_ptr<CAnimation> GetAnimation(const std::string& name) {
        size_t hash = std::hash<std::string>{}(name);
        auto it = animation_map.find(hash);
        if (it != animation_map.end()) {
            return it->second;
        }
        return nullptr;
    }
    UINT32 GetMinSize();


private:
    // 私有构造函数和赋值操作符确保不会创建额外的实例

    CAnimationManager(const CAnimationManager&) = delete;
    CAnimationManager& operator=(const CAnimationManager&) = delete;

    // 存储动画的哈希映射
    std::unordered_map<size_t, std::shared_ptr<CAnimation>> animation_map;


};


#endif