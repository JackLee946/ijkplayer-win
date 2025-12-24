#include "IJKPlayerWindow.h"
#include "Control/UIButton.h"
#include "Control/UISlider.h"
#include "Control/UILabel.h"
#include "Control/UIList.h"
#include "Core/UIManager.h"
#include "logging.h"
#include <commdlg.h>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <io.h>

// 控件ID定义
const TCHAR* const IJKPlayerWindow::kVideoContainer = _T("video_container");
const TCHAR* const IJKPlayerWindow::kPlayButton = _T("btn_play");
const TCHAR* const IJKPlayerWindow::kPauseButton = _T("btn_pause");
const TCHAR* const IJKPlayerWindow::kStopButton = _T("btn_stop");
const TCHAR* const IJKPlayerWindow::kPrevButton = _T("btn_prev");
const TCHAR* const IJKPlayerWindow::kNextButton = _T("btn_next");
const TCHAR* const IJKPlayerWindow::kFastBackwardButton = _T("btn_fast_backward");
const TCHAR* const IJKPlayerWindow::kFastForwardButton = _T("btn_fast_forward");
const TCHAR* const IJKPlayerWindow::kFullscreenButton = _T("btn_fullscreen");
const TCHAR* const IJKPlayerWindow::kProgressSlider = _T("slider_progress");
const TCHAR* const IJKPlayerWindow::kVolumeSlider = _T("slider_volume");
const TCHAR* const IJKPlayerWindow::kTimeLabel = _T("label_time");
const TCHAR* const IJKPlayerWindow::kStatusLabel = _T("label_status");
const TCHAR* const IJKPlayerWindow::kPlaylistList = _T("list_playlist");
const TCHAR* const IJKPlayerWindow::kMinimizeButton = _T("btn_minimize");
const TCHAR* const IJKPlayerWindow::kMaximizeButton = _T("btn_maximize");
const TCHAR* const IJKPlayerWindow::kCloseButton = _T("btn_close");
const TCHAR* const IJKPlayerWindow::kVolumeButton = _T("btn_volume");
const TCHAR* const IJKPlayerWindow::kVolumeZeroButton = _T("btn_volume_zero");
const TCHAR* const IJKPlayerWindow::kOpenMiniButton = _T("btn_open_mini");
const TCHAR* const IJKPlayerWindow::kPlaylistShowButton = _T("btnPlaylistShow");
const TCHAR* const IJKPlayerWindow::kPlaylistHideButton = _T("btnPlaylistHide");
const TCHAR* const IJKPlayerWindow::kScreenNormalButton = _T("btn_screen_normal");
const TCHAR* const IJKPlayerWindow::kSideHideButton = _T("btnSideHide");
const TCHAR* const IJKPlayerWindow::kSideShowButton = _T("btnSideShow");
const TCHAR* const IJKPlayerWindow::kPlaylistPanel = _T("playlist_panel");

IJKPlayerWindow::IJKPlayerWindow()
    : m_videoContainer(nullptr)
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_stopButton(nullptr)
    , m_prevButton(nullptr)
    , m_nextButton(nullptr)
    , m_fastBackwardButton(nullptr)
    , m_fastForwardButton(nullptr)
    , m_fullscreenButton(nullptr)
    , m_minimizeButton(nullptr)
    , m_maximizeButton(nullptr)
    , m_closeButton(nullptr)
    , m_volumeButton(nullptr)
    , m_volumeZeroButton(nullptr)
    , m_openMiniButton(nullptr)
    , m_playlistShowButton(nullptr)
    , m_playlistHideButton(nullptr)
    , m_screenNormalButton(nullptr)
    , m_sideHideButton(nullptr)
    , m_sideShowButton(nullptr)
    , m_playlistPanel(nullptr)
    , m_progressSlider(nullptr)
    , m_volumeSlider(nullptr)
    , m_timeLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_playlistList(nullptr)
    , m_pMenuWnd(nullptr)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_updateTimer(0)
    , m_videoHwnd(nullptr)
    , m_autoPlayPending(false)
    , m_videoInitPending(false)
{
    m_playerController = std::make_unique<PlayerController>();
    m_videoRenderer = std::make_unique<VideoRenderer>();
    m_playlistManager = std::make_unique<PlaylistManager>();
}

IJKPlayerWindow::~IJKPlayerWindow()
{
    StopUpdateTimer();
    if (m_playerController) {
        m_playerController->Release();
    }
    if (m_videoRenderer) {
        m_videoRenderer->Release();
    }
    if (m_videoHwnd && IsWindow(m_videoHwnd)) {
        m_pm.RemoveNativeWindow(m_videoHwnd);
        DestroyWindow(m_videoHwnd);
        m_videoHwnd = nullptr;
    }
}

CDuiString IJKPlayerWindow::GetSkinFile()
{
    return _T("IJKPlayer.xml");
}

LPCTSTR IJKPlayerWindow::GetWindowClassName(void) const
{
    return _T("IJKPlayerWindow");
}

void IJKPlayerWindow::InitWindow()
{
    SetupUI();
    InitializeComponents();

    // 自测功能已移除：不再自动最大化或自动退出
    // 如需自测，请手动创建 selftest.maximize 文件并重新编译启用相关代码

    // 自动播放同目录下的测试文件，便于验证播放链路
    if (m_currentFile.empty() && _access("./test.flv", 0) == 0) {
        const std::string filePath = "./test.flv";
        m_playlistManager->AddItem(filePath);
        m_playlistManager->SetCurrentIndex(m_playlistManager->GetCount() - 1);
        UpdatePlaylistUI();

        m_autoPlayPending = false;
        if (m_playerController->OpenFile(filePath) && m_playerController->Prepare()) {
            m_currentFile = filePath;
            m_autoPlayPending = true;
            if (m_statusLabel) m_statusLabel->SetText(_T("Preparing test.flv..."));
        }
    }
}

void IJKPlayerWindow::SetupUI()
{
    // 获取UI控件指针
    m_videoContainer = m_pm.FindControl(kVideoContainer);
    m_playButton = static_cast<CButtonUI*>(m_pm.FindControl(kPlayButton));
    m_pauseButton = static_cast<CButtonUI*>(m_pm.FindControl(kPauseButton));
    m_stopButton = static_cast<CButtonUI*>(m_pm.FindControl(kStopButton));
    m_prevButton = static_cast<CButtonUI*>(m_pm.FindControl(kPrevButton));
    m_nextButton = static_cast<CButtonUI*>(m_pm.FindControl(kNextButton));
    m_fastBackwardButton = static_cast<CButtonUI*>(m_pm.FindControl(kFastBackwardButton));
    m_fastForwardButton = static_cast<CButtonUI*>(m_pm.FindControl(kFastForwardButton));
    m_fullscreenButton = static_cast<CButtonUI*>(m_pm.FindControl(kFullscreenButton));
    m_screenNormalButton = static_cast<CButtonUI*>(m_pm.FindControl(kScreenNormalButton));
    m_minimizeButton = static_cast<CButtonUI*>(m_pm.FindControl(kMinimizeButton));
    m_maximizeButton = static_cast<CButtonUI*>(m_pm.FindControl(kMaximizeButton));
    m_closeButton = static_cast<CButtonUI*>(m_pm.FindControl(kCloseButton));
    m_volumeButton = static_cast<CButtonUI*>(m_pm.FindControl(kVolumeButton));
    m_volumeZeroButton = static_cast<CButtonUI*>(m_pm.FindControl(kVolumeZeroButton));
    m_openMiniButton = static_cast<CButtonUI*>(m_pm.FindControl(kOpenMiniButton));
    m_playlistShowButton = static_cast<CButtonUI*>(m_pm.FindControl(kPlaylistShowButton));
    m_playlistHideButton = static_cast<CButtonUI*>(m_pm.FindControl(kPlaylistHideButton));
    m_sideHideButton = static_cast<CButtonUI*>(m_pm.FindControl(kSideHideButton));
    m_sideShowButton = static_cast<CButtonUI*>(m_pm.FindControl(kSideShowButton));
    m_playlistPanel = m_pm.FindControl(kPlaylistPanel);
    m_progressSlider = static_cast<CSliderUI*>(m_pm.FindControl(kProgressSlider));
    m_volumeSlider = static_cast<CSliderUI*>(m_pm.FindControl(kVolumeSlider));
    m_timeLabel = static_cast<CLabelUI*>(m_pm.FindControl(kTimeLabel));
    m_statusLabel = static_cast<CLabelUI*>(m_pm.FindControl(kStatusLabel));
    m_playlistList = static_cast<CListUI*>(m_pm.FindControl(kPlaylistList));

    // 设置初始状态
    if (m_pauseButton) {
        m_pauseButton->SetVisible(false);
    }
    if (m_screenNormalButton) {
        m_screenNormalButton->SetVisible(false);
    }
    if (m_progressSlider) {
        m_progressSlider->SetMinValue(0);
        m_progressSlider->SetMaxValue(1000);
    }
    if (m_volumeSlider) {
        m_volumeSlider->SetMinValue(0);
        m_volumeSlider->SetMaxValue(100);
        m_volumeSlider->SetValue(100); // 默认100%音量
    }
    if (m_volumeZeroButton) {
        m_volumeZeroButton->SetVisible(false);
    }
}

void IJKPlayerWindow::InitializeComponents()
{
    // 初始化播放器控制器
    if (!m_playerController->Initialize()) {
        Log::Error("Failed to initialize PlayerController");
        return;
    }

    // 设置回调
    m_playerController->SetStateCallback(
        [this](IjkMsgState state, int arg1, int arg2) {
            OnPlayerStateChanged(state, arg1, arg2);
        });

    m_playerController->SetVideoFrameCallback(
        [this](IjkVideoFrame* frame) {
            OnVideoFrame(frame);
        });

    // 视频渲染窗口/SDL 初始化需要等布局完成后再做（否则 child HWND 可能是 0x0 大小）
    m_videoInitPending = true;
    ::SetTimer(m_hWnd, 2, 100, NULL);

    // 设置初始音量（0~100）
    m_playerController->SetVolume(50.0f);
}

HWND IJKPlayerWindow::GetVideoContainerHWND()
{
    if (!m_videoContainer) {
        return nullptr;
    }

    // duilib控件本身没有HWND，需要创建一个原生子窗口
    // 获取控件的矩形区域
    RECT rect = m_videoContainer->GetPos();
    
    // 创建或更新视频渲染窗口
    if (!m_videoHwnd || !IsWindow(m_videoHwnd)) {
        m_videoHwnd = CreateWindow(
            _T("STATIC"),
            _T(""),
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            m_hWnd,
            (HMENU)1001,  // 使用一个ID
            GetModuleHandle(NULL),
            NULL);
        
        if (m_videoHwnd) {
            // 注册到duilib的原生窗口管理
            m_pm.AddNativeWindow(m_videoContainer, m_videoHwnd);
        }
    } else {
        // 更新窗口位置和大小
        SetWindowPos(m_videoHwnd, NULL, rect.left, rect.top, 
                     rect.right - rect.left, rect.bottom - rect.top,
                     SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    return m_videoHwnd;
}

void IJKPlayerWindow::Notify(TNotifyUI& msg)
{
    if (msg.sType == DUI_MSGTYPE_CLICK) {
        CDuiString name = msg.pSender->GetName();

        // 先交给 WindowImplBase 处理系统按钮（最小化/最大化/还原/关闭）
        if (name == _T("closebtn") || name == _T("minbtn") || name == _T("maxbtn") || name == _T("restorebtn")) {
            WindowImplBase::OnClick(msg);
            return;
        }

        // 窗口控制按钮
        if (name == kMinimizeButton) {
            ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return;
        }
        else if (name == kMaximizeButton) {
            if (::IsZoomed(m_hWnd)) {
                ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            } else {
                ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            }
            return;
        }
        else if (name == kCloseButton) {
            ::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
            return;
        }
        // “全屏”按钮：用主窗口最大化/还原实现（嵌入 HWND 的 SDL_Window 不适合 SDL_SetWindowFullscreen）
        else if (name == kFullscreenButton) {
            if (::IsZoomed(m_hWnd)) {
                ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            } else {
                ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            }
            return;
        }
        
        if (name == kPlayButton) {
            OnPlay();
        }
        else if (name == kPauseButton) {
            OnPause();
        }
        else if (name == kStopButton) {
            OnStop();
        }
        else if (name == kPrevButton) {
            OnPrev();
        }
        else if (name == kNextButton) {
            OnNext();
        }
        else if (name == kFastBackwardButton) {
            OnFastBackward();
        }
        else if (name == kFastForwardButton) {
            OnFastForward();
        }
        else if (name == kFullscreenButton) {
            OnFullscreen();
        }
        else if (name == kScreenNormalButton) {
            OnFullscreen(); // 退出全屏与全屏使用相同的逻辑
        }
        else if (name == kVolumeButton) {
            // 点击音量按钮切换到静音状态
            if (m_volumeButton && m_volumeZeroButton) {
                m_volumeButton->SetVisible(false);
                m_volumeZeroButton->SetVisible(true);
                if (m_playerController) {
                    m_playerController->SetVolume(0.0f);
                    if (m_volumeSlider) {
                        m_volumeSlider->SetValue(0);
                    }
                }
            }
        }
        else if (name == kVolumeZeroButton) {
            // 点击静音按钮恢复音量
            if (m_volumeButton && m_volumeZeroButton) {
                m_volumeButton->SetVisible(true);
                m_volumeZeroButton->SetVisible(false);
                if (m_playerController) {
                    m_playerController->SetVolume(50.0f); // 恢复到50%音量
                    if (m_volumeSlider) {
                        m_volumeSlider->SetValue(50);
                    }
                }
            }
        }
        else if (name == kOpenMiniButton) {
            OnOpenFile();
        }
        else if (name == _T("logotext") || name == _T("logo")) {
            // 点击标题按钮或logo显示下拉菜单
            POINT pt = { msg.ptMouse.x, msg.ptMouse.y };
            CDuiRect rc = msg.pSender->GetPos();
            pt.x = rc.left;
            pt.y = rc.bottom;
            
            // 创建并显示自定义菜单窗口
            if (m_pMenuWnd == nullptr) {
                m_pMenuWnd = new MenuWnd(_T("menu.xml"));
            }
            
            if (m_pMenuWnd != nullptr) {
                m_pMenuWnd->Init(&m_pm, pt);
                m_pMenuWnd->ShowWindow(true);
            }
        }
        else if (name == kPlaylistShowButton) {
            // 显示播放列表
            if (m_playlistShowButton && m_playlistHideButton) {
                m_playlistShowButton->SetVisible(false);
                m_playlistHideButton->SetVisible(true);
                // 显示播放列表面板
                if (m_playlistPanel) {
                    m_playlistPanel->SetVisible(true);
                }
            }
        }
        else if (name == kPlaylistHideButton) {
            // 隐藏播放列表
            if (m_playlistShowButton && m_playlistHideButton) {
                m_playlistShowButton->SetVisible(true);
                m_playlistHideButton->SetVisible(false);
                // 隐藏播放列表面板
                if (m_playlistPanel) {
                    m_playlistPanel->SetVisible(false);
                }
            }
        }
        else if (name == kSideHideButton) {
            // 隐藏播放列表
            if (m_sideHideButton && m_sideShowButton) {
                m_sideHideButton->SetVisible(false);
                m_sideShowButton->SetVisible(true);
                // 隐藏播放列表面板
                if (m_playlistPanel) {
                    m_playlistPanel->SetVisible(false);
                }
            }
        }
        else if (name == kSideShowButton) {
            // 显示播放列表
            if (m_sideHideButton && m_sideShowButton) {
                m_sideHideButton->SetVisible(true);
                m_sideShowButton->SetVisible(false);
                // 显示播放列表面板
                if (m_playlistPanel) {
                    m_playlistPanel->SetVisible(true);
                }
            }
        }
        else if (name == _T("btn_open")) {
            OnOpenFile();
        }
        // 处理菜单点击事件
        else if (name == _T("menu_OpenFile")) {
            OnOpenFile();
        }
        else if (name == _T("menu_AddToPlaylist")) {
            OnAddToPlaylist();
        }
        else if (name == _T("menu_Exit")) {
            ::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
        }
    }
    else if (msg.sType == DUI_MSGTYPE_VALUECHANGED) {
        CDuiString name = msg.pSender->GetName();
        
        if (name == kProgressSlider && m_progressSlider) {
            int value = m_progressSlider->GetValue();
            OnSeek(value);
        }
        else if (name == kVolumeSlider && m_volumeSlider) {
            int value = m_volumeSlider->GetValue();
            OnVolumeChanged(value);
        }
    }
    else if (msg.sType == DUI_MSGTYPE_ITEMDBCLICK) {
        if (m_playlistList && msg.pSender == m_playlistList) {
            int index = m_playlistList->GetCurSel();
            OnPlaylistItemSelected(index);
        }
    }
}

LRESULT IJKPlayerWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    StopUpdateTimer();
    if (m_playerController) {
        m_playerController->Stop();
        m_playerController->Release();
    }
    if (m_videoRenderer) {
        m_videoRenderer->Release();
    }
    bHandled = FALSE;
    return 0;
}

LRESULT IJKPlayerWindow::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt;
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    ::ClientToScreen(m_hWnd, &pt);

    HMENU hMenu = ::CreatePopupMenu();
    if (hMenu) {
        ::AppendMenu(hMenu, MF_STRING, 1001, _T("打开文件"));
        ::AppendMenu(hMenu, MF_STRING, 1002, _T("添加到播放列表"));
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, 1003, _T("退出"));

        ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
        ::DestroyMenu(hMenu);
    }

    bHandled = TRUE;
    return 0;
}

LRESULT IJKPlayerWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int id = LOWORD(wParam);
    if (id == 1001) {
        OnOpenFile();
        bHandled = TRUE;
        return 0;
    } else if (id == 1002) {
        OnAddToPlaylist();
        bHandled = TRUE;
        return 0;
    } else if (id == 1003) {
        ::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
        bHandled = TRUE;
        return 0;
    }

    bHandled = FALSE;
    return 0;
}

LRESULT IJKPlayerWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // 注意：duilib 的布局刷新发生在 WindowImplBase::HandleMessage() 后续的 m_pm.MessageHandler() 中，
    // 在这里直接读 m_videoContainer->GetPos() 拿到的是“旧布局”，会导致最大化后仍然按旧尺寸摆放。
    // 做法：先让消息继续往下走完成布局，然后异步触发一次“子窗口对齐”。
    LRESULT lRes = WindowImplBase::OnSize(uMsg, wParam, lParam, bHandled);
    ::PostMessage(m_hWnd, WM_APP + 101, 0, 0);
    return lRes;
}

LRESULT IJKPlayerWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (uMsg == WM_TIMER && wParam == 1) {
        UpdateProgress();
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == WM_TIMER && wParam == 2) {
        if (m_videoInitPending) {
            // 反复尝试直到 video_container 有有效大小
            if (m_videoContainer) {
                RECT rc = m_videoContainer->GetPos();
                const int w = rc.right - rc.left;
                const int h = rc.bottom - rc.top;
                if (w > 10 && h > 10) {
                    HWND hwnd = GetVideoContainerHWND();
                    if (hwnd && m_videoRenderer) {
                        if (m_videoRenderer->Initialize(hwnd)) {
                            Log::Info("VideoRenderer initialized after layout: %dx%d", w, h);
                            m_videoInitPending = false;
                            ::KillTimer(m_hWnd, 2);
                        } else {
                            Log::Error("Failed to initialize VideoRenderer on hwnd: %p", hwnd);
                        }
                    }
                }
            }
        } else {
            ::KillTimer(m_hWnd, 2);
        }
        bHandled = TRUE;
        return 0;
    }
    // 自测相关 timer 已移除：不再自动最大化或自动退出
    // Timer 3/4/5 处理已禁用，避免意外触发
    if (uMsg == WM_APP + 101) {
        // duilib 布局完成后对齐视频子窗口
        // 强制触发一次绘制/布局更新，确保 NeedUpdate() 已经落实到控件位置
        ::UpdateWindow(m_hWnd);

        if (m_videoContainer && m_videoHwnd && ::IsWindow(m_videoHwnd)) {
            RECT rect = m_videoContainer->GetPos();
            ::SetWindowPos(
                m_videoHwnd,
                NULL,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

            RECT rcClient{};
            ::GetClientRect(m_videoHwnd, &rcClient);
            Log::Info("LayoutSync: video_container pos=%d,%d,%d,%d video_hwnd client=%dx%d",
                rect.left, rect.top, rect.right, rect.bottom,
                rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
        }
        // 尽快触发一次渲染，以便自测时立刻更新 RenderRect
        ::PostMessage(m_hWnd, WM_APP + 100, 0, 0);
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == WM_APP + 100) {
        if (m_videoRenderer) {
            m_videoRenderer->RenderPending();
        }
        bHandled = TRUE;
        return 0;
    }
    bHandled = FALSE;
    return 0;
}

void IJKPlayerWindow::OnOpenFile()
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    // 注意：filter 需要以双 '\0' 结尾
    ofn.lpstrFilter = "Media Files\0*.mp4;*.avi;*.mkv;*.flv;*.mov;*.wmv;*.mp3;*.wav;*.aac\0All Files\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        std::string filePath = szFile;
        m_playlistManager->AddItem(filePath);
        m_playlistManager->SetCurrentIndex(m_playlistManager->GetCount() - 1);
        UpdatePlaylistUI();
        
        // 按 SDK 示例：Prepare 是异步的，等 PREPARED 回调后再 Start
        m_autoPlayPending = false;
        if (m_playerController->OpenFile(filePath) && m_playerController->Prepare()) {
            m_currentFile = filePath;
            m_autoPlayPending = true;
            if (m_statusLabel) m_statusLabel->SetText(_T("Preparing..."));
        } else {
            if (m_statusLabel) m_statusLabel->SetText(_T("Open/Prepare failed"));
        }
    }
}

void IJKPlayerWindow::OnPlay()
{
    if (!m_isPlaying) {
        if (m_currentFile.empty()) {
            OnOpenFile();
            return;
        }
        
        if (m_playerController->Start()) {
            m_isPlaying = true;
            m_isPaused = false;
            if (m_playButton) m_playButton->SetVisible(false);
            if (m_pauseButton) m_pauseButton->SetVisible(true);
            StartUpdateTimer();
            if (m_statusLabel) m_statusLabel->SetText(_T("Playing"));
        } else {
            if (m_statusLabel) m_statusLabel->SetText(_T("Start failed"));
        }
    }
}

void IJKPlayerWindow::OnPause()
{
    if (m_playerController->Pause()) {
        m_isPlaying = false;
        m_isPaused = true;
        if (m_playButton) m_playButton->SetVisible(true);
        if (m_pauseButton) m_pauseButton->SetVisible(false);
        StopUpdateTimer();
    }
}

void IJKPlayerWindow::OnStop()
{
    if (m_playerController->Stop()) {
        m_isPlaying = false;
        m_isPaused = false;
        if (m_playButton) m_playButton->SetVisible(true);
        if (m_pauseButton) m_pauseButton->SetVisible(false);
        StopUpdateTimer();
        UpdateProgress();
    }
}

void IJKPlayerWindow::OnPrev()
{
    auto prevItem = m_playlistManager->GetPrevious();
    if (prevItem) {
        OnPlaylistItemSelected(m_playlistManager->GetCurrentIndex());
        OnPlay();
    }
}

void IJKPlayerWindow::OnNext()
{
    auto nextItem = m_playlistManager->GetNext();
    if (nextItem) {
        OnPlaylistItemSelected(m_playlistManager->GetCurrentIndex());
        OnPlay();
    }
}

void IJKPlayerWindow::OnFastBackward()
{
    if (m_playerController) {
        long currentPos = m_playerController->GetCurrentPosition();
        long newPos = currentPos - 10000; // 快退10秒
        if (newPos < 0) newPos = 0;
        m_playerController->SeekTo(newPos);
    }
}

void IJKPlayerWindow::OnFastForward()
{
    if (m_playerController) {
        long currentPos = m_playerController->GetCurrentPosition();
        long duration = m_playerController->GetDuration();
        long newPos = currentPos + 10000; // 快进10秒
        if (newPos > duration) newPos = duration;
        m_playerController->SeekTo(newPos);
    }
}

void IJKPlayerWindow::OnFullscreen()
{
    if (::IsZoomed(m_hWnd)) {
        ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    } else {
        ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }
}

void IJKPlayerWindow::OnSeek(int position)
{
    if (m_progressSlider && m_playerController) {
        long duration = m_playerController->GetDuration();
        if (duration > 0) {
            long seekPos = (long)((double)position / 100.0 * duration);
            m_playerController->SeekTo(seekPos);
        }
    }
}

void IJKPlayerWindow::OnVolumeChanged(int volume)
{
    if (m_playerController) {
        m_playerController->SetVolume((float)volume);
    }
}

void IJKPlayerWindow::OnPlaylistItemSelected(int index)
{
    if (index >= 0 && index < (int)m_playlistManager->GetCount()) {
        m_playlistManager->SetCurrentIndex(index);
        auto item = m_playlistManager->GetCurrentItem();
        if (item) {
            m_currentFile = item->filePath;
            if (m_playerController->OpenFile(item->filePath)) {
                if (m_playerController->Prepare()) {
                    if (m_statusLabel) {
                        m_statusLabel->SetText(CDuiString(item->fileName.c_str()));
                    }
                }
            }
        }
    }
}

void IJKPlayerWindow::OnAddToPlaylist()
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Media Files\0*.mp4;*.avi;*.mkv;*.flv;*.mov;*.wmv;*.mp3;*.wav;*.aac\0All Files\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

    if (GetOpenFileNameA(&ofn)) {
        std::string filePath = szFile;
        m_playlistManager->AddItem(filePath);
        UpdatePlaylistUI();
    }
}

void IJKPlayerWindow::OnPlayerStateChanged(IjkMsgState state, int arg1, int arg2)
{
    switch (state) {
    case IJK_MSG_PREPARED:
        Log::Info("Player prepared");
        if (m_statusLabel) m_statusLabel->SetText(_T("Prepared"));
        if (m_autoPlayPending) {
            m_autoPlayPending = false;
            OnPlay();
        }
        break;
    case IJK_MSG_COMPLETED: {
        Log::Info("Playback completed");
        m_isPlaying = false;
        if (m_playButton) m_playButton->SetVisible(true);
        if (m_pauseButton) m_pauseButton->SetVisible(false);
        StopUpdateTimer();
        
        // 播放下一首
        auto nextItem = m_playlistManager->GetNext();
        if (nextItem) {
            OnPlaylistItemSelected(m_playlistManager->GetCurrentIndex());
            OnPlay();
        }
        break;
    }
    case IJK_MSG_ERROR:
        Log::Error("Player error: %d, %d", arg1, arg2);
        if (m_statusLabel) {
            m_statusLabel->SetText(_T("Error"));
        }
        break;
    default:
        break;
    }
}

void IJKPlayerWindow::OnVideoFrame(IjkVideoFrame* frame)
{
    if (!m_videoRenderer || !frame) {
        return;
    }

    // 解码线程：深拷贝帧并通知 UI 线程渲染，避免 SDL 跨线程导致堆损坏
    m_videoRenderer->SubmitFrame(frame);
    ::PostMessage(m_hWnd, WM_APP + 100, 0, 0);
}

void IJKPlayerWindow::StartUpdateTimer()
{
    if (m_updateTimer == 0) {
        m_updateTimer = ::SetTimer(m_hWnd, 1, 100, NULL); // 100ms更新一次
    }
}

void IJKPlayerWindow::StopUpdateTimer()
{
    if (m_updateTimer != 0) {
        ::KillTimer(m_hWnd, m_updateTimer);
        m_updateTimer = 0;
    }
}

void IJKPlayerWindow::UpdateProgress()
{
    if (!m_playerController || !m_progressSlider || !m_timeLabel) {
        return;
    }

    long current = m_playerController->GetCurrentPosition();
    long duration = m_playerController->GetDuration();

    if (duration > 0) {
        int progress = (int)((double)current / duration * 100);
        m_progressSlider->SetValue(progress);
        
        std::string timeStr = FormatTime(current) + " / " + FormatTime(duration);
        m_timeLabel->SetText(CDuiString(timeStr.c_str()));
    } else {
        m_progressSlider->SetValue(0);
        m_timeLabel->SetText(_T("00:00 / 00:00"));
    }
}

void IJKPlayerWindow::UpdateStatus()
{
    UpdateProgress();
}

void IJKPlayerWindow::UpdatePlaylistUI()
{
    if (!m_playlistList) {
        return;
    }

    m_playlistList->RemoveAll();
    
    for (size_t i = 0; i < m_playlistManager->GetCount(); ++i) {
        auto item = m_playlistManager->GetItem(i);
        if (item) {
            CListTextElementUI* pListElement = new CListTextElementUI;
            pListElement->SetText(0, item->fileName.c_str());
            pListElement->SetText(1, item->filePath.c_str());
            m_playlistList->Add(pListElement);
        }
    }
}

std::string IJKPlayerWindow::FormatTime(long ms)
{
    long seconds = ms / 1000;
    long minutes = seconds / 60;
    seconds = seconds % 60;
    long hours = minutes / 60;
    minutes = minutes % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << std::setfill('0') << std::setw(2) << hours << ":";
    }
    oss << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

