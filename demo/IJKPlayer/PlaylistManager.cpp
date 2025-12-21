#include "PlaylistManager.h"
#include "logging.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>

PlaylistManager::PlaylistManager()
    : m_currentIndex(0)
    , m_playMode(PlayMode::Sequential)
{
}

PlaylistManager::~PlaylistManager()
{
    Clear();
}

void PlaylistManager::AddItem(const std::string& filePath)
{
    if (filePath.empty()) {
        return;
    }

    auto item = std::make_unique<PlaylistItem>(filePath);
    m_items.push_back(std::move(item));
    Log::Info("Added to playlist: %s", filePath.c_str());
}

void PlaylistManager::RemoveItem(size_t index)
{
    if (index >= m_items.size()) {
        return;
    }

    m_items.erase(m_items.begin() + index);

    // 调整当前索引
    if (m_currentIndex >= m_items.size() && m_items.size() > 0) {
        m_currentIndex = m_items.size() - 1;
    } else if (m_items.size() == 0) {
        m_currentIndex = 0;
    }
}

void PlaylistManager::Clear()
{
    m_items.clear();
    m_currentIndex = 0;
}

PlaylistItem* PlaylistManager::GetItem(size_t index)
{
    if (index >= m_items.size()) {
        return nullptr;
    }
    return m_items[index].get();
}

const PlaylistItem* PlaylistManager::GetItem(size_t index) const
{
    if (index >= m_items.size()) {
        return nullptr;
    }
    return m_items[index].get();
}

void PlaylistManager::SetCurrentIndex(size_t index)
{
    if (index < m_items.size()) {
        m_currentIndex = index;
    }
}

PlaylistItem* PlaylistManager::GetCurrentItem()
{
    if (m_items.empty() || m_currentIndex >= m_items.size()) {
        return nullptr;
    }
    return m_items[m_currentIndex].get();
}

const PlaylistItem* PlaylistManager::GetCurrentItem() const
{
    if (m_items.empty() || m_currentIndex >= m_items.size()) {
        return nullptr;
    }
    return m_items[m_currentIndex].get();
}

PlaylistItem* PlaylistManager::GetNext()
{
    if (m_items.empty()) {
        return nullptr;
    }

    switch (m_playMode) {
    case PlayMode::Sequential:
        m_currentIndex = (m_currentIndex + 1) % m_items.size();
        break;
    case PlayMode::Random: {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, m_items.size() - 1);
        m_currentIndex = dis(gen);
        break;
    }
    case PlayMode::SingleLoop:
        // 单曲循环，不改变索引
        break;
    }

    return GetCurrentItem();
}

PlaylistItem* PlaylistManager::GetPrevious()
{
    if (m_items.empty()) {
        return nullptr;
    }

    switch (m_playMode) {
    case PlayMode::Sequential:
        if (m_currentIndex == 0) {
            m_currentIndex = m_items.size() - 1;
        } else {
            m_currentIndex--;
        }
        break;
    case PlayMode::Random: {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, m_items.size() - 1);
        m_currentIndex = dis(gen);
        break;
    }
    case PlayMode::SingleLoop:
        // 单曲循环，不改变索引
        break;
    }

    return GetCurrentItem();
}

bool PlaylistManager::SaveToFile(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Log::Error("Failed to open file for writing: %s", filePath.c_str());
        return false;
    }

    for (const auto& item : m_items) {
        file << item->filePath << std::endl;
    }

    file.close();
    Log::Info("Saved playlist to: %s (%zu items)", filePath.c_str(), m_items.size());
    return true;
}

bool PlaylistManager::LoadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Log::Error("Failed to open file for reading: %s", filePath.c_str());
        return false;
    }

    Clear();

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            AddItem(line);
        }
    }

    file.close();
    Log::Info("Loaded playlist from: %s (%zu items)", filePath.c_str(), m_items.size());
    return true;
}


