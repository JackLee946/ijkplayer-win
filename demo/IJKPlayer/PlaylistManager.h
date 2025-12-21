#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

#include <string>
#include <vector>
#include <memory>

struct PlaylistItem {
    std::string filePath;
    std::string fileName;
    long duration; // 毫秒

    PlaylistItem(const std::string& path, const std::string& name = "")
        : filePath(path), fileName(name.empty() ? path : name), duration(0)
    {
        // 从路径提取文件名
        if (fileName.empty() || fileName == path) {
            size_t pos = path.find_last_of("\\/");
            if (pos != std::string::npos) {
                fileName = path.substr(pos + 1);
            } else {
                fileName = path;
            }
        }
    }
};

enum class PlayMode {
    Sequential,  // 顺序播放
    Random,     // 随机播放
    SingleLoop  // 单曲循环
};

class PlaylistManager {
public:
    PlaylistManager();
    ~PlaylistManager();

    // 播放列表操作
    void AddItem(const std::string& filePath);
    void RemoveItem(size_t index);
    void Clear();
    size_t GetCount() const { return m_items.size(); }
    PlaylistItem* GetItem(size_t index);
    const PlaylistItem* GetItem(size_t index) const;

    // 当前播放项
    void SetCurrentIndex(size_t index);
    size_t GetCurrentIndex() const { return m_currentIndex; }
    PlaylistItem* GetCurrentItem();
    const PlaylistItem* GetCurrentItem() const;

    // 播放模式
    void SetPlayMode(PlayMode mode) { m_playMode = mode; }
    PlayMode GetPlayMode() const { return m_playMode; }

    // 导航
    PlaylistItem* GetNext();
    PlaylistItem* GetPrevious();

    // 保存/加载
    bool SaveToFile(const std::string& filePath);
    bool LoadFromFile(const std::string& filePath);

private:
    std::vector<std::unique_ptr<PlaylistItem>> m_items;
    size_t m_currentIndex;
    PlayMode m_playMode;
};

#endif // PLAYLIST_MANAGER_H


