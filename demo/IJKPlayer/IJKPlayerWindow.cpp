#include "IJKPlayerWindow.h"
#include "NetworkStreamDialog.h"
#include "Control/UIButton.h"
#include "Control/UISlider.h"
#include "Control/UILabel.h"
#include "Control/UIList.h"
#include "Control/UITreeView.h"
#include "Core/UIManager.h"
#include "logging.h"
#include <commdlg.h>
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <io.h>
#include <vector>
#include <algorithm>
#include <stdint.h>

// 控件ID定义
const TCHAR* const IJKPlayerWindow::kVideoContainer = _T("video_container");
const TCHAR* const IJKPlayerWindow::kVideoBackground = _T("video_background");
const TCHAR* const IJKPlayerWindow::kTitleBar = _T("title");
const TCHAR* const IJKPlayerWindow::kControlPanel = _T("control_panel");
const TCHAR* const IJKPlayerWindow::kPlayButton = _T("btn_play");
const TCHAR* const IJKPlayerWindow::kPauseButton = _T("btn_pause");
const TCHAR* const IJKPlayerWindow::kStopButton = _T("btn_stop");
const TCHAR* const IJKPlayerWindow::kPrevButton = _T("btn_prev");
const TCHAR* const IJKPlayerWindow::kNextButton = _T("btn_next");
// 注意：res/IJKPlayer.xml 里是 btnFastBackward / btnFastForward
const TCHAR* const IJKPlayerWindow::kFastBackwardButton = _T("btnFastBackward");
const TCHAR* const IJKPlayerWindow::kFastForwardButton = _T("btnFastForward");
const TCHAR* const IJKPlayerWindow::kFullscreenButton = _T("btn_fullscreen");
const TCHAR* const IJKPlayerWindow::kProgressSlider = _T("slider_progress");
const TCHAR* const IJKPlayerWindow::kVolumeSlider = _T("slider_volume");
const TCHAR* const IJKPlayerWindow::kTimeLabel = _T("label_time");
const TCHAR* const IJKPlayerWindow::kStatusLabel = _T("label_status");
// res/IJKPlayer.xml 里播放列表是 TreeView name="treePlaylist"
const TCHAR* const IJKPlayerWindow::kPlaylistList = _T("treePlaylist");
const TCHAR* const IJKPlayerWindow::kVolumeButton = _T("btn_volume");
const TCHAR* const IJKPlayerWindow::kVolumeZeroButton = _T("btn_volume_zero");
const TCHAR* const IJKPlayerWindow::kOpenMiniButton = _T("btn_open_mini");
const TCHAR* const IJKPlayerWindow::kPlaylistShowButton = _T("btnPlaylistShow");
const TCHAR* const IJKPlayerWindow::kPlaylistHideButton = _T("btnPlaylistHide");
const TCHAR* const IJKPlayerWindow::kScreenNormalButton = _T("btn_screen_normal");
const TCHAR* const IJKPlayerWindow::kSideHideButton = _T("btnSideHide");
const TCHAR* const IJKPlayerWindow::kSideShowButton = _T("btnSideShow");
const TCHAR* const IJKPlayerWindow::kPlaylistPanel = _T("playlist_panel");

namespace {

static std::string WideToMultiByte(UINT codepage, const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(codepage, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    if (len <= 0) return {};
    std::string out;
    out.resize((size_t)len);
    // 注意：部分 MSVC 标准库下 std::string::data() 返回 const char*；用 &out[0] 获取可写缓冲区
    WideCharToMultiByte(codepage, 0, w.c_str(), (int)w.size(), &out[0], len, NULL, NULL);
    return out;
}

// 本工程默认是 MBCS（见根目录 CMakeLists.txt 的 -D_MBCS），UI/duilib 使用系统代码页(ACP)显示。
// 因此：本地文件路径/文件名要用 ACP，而不是 UTF-8。
static std::string WideToAcp(const std::wstring& w) { return WideToMultiByte(CP_ACP, w); }

bool BrowseForFolder(HWND owner, std::wstring& outFolder)
{
    BROWSEINFOW bi{};
    bi.hwndOwner = owner;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = L"请选择要导入的文件夹";

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return false;

    wchar_t path[MAX_PATH]{};
    bool ok = (SHGetPathFromIDListW(pidl, path) == TRUE);
    CoTaskMemFree(pidl);

    if (!ok || path[0] == L'\0') return false;
    outFolder = path;
    return true;
}

bool IsMediaFileExt(const std::wstring& extLower)
{
    // 与打开文件的 filter 保持一致（不区分大小写）
    static const wchar_t* kExts[] = {
        L".mp4", L".avi", L".mkv", L".flv", L".mov", L".wmv", L".mp3", L".wav", L".aac"
    };
    for (auto* e : kExts) {
        if (extLower == e) return true;
    }
    return false;
}

bool EnumerateMediaFilesInFolder(const std::wstring& folder, std::vector<std::wstring>& outFiles)
{
    outFiles.clear();
    if (folder.empty()) return false;

    std::wstring pattern = folder;
    if (!pattern.empty() && pattern.back() != L'\\' && pattern.back() != L'/') pattern += L'\\';
    pattern += L"*";

    WIN32_FIND_DATAW ffd{};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        const wchar_t* name = ffd.cFileName;
        if (!name || name[0] == L'\0') continue;
        if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0) continue;

        const bool isDir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (isDir) continue; // 这里只导入当前目录，不递归

        std::wstring fileName(name);
        std::wstring fullPath = folder;
        if (!fullPath.empty() && fullPath.back() != L'\\' && fullPath.back() != L'/') fullPath += L'\\';
        fullPath += fileName;

        // ext lower
        std::wstring ext;
        size_t dot = fileName.find_last_of(L'.');
        if (dot != std::wstring::npos) ext = fileName.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) { return (wchar_t)towlower(c); });
        if (!IsMediaFileExt(ext)) continue;

        outFiles.push_back(fullPath);
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
    return true;
}

} // namespace

IJKPlayerWindow::IJKPlayerWindow()
    : m_videoContainer(nullptr)
    , m_videoBackground(nullptr)
    , m_titleBar(nullptr)
    , m_controlPanel(nullptr)
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_stopButton(nullptr)
    , m_prevButton(nullptr)
    , m_nextButton(nullptr)
    , m_fastBackwardButton(nullptr)
    , m_fastForwardButton(nullptr)
    , m_fullscreenButton(nullptr)
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
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_updateTimer(0)
    , m_videoHwnd(nullptr)
    , m_autoPlayPending(false)
    , m_videoInitPending(false)
    , m_lastVolumeBeforeMute(50)
    , m_videoFullscreen(false)
    , m_prevStyle(0)
    , m_prevExStyle(0)
    , m_prevTitleVisible(true)
    , m_prevControlVisible(true)
    , m_prevPlaylistVisible(true)
    , m_mediaInfoVisible(false)
    , m_mediaInfoHwnd(nullptr)
    , m_videoOldProc(nullptr)
{
    ZeroMemory(&m_prevPlacement, sizeof(m_prevPlacement));
    m_prevPlacement.length = sizeof(m_prevPlacement);
    m_playerController = std::make_unique<PlayerController>();
    m_videoRenderer = std::make_unique<VideoRenderer>();
    m_playlistManager = std::make_unique<PlaylistManager>();
}

IJKPlayerWindow::~IJKPlayerWindow()
{
    // 智能指针会自动释放资源，不需要手动调用Release()
    // 确保视频窗口先被销毁
    if (m_videoHwnd && IsWindow(m_videoHwnd)) {
        if (m_videoOldProc) {
            ::SetWindowLongPtr(m_videoHwnd, GWLP_WNDPROC, (LONG_PTR)m_videoOldProc);
            m_videoOldProc = nullptr;
        }
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

    // 自动播放功能已禁用：不再自动播放测试文件
    /*if (m_currentFile.empty() && _access("./test.flv", 0) == 0) {
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
    }*/
}

void IJKPlayerWindow::SetupUI()
{
    // 获取UI控件指针
    m_videoContainer = m_pm.FindControl(kVideoContainer);
    m_videoBackground = static_cast<CButtonUI*>(m_pm.FindControl(kVideoBackground));
    m_titleBar = m_pm.FindControl(kTitleBar);
    m_controlPanel = m_pm.FindControl(kControlPanel);
    m_playButton = static_cast<CButtonUI*>(m_pm.FindControl(kPlayButton));
    m_pauseButton = static_cast<CButtonUI*>(m_pm.FindControl(kPauseButton));
    m_stopButton = static_cast<CButtonUI*>(m_pm.FindControl(kStopButton));
    m_prevButton = static_cast<CButtonUI*>(m_pm.FindControl(kPrevButton));
    m_nextButton = static_cast<CButtonUI*>(m_pm.FindControl(kNextButton));
    m_fastBackwardButton = static_cast<CButtonUI*>(m_pm.FindControl(kFastBackwardButton));
    m_fastForwardButton = static_cast<CButtonUI*>(m_pm.FindControl(kFastForwardButton));
    m_fullscreenButton = static_cast<CButtonUI*>(m_pm.FindControl(kFullscreenButton));
    m_screenNormalButton = static_cast<CButtonUI*>(m_pm.FindControl(kScreenNormalButton));
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

    // 设置视频窗口背景图片（未播放时显示）
    // 背景图片通过 video_background Button 显示，初始状态为可见
    if (m_videoBackground) {
        m_videoBackground->SetVisible(true);
    }
    // 初始状态隐藏视频窗口，让背景图片显示
    if (m_videoHwnd && IsWindow(m_videoHwnd)) {
        ::ShowWindow(m_videoHwnd, SW_HIDE);
    }

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
        // 默认值以 XML 为准；同时记录一份用于“取消静音”的恢复
        m_lastVolumeBeforeMute = m_volumeSlider->GetValue();
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
    if (m_volumeSlider) {
        m_playerController->SetVolume((float)m_volumeSlider->GetValue());
    } else {
        m_playerController->SetVolume(50.0f);
    }
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
        // 注册一个带 CS_DBLCLKS 的窗口类，用于接收双击消息
        static bool s_clsReg = false;
        static const TCHAR* kVideoHostClass = _T("IJKVideoHostWindow");
        if (!s_clsReg) {
            WNDCLASS wc{};
            wc.style = CS_DBLCLKS;
            wc.lpfnWndProc = ::DefWindowProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = kVideoHostClass;
            wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
            ::RegisterClass(&wc);
            s_clsReg = true;
        }

        m_videoHwnd = CreateWindow(
            kVideoHostClass,
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
            // 绑定子窗口过程，捕获双击
            ::SetWindowLongPtr(m_videoHwnd, GWLP_USERDATA, (LONG_PTR)this);
            m_videoOldProc = (WNDPROC)::SetWindowLongPtr(m_videoHwnd, GWLP_WNDPROC, (LONG_PTR)&IJKPlayerWindow::VideoHostWndProc);
            
            // 注册到duilib的原生窗口管理
            m_pm.AddNativeWindow(m_videoContainer, m_videoHwnd);
            
            Log::Info("Created video hwnd: %p, pos: %d,%d,%d,%d", 
                     m_videoHwnd, rect.left, rect.top, rect.right, rect.bottom);

            // 创建媒体信息叠加层（原生半透明窗口，覆盖在视频 HWND 之上）
            EnsureMediaInfoOverlay();
            SyncMediaInfoOverlayPos();
        }
    } else {
        // 更新窗口位置和大小
        SetWindowPos(m_videoHwnd, NULL, rect.left, rect.top, 
                     rect.right - rect.left, rect.bottom - rect.top,
                     SWP_NOZORDER | SWP_SHOWWINDOW);
        SyncMediaInfoOverlayPos();
    }

    return m_videoHwnd;
}

LRESULT CALLBACK IJKPlayerWindow::VideoHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<IJKPlayerWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (self) {
        if (uMsg == WM_LBUTTONDBLCLK) {
            self->ToggleVideoFullscreen();
            return 0;
        }
        // 处理空格键切换播放/暂停（当视频窗口有焦点时）
        if (uMsg == WM_KEYDOWN && wParam == VK_SPACE) {
            if (self->m_isPlaying) {
                self->OnPause();
            } else {
                self->OnPlay();
            }
            return 0;
        }
        // 处理左右方向键切换上一首/下一首（当视频窗口有焦点时）
        if (uMsg == WM_KEYDOWN) {
            if (wParam == VK_LEFT) {
                self->OnPrev();
                return 0;
            } else if (wParam == VK_RIGHT) {
                self->OnNext();
                return 0;
            } else if (wParam == 'I' || wParam == 'i') {
                // I键切换媒体信息显示
                self->ToggleMediaInfo();
                return 0;
            }
        }
        if (self->m_videoOldProc) {
            return ::CallWindowProc(self->m_videoOldProc, hWnd, uMsg, wParam, lParam);
        }
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void IJKPlayerWindow::EnsureMediaInfoOverlay()
{
    if (m_mediaInfoHwnd && ::IsWindow(m_mediaInfoHwnd)) {
        return;
    }

    static bool s_clsReg = false;
    static const TCHAR* kOverlayClass = _T("IJKMediaInfoOverlayWindow");
    if (!s_clsReg) {
        WNDCLASS wc{};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &IJKPlayerWindow::MediaInfoOverlayWndProc;
        wc.hInstance = ::GetModuleHandle(NULL);
        wc.lpszClassName = kOverlayClass;
        wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
        ::RegisterClass(&wc);
        s_clsReg = true;
    }

    // WS_POPUP + owner = 主窗口，保证总在主窗口之上；WS_EX_TRANSPARENT 让鼠标穿透到视频窗口
    m_mediaInfoHwnd = ::CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        kOverlayClass,
        _T(""),
        WS_POPUP,
        0, 0, 10, 10,
        m_hWnd,
        NULL,
        ::GetModuleHandle(NULL),
        NULL);

    if (m_mediaInfoHwnd) {
        ::SetWindowLongPtr(m_mediaInfoHwnd, GWLP_USERDATA, (LONG_PTR)this);
        // 全窗口透明度（0~255）。值越小越透明
        ::SetLayeredWindowAttributes(m_mediaInfoHwnd, 0, 170, LWA_ALPHA);
        ::ShowWindow(m_mediaInfoHwnd, SW_HIDE);
    }
}

void IJKPlayerWindow::SyncMediaInfoOverlayPos()
{
    if (!m_mediaInfoVisible) {
        if (m_mediaInfoHwnd && ::IsWindow(m_mediaInfoHwnd)) {
            ::ShowWindow(m_mediaInfoHwnd, SW_HIDE);
        }
        return;
    }

    EnsureMediaInfoOverlay();
    if (!m_mediaInfoHwnd || !::IsWindow(m_mediaInfoHwnd) || !m_videoContainer) {
        return;
    }

    // 注意：WS_POPUP 的窗口坐标是“屏幕坐标”，而 duilib GetPos() 是“主窗口 client 坐标”
    // 所以这里需要把 video_container 的 client 坐标转换为屏幕坐标，才能真正贴到右上角。
    RECT rcClient = m_videoContainer->GetPos();

    POINT ptScreen{ rcClient.left, rcClient.top };
    ::ClientToScreen(m_hWnd, &ptScreen);

    // 叠加层固定在视频区域右上角（并做边界保护，避免视频区域太小导致 x 变成负数）
    const int w = 320;
    const int h = 210;
    const int margin = 10;
    const int areaW = rcClient.right - rcClient.left;
    int xLocal = areaW - w - margin;
    if (xLocal < margin) xLocal = margin;
    const int x = ptScreen.x + xLocal;
    const int y = ptScreen.y + margin;

    ::SetWindowPos(
        m_mediaInfoHwnd,
        HWND_TOPMOST,
        x, y, w, h,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

LRESULT CALLBACK IJKPlayerWindow::MediaInfoOverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<IJKPlayerWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (uMsg) {
    case WM_NCHITTEST:
        // 鼠标穿透
        return HTTRANSPARENT;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = ::BeginPaint(hWnd, &ps);

        RECT rc{};
        ::GetClientRect(hWnd, &rc);
        HBRUSH br = ::CreateSolidBrush(RGB(0, 0, 0));
        ::FillRect(hdc, &rc, br);
        ::DeleteObject(br);

        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, RGB(255, 255, 255));

        HFONT hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
        HFONT hOld = (HFONT)::SelectObject(hdc, hFont);

        RECT textRc = rc;
        textRc.left += 10;
        textRc.top += 8;
        textRc.right -= 10;
        textRc.bottom -= 8;

        const char* text = (self ? self->m_mediaInfoText.c_str() : "");
        ::DrawTextA(hdc, text, -1, &textRc, DT_LEFT | DT_TOP | DT_WORDBREAK);

        ::SelectObject(hdc, hOld);
        ::EndPaint(hWnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void IJKPlayerWindow::ToggleVideoFullscreen()
{
    if (m_videoFullscreen) ExitVideoFullscreen();
    else EnterVideoFullscreen();
}

void IJKPlayerWindow::SetChromeVisible(bool visible)
{
    if (m_titleBar) m_titleBar->SetVisible(visible);
    if (m_controlPanel) m_controlPanel->SetVisible(visible);
    if (m_playlistPanel) m_playlistPanel->SetVisible(visible);
    ::PostMessage(m_hWnd, WM_APP + 101, 0, 0);
}

void IJKPlayerWindow::EnterVideoFullscreen()
{
    if (m_videoFullscreen) return;
    m_videoFullscreen = true;

    m_prevStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
    m_prevExStyle = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
    ::GetWindowPlacement(m_hWnd, &m_prevPlacement);

    m_prevTitleVisible = m_titleBar ? m_titleBar->IsVisible() : true;
    m_prevControlVisible = m_controlPanel ? m_controlPanel->IsVisible() : true;
    m_prevPlaylistVisible = m_playlistPanel ? m_playlistPanel->IsVisible() : true;

    SetChromeVisible(false);

    // 无边框全屏：覆盖当前显示器工作区/显示区
    HMONITOR hMon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    ::GetMonitorInfo(hMon, &mi);
    RECT rc = mi.rcMonitor;

    LONG_PTR style = m_prevStyle;
    style &= ~(WS_CAPTION | WS_THICKFRAME);
    style |= WS_POPUP;
    ::SetWindowLongPtr(m_hWnd, GWL_STYLE, style);

    LONG_PTR exStyle = m_prevExStyle;
    exStyle &= ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);
    ::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exStyle);

    ::SetWindowPos(m_hWnd, HWND_TOP,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void IJKPlayerWindow::ExitVideoFullscreen()
{
    if (!m_videoFullscreen) return;
    m_videoFullscreen = false;

    ::SetWindowLongPtr(m_hWnd, GWL_STYLE, m_prevStyle);
    ::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_prevExStyle);
    ::SetWindowPlacement(m_hWnd, &m_prevPlacement);
    ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    if (m_titleBar) m_titleBar->SetVisible(m_prevTitleVisible);
    if (m_controlPanel) m_controlPanel->SetVisible(m_prevControlVisible);
    if (m_playlistPanel) m_playlistPanel->SetVisible(m_prevPlaylistVisible);
    ::PostMessage(m_hWnd, WM_APP + 101, 0, 0);
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

        // “全屏”按钮：用主窗口最大化/还原实现（嵌入 HWND 的 SDL_Window 不适合 SDL_SetWindowFullscreen）
        else if (name == kFullscreenButton) {
            OnFullscreen();
            return;
        }
        else if (name == kScreenNormalButton) {
            OnScreenNormal();
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
        else if (name == kVolumeButton) {
            // 点击音量按钮切换到静音状态
            if (m_volumeButton && m_volumeZeroButton) {
                if (m_volumeSlider) {
                    int v = m_volumeSlider->GetValue();
                    if (v > 0) m_lastVolumeBeforeMute = v;
                }
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
                    int restore = (m_lastVolumeBeforeMute > 0) ? m_lastVolumeBeforeMute : 50;
                    m_playerController->SetVolume((float)restore);
                    if (m_volumeSlider) m_volumeSlider->SetValue(restore);
                }
            }
        }
        else if (name == kOpenMiniButton) {
            OnOpenFile();
        }
        else if (name == _T("logo")) {
            // 点击logo显示下拉菜单
            POINT pt = { msg.ptMouse.x, msg.ptMouse.y };
            CDuiRect rc = msg.pSender->GetPos();
            pt.x = rc.left;
            pt.y = rc.bottom;
            
            // 创建并显示自定义菜单窗口
            // 每次都创建新实例，避免生命周期管理问题
            MenuWnd* pMenuWnd = new MenuWnd(_T("menu.xml"));
            if (pMenuWnd != nullptr) {
                pMenuWnd->Init(&m_pm, pt);
                pMenuWnd->ShowWindow(true);
            }
        }
        else if (name == kPlaylistShowButton) {
            // 显示播放列表
            SetPlaylistVisible(true);
        }
        else if (name == kPlaylistHideButton) {
            // 隐藏播放列表
            SetPlaylistVisible(false);
        }
        else if (name == kSideHideButton) {
            // 隐藏播放列表
            SetPlaylistVisible(false);
        }
        else if (name == kSideShowButton) {
            // 显示播放列表
            SetPlaylistVisible(true);
        }
        else if (name == _T("btn_open")) {
            OnOpenFile();
        }
        // 处理菜单点击事件
        else if (name == _T("menu_OpenFile")) {
            OnOpenFile();
        }
        else if (name == _T("menu_OpenFolder")) {
            OnOpenFolder();
        }
        else if (name == _T("menu_OpenNetworkStream")) {
            OnOpenNetworkStream();
        }
        else if (name == _T("menu_NoFrame")) {
            OnToggleNoFrame();
        }
        else if (name == _T("menu_SetFullScreen")) {
            OnFullscreen();
        }
        else if (name == _T("menu_ExitFullScreen")) {
            OnScreenNormal();
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
    else if (msg.sType == DUI_MSGTYPE_ITEMCLICK || msg.sType == DUI_MSGTYPE_ITEMACTIVATE) {
        // TreeView 的条目也是 ListItem，点击/双击都会从 item 发送通知；通过 owner 判断是否来自播放列表
        IListItemUI* pItem = static_cast<IListItemUI*>(msg.pSender->GetInterface(DUI_CTR_LISTITEM));
        if (pItem && m_playlistList && pItem->GetOwner() == m_playlistList) {
            int index = pItem->GetIndex();
            OnPlaylistItemSelected(index);
            if (msg.sType == DUI_MSGTYPE_ITEMACTIVATE) {
                // 双击/回车直接播放
                m_autoPlayPending = true;
            }
        }
    }
}

LRESULT IJKPlayerWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    StopUpdateTimer();
    
    // 停止定时器2（用于视频初始化）
    ::KillTimer(m_hWnd, 2);
    
    if (m_playerController) {
        m_playerController->Stop();
    }
    
    // 允许默认关闭流程继续，触发WM_DESTROY
    bHandled = FALSE;
    return 0;
}

LRESULT IJKPlayerWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // 释放所有资源
    StopUpdateTimer();
    
    // 停止定时器2（用于视频初始化）
    ::KillTimer(m_hWnd, 2);
    
    // 先停止播放器
    if (m_playerController) {
        m_playerController->Stop();
    }
    
    // 销毁视频渲染窗口（必须在释放渲染器之前）
    if (m_videoHwnd && IsWindow(m_videoHwnd)) {
        m_pm.RemoveNativeWindow(m_videoHwnd);
        DestroyWindow(m_videoHwnd);
        m_videoHwnd = nullptr;
    }

    // 销毁媒体信息叠加层窗口
    if (m_mediaInfoHwnd && ::IsWindow(m_mediaInfoHwnd)) {
        ::DestroyWindow(m_mediaInfoHwnd);
        m_mediaInfoHwnd = nullptr;
    }
    
    // 智能指针会在析构时自动释放资源，不需要手动调用Release()
    // 调用基类的OnDestroy方法
    bHandled = FALSE;
    return 0;
}

void IJKPlayerWindow::OnFinalMessage(HWND hWnd)
{
    Log::Info("OnFinalMessage: Start");
    
    // 确保所有定时器都被停止 - 使用传入的hWnd参数而不是m_hWnd
    StopUpdateTimer();
    ::KillTimer(hWnd, 1);
    ::KillTimer(hWnd, 2);
    
    // 确保视频窗口已经销毁
    if (m_videoHwnd && IsWindow(m_videoHwnd)) {
        Log::Info("OnFinalMessage: Destroying video hwnd: %p", m_videoHwnd);
        m_pm.RemoveNativeWindow(m_videoHwnd);
        DestroyWindow(m_videoHwnd);
        m_videoHwnd = nullptr;
    }

    // 确保媒体信息叠加层窗口已经销毁
    if (m_mediaInfoHwnd && ::IsWindow(m_mediaInfoHwnd)) {
        Log::Info("OnFinalMessage: Destroying media info overlay hwnd: %p", m_mediaInfoHwnd);
        ::DestroyWindow(m_mediaInfoHwnd);
        m_mediaInfoHwnd = nullptr;
    }
    
    // 彻底停止播放器和解码器
    if (m_playerController) {
        Log::Info("OnFinalMessage: Stopping player controller");
        m_playerController->Stop();
        // 清除智能指针，强制释放资源
        m_playerController.reset();
    }
    
    // 确保视频渲染器资源被释放
    if (m_videoRenderer) {
        Log::Info("OnFinalMessage: Releasing video renderer");
        m_videoRenderer.reset();
    }
    
    // 调用基类的OnFinalMessage方法
    Log::Info("OnFinalMessage: Calling base class");
    WindowImplBase::OnFinalMessage(hWnd);
    
    // 强制退出进程，确保不会有残留线程
    Log::Info("OnFinalMessage: Exiting process");
    ::ExitProcess(0);
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
    } else if (id == 2001) {
        OnOpenFile();
        bHandled = TRUE;
        return 0;
    } else if (id == 2002) {
        OnOpenFolder();
        bHandled = TRUE;
        return 0;
    } else if (id == 2003) {
        OnOpenNetworkStream();
        bHandled = TRUE;
        return 0;
    } else if (id == 2004) {
        OnToggleNoFrame();
        bHandled = TRUE;
        return 0;
    } else if (id == 2005) {
        OnFullscreen();
        bHandled = TRUE;
        return 0;
    } else if (id == 2006) {
        OnScreenNormal();
        bHandled = TRUE;
        return 0;
    } else if (id == 2007) {
        OnAddToPlaylist();
        bHandled = TRUE;
        return 0;
    } else if (id == 2008) {
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
    // 同步全屏/退出全屏按钮
    const bool zoomed = ::IsZoomed(m_hWnd) != FALSE;
    if (m_fullscreenButton) m_fullscreenButton->SetVisible(!zoomed);
    if (m_screenNormalButton) m_screenNormalButton->SetVisible(zoomed);
    return lRes;
}

LRESULT IJKPlayerWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // 处理从 MenuWnd 投递过来的命令（以及其他 WM_COMMAND）
    if (uMsg == WM_COMMAND) {
        BOOL handled = FALSE;
        LRESULT ret = OnCommand(uMsg, wParam, lParam, handled);
        bHandled = handled;
        return ret;
    }
    // 处理空格键切换播放/暂停
    if (uMsg == WM_KEYDOWN && wParam == VK_SPACE) {
        if (m_isPlaying) {
            OnPause();
        } else {
            OnPlay();
        }
        bHandled = TRUE;
        return 0;
    }
    // 处理左右方向键切换上一首/下一首
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_LEFT) {
            OnPrev();
            bHandled = TRUE;
            return 0;
        } else if (wParam == VK_RIGHT) {
            OnNext();
            bHandled = TRUE;
            return 0;
        } else if (wParam == 'I' || wParam == 'i') {
            // I键切换媒体信息显示
            ToggleMediaInfo();
            bHandled = TRUE;
            return 0;
        }
    }
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
                            
                            // 初始状态隐藏视频窗口，让背景图片显示
                            if (!m_isPlaying) {
                                ::ShowWindow(hwnd, SW_HIDE);
                            }
                            
                            // 初始化完成后立即同步一次窗口大小
                            ::PostMessage(m_hWnd, WM_APP + 101, 0, 0);
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
            
            // 确保视频渲染窗口始终可见
            if (!::IsWindowVisible(m_videoHwnd)) {
                ::ShowWindow(m_videoHwnd, SW_SHOW);
            }
            
            // 更新视频渲染窗口的位置和大小
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
        // 同步媒体信息叠加层位置（始终覆盖在视频窗口之上）
        SyncMediaInfoOverlayPos();
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
    // 检查必要的指针是否为空
    if (!m_playlistManager || !m_playerController) {
        if (m_statusLabel) m_statusLabel->SetText(_T("Internal error: missing components"));
        return;
    }

    OPENFILENAMEW ofn;
    wchar_t szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    // 注意：filter 需要以双 '\0' 结尾
    ofn.lpstrFilter = L"Media Files\0*.mp4;*.avi;*.mkv;*.flv;*.mov;*.wmv;*.mp3;*.wav;*.aac\0All Files\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring filePathW = szFile;
        std::string filePath = WideToAcp(filePathW);
        if (filePath.empty()) {
            if (m_statusLabel) m_statusLabel->SetText(_T("Invalid path"));
            return;
        }
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
            // 开始播放时隐藏背景图片，显示视频窗口
            if (m_videoBackground) {
                m_videoBackground->SetVisible(false);
            }
            if (m_videoHwnd && IsWindow(m_videoHwnd)) {
                ::ShowWindow(m_videoHwnd, SW_SHOW);
            }
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
        // 停止播放时显示背景图片，隐藏视频窗口
        if (m_videoBackground) {
            m_videoBackground->SetVisible(true);
        }
        if (m_videoHwnd && IsWindow(m_videoHwnd)) {
            ::ShowWindow(m_videoHwnd, SW_HIDE);
        }
    }
}

void IJKPlayerWindow::OnPrev()
{
    auto prevItem = m_playlistManager->GetPrevious();
    if (prevItem) {
        OnPlaylistItemSelected(m_playlistManager->GetCurrentIndex());
        m_autoPlayPending = true;
    }
}

void IJKPlayerWindow::OnNext()
{
    auto nextItem = m_playlistManager->GetNext();
    if (nextItem) {
        OnPlaylistItemSelected(m_playlistManager->GetCurrentIndex());
        m_autoPlayPending = true;
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
            // slider_progress 取值范围是 0~1000
            const int maxV = m_progressSlider->GetMaxValue();
            const double denom = (maxV > 0) ? (double)maxV : 1000.0;
            long seekPos = (long)((double)position / denom * (double)duration);
            m_playerController->SeekTo(seekPos);
            UpdateProgress();
        }
    }
}

void IJKPlayerWindow::OnVolumeChanged(int volume)
{
    if (m_playerController) {
        m_playerController->SetVolume((float)volume);
        if (volume <= 0) {
            if (m_volumeButton) m_volumeButton->SetVisible(false);
            if (m_volumeZeroButton) m_volumeZeroButton->SetVisible(true);
        } else {
            m_lastVolumeBeforeMute = volume;
            if (m_volumeButton) m_volumeButton->SetVisible(true);
            if (m_volumeZeroButton) m_volumeZeroButton->SetVisible(false);
        }
    }
}

void IJKPlayerWindow::OnPlaylistItemSelected(int index)
{
    if (index >= 0 && index < (int)m_playlistManager->GetCount()) {
        m_playlistManager->SetCurrentIndex(index);
        
        // 同步更新播放列表 UI 选中状态
        if (m_playlistList) {
            m_playlistList->SelectItem(index, false);
        }
        
        auto item = m_playlistManager->GetCurrentItem();
        if (item) {
            m_currentFile = item->filePath;
            // 切换媒体前先 stop，避免前一个播放线程/音视频设备占用
            if (m_playerController) {
                m_playerController->Stop();
            }
            // 停止后显示背景图片，隐藏视频窗口
            // 但如果即将自动播放（m_autoPlayPending 为 true），则不显示背景图片
            // 这样在 PREPARED 回调时，背景图片已经隐藏了
            if (!m_autoPlayPending) {
                if (m_videoBackground) {
                    m_videoBackground->SetVisible(true);
                }
                if (m_videoHwnd && IsWindow(m_videoHwnd)) {
                    ::ShowWindow(m_videoHwnd, SW_HIDE);
                }
            } else {
                // 即将自动播放，保持背景图片隐藏，视频窗口显示
                if (m_videoBackground) {
                    m_videoBackground->SetVisible(false);
                }
                if (m_videoHwnd && IsWindow(m_videoHwnd)) {
                    ::ShowWindow(m_videoHwnd, SW_SHOW);
                }
            }
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
    if (!m_playlistManager) return;

    OPENFILENAMEW ofn{};
    // 多选时返回缓冲区格式为：dir\0file1\0file2\0...\0\0
    std::vector<wchar_t> buffer;
    buffer.resize(64 * 1024, L'\0');

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = (DWORD)buffer.size();
    ofn.lpstrFilter = L"Media Files\0*.mp4;*.avi;*.mkv;*.flv;*.mov;*.wmv;*.mp3;*.wav;*.aac\0All Files\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (!GetOpenFileNameW(&ofn)) return;

    const wchar_t* p = buffer.data();
    std::wstring dir = p;
    p += dir.size() + 1;

    // 只选了一个文件：buffer 里直接是完整路径，后面紧跟 \0\0
    if (*p == L'\0') {
        std::string one = WideToAcp(dir);
        if (!one.empty()) m_playlistManager->AddItem(one);
        UpdatePlaylistUI();
        return;
    }

    // 多选：dir + 多个文件名
    size_t added = 0;
    while (*p) {
        std::wstring fileName = p;
        p += fileName.size() + 1;

        std::wstring fullPath = dir;
        if (!fullPath.empty() && fullPath.back() != L'\\' && fullPath.back() != L'/') fullPath += L'\\';
        fullPath += fileName;

        std::string acp = WideToAcp(fullPath);
        if (!acp.empty()) {
            m_playlistManager->AddItem(acp);
            added++;
        }
    }

    UpdatePlaylistUI();
}

void IJKPlayerWindow::SetPlaylistVisible(bool visible)
{
    // 同步第一组按钮状态（playlistShow/playlistHide）
    if (m_playlistShowButton && m_playlistHideButton)
    {
        m_playlistShowButton->SetVisible(!visible);
        m_playlistHideButton->SetVisible(visible);
    }
    
    // 同步第二组按钮状态（sideShow/sideHide）
    if (m_sideShowButton && m_sideHideButton)
    {
        m_sideShowButton->SetVisible(!visible);
        m_sideHideButton->SetVisible(visible);
    }
    
    // 设置播放列表面板的可见性
    if (m_playlistPanel)
    {
        m_playlistPanel->SetVisible(visible);
    }
    
    // 触发视频窗口大小调整，以适应播放列表状态变化
    ::PostMessage(m_hWnd, WM_APP + 101, 0, 0);
}

void IJKPlayerWindow::OnOpenFolder()
{
    if (!m_playlistManager) return;

    std::wstring folder;
    if (!BrowseForFolder(m_hWnd, folder)) {
        return;
    }

    const size_t oldCount = m_playlistManager->GetCount();
    std::vector<std::wstring> files;
    if (!EnumerateMediaFilesInFolder(folder, files)) {
        if (m_statusLabel) m_statusLabel->SetText(_T("Invalid folder"));
        return;
    }

    size_t added = 0;
    for (const auto& fullPath : files) {
        std::string pathAcp = WideToAcp(fullPath);
        if (pathAcp.empty()) continue;
        m_playlistManager->AddItem(pathAcp);
        added++;
    }

    UpdatePlaylistUI();

    if (m_statusLabel) {
        CDuiString s;
        s.Format(_T("Folder imported: %u files"), (UINT)added);
        m_statusLabel->SetText(s);
    }

    // 若之前播放列表为空，默认选中并准备第一首
    if (oldCount == 0 && added > 0) {
        OnPlaylistItemSelected(0);
        m_autoPlayPending = true;
    }
}

void IJKPlayerWindow::OnOpenNetworkStream()
{
    NetworkStreamDialog dialog;
    dialog.Create(m_hWnd, _T("打开网络流"), UI_WNDSTYLE_DIALOG, 0L, 0, 0, 520, 240);
    dialog.CenterWindow();
    dialog.ShowModal();

    std::string networkUrl;
    if (dialog.GetResult(networkUrl) && !networkUrl.empty()) {
        m_playlistManager->AddItem(networkUrl);
        m_playlistManager->SetCurrentIndex(m_playlistManager->GetCount() - 1);
        UpdatePlaylistUI();

        if (m_playerController->OpenFile(networkUrl) && m_playerController->Prepare()) {
            m_currentFile = networkUrl;
            if (m_statusLabel) m_statusLabel->SetText(_T("正在播放网络流..."));
            m_autoPlayPending = true;
        }
    }
}

void IJKPlayerWindow::OnScreenNormal()
{
    if (::IsZoomed(m_hWnd)) {
        ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    }
}

void IJKPlayerWindow::OnToggleNoFrame()
{
    LONG_PTR style = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
    const bool hasCaption = (style & WS_CAPTION) != 0;
    if (hasCaption) {
        style &= ~(WS_CAPTION | WS_THICKFRAME);
        if (m_statusLabel) m_statusLabel->SetText(_T("No-frame: ON"));
    } else {
        style |= (WS_CAPTION | WS_THICKFRAME);
        if (m_statusLabel) m_statusLabel->SetText(_T("No-frame: OFF"));
    }
    ::SetWindowLongPtr(m_hWnd, GWL_STYLE, style);
    ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void IJKPlayerWindow::OnPlayerStateChanged(IjkMsgState state, int arg1, int arg2)
{
    switch (state) {
    case IJK_MSG_PREPARED:
        Log::Info("Player prepared");
        if (m_statusLabel) m_statusLabel->SetText(_T("Prepared"));
        if (m_autoPlayPending) {
            // 如果即将自动播放，确保背景图片隐藏，视频窗口显示
            if (m_videoBackground) {
                m_videoBackground->SetVisible(false);
            }
            if (m_videoHwnd && IsWindow(m_videoHwnd)) {
                ::ShowWindow(m_videoHwnd, SW_SHOW);
            }
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
            m_autoPlayPending = true;
        } else {
            // 没有下一首，显示背景图片，隐藏视频窗口
            if (m_videoBackground) {
                m_videoBackground->SetVisible(true);
            }
            if (m_videoHwnd && IsWindow(m_videoHwnd)) {
                ::ShowWindow(m_videoHwnd, SW_HIDE);
            }
        }
        break;
    }
    case IJK_MSG_ERROR:
        Log::Error("Player error: %d, %d", arg1, arg2);
        m_isPlaying = false;
        if (m_playButton) m_playButton->SetVisible(true);
        if (m_pauseButton) m_pauseButton->SetVisible(false);
        StopUpdateTimer();
        if (m_statusLabel) {
            m_statusLabel->SetText(_T("Error"));
        }
        // 播放错误时显示背景图片，隐藏视频窗口
        if (m_videoBackground) {
            m_videoBackground->SetVisible(true);
        }
        if (m_videoHwnd && IsWindow(m_videoHwnd)) {
            ::ShowWindow(m_videoHwnd, SW_HIDE);
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
        ::SetTimer(m_hWnd, 1, 100, NULL); // 100ms更新一次
        m_updateTimer = 1;
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
        const int maxV = m_progressSlider->GetMaxValue();
        const int denom = (maxV > 0) ? maxV : 1000;
        int progress = (int)((double)current / (double)duration * (double)denom);
        if (progress < 0) progress = 0;
        if (progress > denom) progress = denom;
        m_progressSlider->SetValue(progress);
        
        std::string timeStr = FormatTime(current) + " / " + FormatTime(duration);
        m_timeLabel->SetText(CDuiString(timeStr.c_str()));
    } else {
        m_progressSlider->SetValue(0);
        m_timeLabel->SetText(_T("00:00 / 00:00"));
    }
    
    // 更新媒体信息
    UpdateMediaInfo();
}

void IJKPlayerWindow::UpdateStatus()
{
    UpdateProgress();
}

void IJKPlayerWindow::UpdateMediaInfo()
{
    if (!m_playerController || !m_mediaInfoVisible) {
        return;
    }

    EnsureMediaInfoOverlay();
    if (!m_mediaInfoHwnd || !::IsWindow(m_mediaInfoHwnd)) {
        return;
    }

    IjkMetadata metadata{};
    if (!m_playerController->GetMediaMeta(&metadata)) {
        m_mediaInfoText = "Media info unavailable";
        ::InvalidateRect(m_mediaInfoHwnd, NULL, TRUE);
        return;
    }

    auto formatBitrate = [](int64_t br) -> std::string {
        if (br <= 0) return "N/A";
        char buf[64]{};
        if (br >= 1000000) {
            sprintf_s(buf, "%.2f Mbps", (double)br / 1000000.0);
        } else if (br >= 1000) {
            sprintf_s(buf, "%.2f Kbps", (double)br / 1000.0);
        } else {
            sprintf_s(buf, "%lld bps", (long long)br);
        }
        return std::string(buf);
    };

    // 运行时“实时变化”的读速（bytes/s）。在 ijkplayer 内部对应 FFP_PROP_INT64_TCP_SPEED = 20200
    // 这里换算成 bps 作为实时码率来源：bps = bytes/s * 8
    const long tcpSpeedBytesPerSec = m_playerController->GetPropertyLong(20200 /*FFP_PROP_INT64_TCP_SPEED*/, -1);
    const int64_t totalBrRt =
        (tcpSpeedBytesPerSec > 0) ? (int64_t)tcpSpeedBytesPerSec * 8
                                  : (int64_t)m_playerController->GetPropertyLong(INT64_BIT_RATE_TOTAL, -1);

    // 只显示“输出帧率”
    const float fpsOut = m_playerController->GetPropertyFloat(FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND, 0.0f);

    std::ostringstream oss;
    oss << "Video\r\n";
    if (metadata.video_code_name[0] != '\0') {
        oss << "  Codec: " << metadata.video_code_name << "\r\n";
    }
    if (fpsOut > 0.01f) {
        oss << "  FPS: " << std::fixed << std::setprecision(2) << fpsOut << "\r\n";
    }
    if (metadata.width > 0 && metadata.height > 0) {
        oss << "  Size: " << metadata.width << "x" << metadata.height << "\r\n";
    }
    // 码率：用“实时总码率”按占比估算到视频/音频（不展示 meta 类型码率）
    int64_t vBrRt = -1;
    int64_t aBrRt = -1;
    if (totalBrRt > 0) {
        const double vMeta = (metadata.video_bitrate > 0) ? (double)metadata.video_bitrate : 0.0;
        const double aMeta = (metadata.audio_bitrate > 0) ? (double)metadata.audio_bitrate : 0.0;
        double vRatio = 0.8; // fallback：没有 meta 时给个默认占比
        if (vMeta + aMeta > 0.0) {
            vRatio = vMeta / (vMeta + aMeta);
        }
        vBrRt = (int64_t)((double)totalBrRt * vRatio);
        aBrRt = (int64_t)((double)totalBrRt * (1.0 - vRatio));
    }
    if (vBrRt > 0) {
        oss << "  Bitrate(rt est): " << formatBitrate(vBrRt) << "\r\n";
    }

    oss << "\r\nAudio\r\n";
    if (metadata.audio_code_name[0] != '\0') {
        oss << "  Codec: " << metadata.audio_code_name << "\r\n";
    }
    if (metadata.audio_channel_layout > 0) {
        int channels = 0;
        uint64_t layout = (uint64_t)metadata.audio_channel_layout;
        while (layout) {
            channels++;
            layout &= layout - 1;
        }
        if (channels == 0) channels = 1;
        std::string chStr;
        if (channels == 1) chStr = "Mono";
        else if (channels == 2) chStr = "Stereo";
        else chStr = std::to_string(channels) + "ch";
        oss << "  Channels: " << chStr << "\r\n";
    }
    if (metadata.audio_samples_per_sec > 0) {
        oss << "  SampleRate: " << metadata.audio_samples_per_sec << " Hz\r\n";
    }
    if (aBrRt > 0) {
        oss << "  Bitrate(rt est): " << formatBitrate(aBrRt) << "\r\n";
    }
    if (totalBrRt > 0) {
        oss << "\r\nTotal Bitrate(rt): " << formatBitrate(totalBrRt) << "\r\n";
    }

    m_mediaInfoText = oss.str();
    ::InvalidateRect(m_mediaInfoHwnd, NULL, TRUE);
}

void IJKPlayerWindow::ToggleMediaInfo()
{
    m_mediaInfoVisible = !m_mediaInfoVisible;
    SyncMediaInfoOverlayPos();
    if (m_mediaInfoVisible) {
        // 立即更新一次信息
        UpdateMediaInfo();
    }
}

void IJKPlayerWindow::UpdatePlaylistUI()
{
    if (!m_playlistList) {
        return;
    }

    // res/IJKPlayer.xml 的播放列表是 TreeView（继承自 CListUI），这里优先按 TreeView 的方式填充
    if (auto* pTree = static_cast<CTreeViewUI*>(m_playlistList->GetInterface(DUI_CTR_TREEVIEW))) {
        pTree->RemoveAll();
        // TreeView 默认会显示“文件夹按钮”，这里关闭（我们当普通列表用）
        pTree->SetVisibleFolderBtn(false);
        pTree->SetVisibleCheckBtn(false);
        for (size_t i = 0; i < m_playlistManager->GetCount(); ++i) {
            auto item = m_playlistManager->GetItem(i);
            if (!item) continue;

            CTreeNodeUI* pNode = new CTreeNodeUI();
            // 明确设置文字颜色：否则默认 0x00000000（黑字），在黑底上不可见
            pNode->SetItemTextColor(0xFF85909F);
            pNode->SetItemHotTextColor(0xFFFFFFFF);
            pNode->SetSelItemTextColor(0xFFFFFFFF);
            pNode->SetSelItemHotTextColor(0xFFFFFFFF);
            pNode->SetFixedHeight(26);

            // 本工程为 MBCS：fileName 使用 ACP 存储，直接显示即可
            pNode->SetItemText(CDuiString(item->fileName.c_str()));
            pTree->Add(pNode);
        }
        return;
    }

    // fallback：普通 List
    m_playlistList->RemoveAll();
    for (size_t i = 0; i < m_playlistManager->GetCount(); ++i) {
        auto item = m_playlistManager->GetItem(i);
        if (!item) continue;
        CListLabelElementUI* pListElement = new CListLabelElementUI;
        pListElement->SetText(CDuiString(item->fileName.c_str()));
        m_playlistList->Add(pListElement);
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

