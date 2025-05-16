
// MediaPlayerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "MediaPlayer.h"
#include "MediaPlayerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_UPDATE_UI 1

std::string CStringToStdString(const CString& cstr, UINT codePage = CP_ACP) {
#ifdef _UNICODE
	// Unicode环境：宽字符 → 多字节
	int length = WideCharToMultiByte(codePage, 0, cstr.GetString(), -1, NULL, 0, NULL, NULL);
	if (length <= 0) return "";

	std::string result;
	result.resize(length - 1);  // 排除终止符
	WideCharToMultiByte(codePage, 0, cstr.GetString(), -1, &result[0], length, NULL, NULL);
	return result;
#else
	// 多字节环境：直接转换
	return std::string(cstr.GetString());
#endif
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMediaPlayerDlg 对话框

CMediaPlayerDlg* g_pDlg = nullptr;

CMediaPlayerDlg::CMediaPlayerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MEDIAPLAYER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_sdl_init_flag = false;
}

void CMediaPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMediaPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_FILE_BROWSE, &CMediaPlayerDlg::OnBnClickedButtonFileBrowse)
	ON_BN_CLICKED(IDC_BUTTON_START, &CMediaPlayerDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CMediaPlayerDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_INFO, &CMediaPlayerDlg::OnBnClickedButtonInfo)
END_MESSAGE_MAP()


// CMediaPlayerDlg 消息处理程序

BOOL CMediaPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	Init();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMediaPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMediaPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMediaPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMediaPlayerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)  // 检查是我们设置的定时器
	{
		UpdatePlayProgress();
	}

	CDialogEx::OnTimer(nIDEvent);  // 调用基类处理
}

static void log_callback(void*, int level, const char* szFmt, va_list varg)
{
	char line[1024] = { 0 };
	vsnprintf(line, sizeof(line), szFmt, varg);

	switch (level) {
	case k_IJK_LOG_DEBUG:
		//Log::Debug("%s", line);
		break;
	case k_IJK_LOG_INFO:
		Log::Info("%s", line);
		break;
	case k_IJK_LOG_WARN:
		Log::Warn("%s", line);
		break;
	case k_IJK_LOG_ERROR:
		Log::Error("%s", line);
		break;
	case k_IJK_LOG_FATAL:
		Log::Fatal("%s", line);
		break;
	default:
		Log::Info("%s", line);
		break;
	}
	return;
}

void video_callback(void* opaque, IjkVideoFrame* frame_callback)
{
	if (g_pDlg->m_sdl_init_flag == false)
	{
		int screen_w, screen_h;
		if (frame_callback->format == PIX_FMT_YUV420P) {
			screen_w = frame_callback->w;
			screen_h = frame_callback->h;
		}
		else if (frame_callback->format == PIX_FMT_NV12) {
			screen_w = frame_callback->linesize[0];
			screen_h = frame_callback->h;
		}
		//screen_w = g_pDlg->m_preview_width;
		//screen_h = g_pDlg->m_preview_height;
		if (frame_callback->format == PIX_FMT_YUV420P) {
			g_pDlg->m_sdl_renderer = SDL_CreateRenderer(g_pDlg->m_screen, -1, SDL_RENDERER_ACCELERATED);
			SDL_RenderSetLogicalSize(g_pDlg->m_sdl_renderer, screen_w, screen_h); // 设置逻辑分辨率（视频原始分辨率）
			//SDL_RenderSetIntegerScale(g_pDlg->m_sdl_renderer, SDL_TRUE);  // 保持整数倍缩放（避免模糊）
			g_pDlg->m_sdl_texture = SDL_CreateTexture(g_pDlg->m_sdl_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
		}
		else if (frame_callback->format == PIX_FMT_NV12) {
			SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
			SDL_RenderSetLogicalSize(g_pDlg->m_sdl_renderer, screen_w, screen_h); // 设置逻辑分辨率（视频原始分辨率）
			//SDL_RenderSetIntegerScale(g_pDlg->m_sdl_renderer, SDL_TRUE);  // 保持整数倍缩放（避免模糊）
			g_pDlg->m_sdl_renderer = SDL_CreateRenderer(g_pDlg->m_screen, -1, SDL_RENDERER_ACCELERATED);
			g_pDlg->m_sdl_texture = SDL_CreateTexture(g_pDlg->m_sdl_renderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
		}

		g_pDlg->m_sdl_rect.x = 0;
		g_pDlg->m_sdl_rect.y = 0;
		g_pDlg->m_sdl_rect.w = screen_w;
		g_pDlg->m_sdl_rect.h = screen_h;

		g_pDlg->m_sdl_init_flag = true;
		Log::Info("screen_w: %d, screen_h: %d", screen_w, screen_h);
	}

	if (frame_callback->format == PIX_FMT_YUV420P)
	{
		SDL_UpdateYUVTexture(g_pDlg->m_sdl_texture, &g_pDlg->m_sdl_rect,
			frame_callback->data[0], frame_callback->linesize[0],
			frame_callback->data[1], frame_callback->linesize[1],
			frame_callback->data[2], frame_callback->linesize[2]);
	}
	else if (frame_callback->format == PIX_FMT_NV12) 
	{
		memcpy(g_pDlg->m_nv12_data, frame_callback->data[0], frame_callback->h * frame_callback->linesize[0]);
		memcpy(g_pDlg->m_nv12_data + frame_callback->h * frame_callback->linesize[0], frame_callback->data[1], frame_callback->h * frame_callback->linesize[0] / 2);
		SDL_UpdateTexture(g_pDlg->m_sdl_texture, NULL, g_pDlg->m_nv12_data, frame_callback->linesize[0]);
	}

	SDL_RenderClear(g_pDlg->m_sdl_renderer);
	SDL_RenderCopy(g_pDlg->m_sdl_renderer, g_pDlg->m_sdl_texture, NULL, &g_pDlg->m_sdl_rect);
	SDL_RenderPresent(g_pDlg->m_sdl_renderer);

}

void msg_callback(void* opaque, IjkMsgState ijk_msgint, int arg1, int arg2)
{
	long duration = 0;
	switch (ijk_msgint)
	{
	case IJK_MSG_VIDEO_DECODE_FPS:
		Log::Info("video decode fps:%d", arg1);
		break;
	case IJK_MSG_VIDEO_GOP_SIZE:
		Log::Info("video gop size:%d", arg1);
		break;
	case IJK_MSG_FLUSH:
		break;
	case IJK_MSG_ERROR:
		printf("ijk error, arg1:%d, arg2:%d\n", arg1, arg2);
		break;
	case IJK_MSG_PREPARED:
		ijkFfplayDecoder_start(g_pDlg->m_ijk_decoder);
		Log::Info("recv ijk msg:prepared");
		break;
	case IJK_MSG_COMPLETED:
		Log::Info("recv ijk msg:complete.");
		ijkFfplayDecoder_pause(g_pDlg->m_ijk_decoder);
		ijkFfplayDecoder_stop(g_pDlg->m_ijk_decoder);
		break;
	case IJK_MSG_VIDEO_SIZE_CHANGED:
		break;
	case IJK_MSG_SAR_CHANGED:
		break;
	case IJK_MSG_VIDEO_RENDERING_START:
		break;
	case IJK_MSG_AUDIO_RENDERING_START:
		break;
	case IJK_MSG_VIDEO_ROTATION_CHANGED:
		break;
	case IJK_MSG_BUFFERING_START:
		Log::Info("ijk buffering start, arg1:%d, arg2:%d\n", arg1, arg2);
		break;
	case IJK_MSG_BUFFERING_END:
		Log::Info("ijk buffering end, arg1:%d, arg2:%d\n", arg1, arg2);
		break;
	case IJK_MSG_BUFFERING_UPDATE:
		Log::Info("ijk buffering update, arg1:%d, arg2:%d\n", arg1, arg2);
		break;
	case IJK_MSG_BUFFERING_BYTES_UPDATE:
		break;
	case IJK_MSG_BUFFERING_TIME_UPDATE:
		break;
	case IJK_MSG_SEEK_COMPLETE:
		break;
	case IJK_MSG_PLAYBACK_STATE_CHANGED:
		break;
	case IJK_MSG_TIMED_TEXT:
		break;
	case IJK_MSG_ACCURATE_SEEK_COMPLETE:
		break;
	case IJK_MSG_VIDEO_DECODER_OPEN:
		break;
	default:
		break;
	}
}

int CMediaPlayerDlg::Init()
{
	g_pDlg = this;
	std::string url = "./mediaplayer.log";
	Log::Initialise(url);
	Log::SetThreshold(Log::LOG_TYPE_DEBUG);

	//init global paraments
	ijkFfplayDecoder_init();

	ijkFfplayDecoder_setLogLevel(k_IJK_LOG_DEBUG);
	ijkFfplayDecoder_setLogCallback(log_callback);
	IjkFfplayDecoderCallBack* decoder_callback = (IjkFfplayDecoderCallBack*)malloc(sizeof(IjkFfplayDecoderCallBack));
	decoder_callback->func_get_frame = video_callback;
	decoder_callback->func_state_change = msg_callback;

	m_ijk_decoder = ijkFfplayDecoder_create();
	ijkFfplayDecoder_setDecoderCallBack(m_ijk_decoder, NULL, decoder_callback);
	ijkFfplayDecoder_setHwDecoderName(m_ijk_decoder, NULL);
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	HWND hWnd = GetDlgItem(IDC_STATIC_SCREEN)->GetSafeHwnd();
	m_screen = SDL_CreateWindowFrom(hWnd);
	m_nv12_data = (char*)malloc(4 * 1024 * 1024);
	return 0;
}

void CMediaPlayerDlg::UpdatePlayProgress()
{

}

void CMediaPlayerDlg::OnBnClickedButtonFileBrowse()
{
	// 初始化COM库
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) {
		AfxMessageBox(_T("COM初始化失败"));
		return;
	}

	IFileOpenDialog* pFileOpen = NULL;
	IShellItem* pItem = NULL;
	PWSTR pszFilePath = NULL;

	// 创建文件对话框实例
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
	if (SUCCEEDED(hr)) {
		// 设置对话框选项
		DWORD dwOptions;
		pFileOpen->GetOptions(&dwOptions);
		pFileOpen->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);

		// 设置文件类型过滤器
		COMDLG_FILTERSPEC rgSpec[] = {
			{ L"MP4 Files", L"*.mp4" },
			{ L"All Files", L"*.*" }
		};
		pFileOpen->SetFileTypes(2, rgSpec);
		pFileOpen->SetFileTypeIndex(1);

		// 显示对话框
		hr = pFileOpen->Show(NULL);
		if (SUCCEEDED(hr)) {
			// 获取选择结果
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				// 获取完整文件路径
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr)) {
					// 转换为CString使用
					CString strFilePath = pszFilePath;
					SetDlgItemText(IDC_EDIT_URL, strFilePath);
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}

	// 反初始化COM库
	CoUninitialize();
}


void CMediaPlayerDlg::OnBnClickedButtonStart()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strfilePath;
	GetDlgItemText(IDC_EDIT_URL, strfilePath);

	if (strfilePath.IsEmpty())
	{
		MessageBox(_T("请选择正确的文件路径"));
		return;
	}

	static bool is_start = false;

	if (!is_start)
	{
		std::string filePath = CStringToStdString(strfilePath);
		ijkFfplayDecoder_setDataSource(m_ijk_decoder, filePath.c_str());
		ijkFfplayDecoder_prepare(m_ijk_decoder);
		SetTimer(TIMER_UPDATE_UI, 1000, NULL);
		is_start = true;
	}
	else
	{
		ijkFfplayDecoder_pause(m_ijk_decoder);
		ijkFfplayDecoder_stop(m_ijk_decoder);
		is_start = false;
	}

}


void CMediaPlayerDlg::OnBnClickedButtonPause()
{
	static bool is_pause = false;
	if (!is_pause) {
		printf("pause player now.\n");
		is_pause = true;
		ijkFfplayDecoder_pause(m_ijk_decoder);
	}
	else {
		printf("resume player now.\n");
		is_pause = false;
		ijkFfplayDecoder_start(m_ijk_decoder);
	}
}

void CMediaPlayerDlg::OnBnClickedButtonInfo()
{
	// TODO: 在此添加控件通知处理程序代码
}