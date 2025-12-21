#ifndef IJK_PLAYER_WINDOW_H
#define IJK_PLAYER_WINDOW_H

#include "Utils/WinImplBase.h"
#include "PlayerController.h"
#include "VideoRenderer.h"
#include "PlaylistManager.h"
#include <memory>
#include <string>

using namespace DuiLib;

class IJKPlayerWindow : public WindowImplBase {
public:
    IJKPlayerWindow();
    virtual ~IJKPlayerWindow();

    // WindowImplBase接口实现
    virtual CDuiString GetSkinFile() override;
    virtual LPCTSTR GetWindowClassName(void) const override;
    virtual void InitWindow() override;
    virtual void Notify(TNotifyUI& msg) override;

    // 窗口消息处理
    virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

protected:
    // UI控件ID定义
    static const TCHAR* const kVideoContainer;
    static const TCHAR* const kPlayButton;
    static const TCHAR* const kPauseButton;
    static const TCHAR* const kStopButton;
    static const TCHAR* const kProgressSlider;
    static const TCHAR* const kVolumeSlider;
    static const TCHAR* const kTimeLabel;
    static const TCHAR* const kStatusLabel;
    static const TCHAR* const kPlaylistList;

private:
    // 播放器组件
    std::unique_ptr<PlayerController> m_playerController;
    std::unique_ptr<VideoRenderer> m_videoRenderer;
    std::unique_ptr<PlaylistManager> m_playlistManager;

    // UI控件指针
    CControlUI* m_videoContainer;
    CButtonUI* m_playButton;
    CButtonUI* m_pauseButton;
    CButtonUI* m_stopButton;
    CSliderUI* m_progressSlider;
    CSliderUI* m_volumeSlider;
    CLabelUI* m_timeLabel;
    CLabelUI* m_statusLabel;
    CListUI* m_playlistList;

    // 状态
    bool m_isPlaying;
    bool m_isPaused;
    std::string m_currentFile;
    UINT_PTR m_updateTimer;
    HWND m_videoHwnd;  // 视频渲染窗口的HWND
    bool m_autoPlayPending;
    bool m_videoInitPending;

    // 初始化方法
    void InitializeComponents();
    void SetupUI();
    HWND GetVideoContainerHWND();

    // 播放控制
    void OnOpenFile();
    void OnPlay();
    void OnPause();
    void OnStop();
    void OnSeek(int position);
    void OnVolumeChanged(int volume);
    void OnPlaylistItemSelected(int index);

    // 回调处理
    void OnPlayerStateChanged(IjkMsgState state, int arg1, int arg2);
    void OnVideoFrame(IjkVideoFrame* frame);

    // 定时器
    void StartUpdateTimer();
    void StopUpdateTimer();
    void UpdateProgress();
    void UpdateStatus();

    // 工具方法
    std::string FormatTime(long ms);
    void UpdatePlaylistUI();
};

#endif // IJK_PLAYER_WINDOW_H

