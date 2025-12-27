#ifndef IJK_PLAYER_WINDOW_H
#define IJK_PLAYER_WINDOW_H

#include "Utils/WinImplBase.h"
#include "PlayerController.h"
#include "VideoRenderer.h"
#include "PlaylistManager.h"
#include "MenuWnd.h"
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
    virtual LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    virtual void OnFinalMessage(HWND hWnd) override;

protected:
    // UI控件ID定义
    static const TCHAR* const kVideoContainer;
    static const TCHAR* const kTitleBar;
    static const TCHAR* const kControlPanel;
    static const TCHAR* const kPlayButton;
    static const TCHAR* const kPauseButton;
    static const TCHAR* const kStopButton;
    static const TCHAR* const kPrevButton;
    static const TCHAR* const kNextButton;
    static const TCHAR* const kFastBackwardButton;
    static const TCHAR* const kFastForwardButton;
    static const TCHAR* const kFullscreenButton;
    static const TCHAR* const kProgressSlider;
    static const TCHAR* const kVolumeSlider;
    static const TCHAR* const kTimeLabel;
    static const TCHAR* const kStatusLabel;
    static const TCHAR* const kPlaylistList;
    static const TCHAR* const kVolumeButton;
    static const TCHAR* const kVolumeZeroButton;
    static const TCHAR* const kOpenMiniButton;
    static const TCHAR* const kPlaylistShowButton;
    static const TCHAR* const kPlaylistHideButton;
    static const TCHAR* const kScreenNormalButton;
    static const TCHAR* const kSideHideButton;
    static const TCHAR* const kSideShowButton;
    static const TCHAR* const kPlaylistPanel;

private:
    // 播放器组件
    std::unique_ptr<PlayerController> m_playerController;
    std::unique_ptr<VideoRenderer> m_videoRenderer;
    std::unique_ptr<PlaylistManager> m_playlistManager;

    // UI控件指针
    CControlUI* m_videoContainer;
    CControlUI* m_titleBar;
    CControlUI* m_controlPanel;
    CButtonUI* m_playButton;
    CButtonUI* m_pauseButton;
    CButtonUI* m_stopButton;
    CButtonUI* m_prevButton;
    CButtonUI* m_nextButton;
    CButtonUI* m_fastBackwardButton;
    CButtonUI* m_fastForwardButton;
    CButtonUI* m_fullscreenButton;
    CButtonUI* m_volumeButton;
    CButtonUI* m_volumeZeroButton;
    CButtonUI* m_openMiniButton;
    CButtonUI* m_playlistShowButton;
    CButtonUI* m_playlistHideButton;
    CButtonUI* m_screenNormalButton;
    CButtonUI* m_sideHideButton;
    CButtonUI* m_sideShowButton;
    CControlUI* m_playlistPanel;
    CSliderUI* m_progressSlider;
    CSliderUI* m_volumeSlider;
    CLabelUI* m_timeLabel;
    CLabelUI* m_statusLabel;
    CListUI* m_playlistList; // res/IJKPlayer.xml 里是 TreeView，但其继承自 CListUI，按列表方式使用即可

    // 状态
    bool m_isPlaying;
    bool m_isPaused;
    std::string m_currentFile;
    UINT_PTR m_updateTimer;
    HWND m_videoHwnd;  // 视频渲染窗口的HWND
    bool m_autoPlayPending;
    bool m_videoInitPending;
    int m_lastVolumeBeforeMute; // 用于静音/取消静音恢复

    // 双击视频区域全屏（无边框）状态
    bool m_videoFullscreen;
    WINDOWPLACEMENT m_prevPlacement;
    LONG_PTR m_prevStyle;
    LONG_PTR m_prevExStyle;
    bool m_prevTitleVisible;
    bool m_prevControlVisible;
    bool m_prevPlaylistVisible;

    WNDPROC m_videoOldProc;
    static LRESULT CALLBACK VideoHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 初始化方法
    void InitializeComponents();
    void SetupUI();
    HWND GetVideoContainerHWND();
    void ToggleVideoFullscreen();
    void EnterVideoFullscreen();
    void ExitVideoFullscreen();
    void SetChromeVisible(bool visible);

    // 播放控制
    void OnOpenFile();
    void OnOpenFolder();
    void OnOpenNetworkStream();
    void OnPlay();
    void OnPause();
    void OnStop();
    void OnPrev();
    void OnNext();
    void OnFastBackward();
    void OnFastForward();
    void OnFullscreen();
    void OnScreenNormal();
    void OnToggleNoFrame();
    void OnSeek(int position);
    void OnVolumeChanged(int volume);
    void OnPlaylistItemSelected(int index);
    void OnAddToPlaylist();
    
    // 播放列表控制
    void SetPlaylistVisible(bool visible);

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

