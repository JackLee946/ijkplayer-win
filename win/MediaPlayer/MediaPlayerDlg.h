
// MediaPlayerDlg.h: 头文件
//

#pragma once

#include "InfoDialog.h"
#include "logging.h"

extern "C"
{
#include "../win/ijkplayer/ijk_ffplay_decoder.h"
#include "libavformat/url.h"
#include "SDL.h"
}

// CMediaPlayerDlg 对话框
class CMediaPlayerDlg : public CDialogEx
{
// 构造
public:
	CMediaPlayerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MEDIAPLAYER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonFileBrowse();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonInfo();

private:
	int Init();
	void UpdatePlayProgress();
    // 更新时间显示
    void UpdateTimeDisplay(int64_t currentPos, int64_t duration);
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

    void UpdateMediaInfo();

	friend void video_callback(void* opaque, IjkVideoFrame* frame_callback);
	friend void msg_callback(void* opaque, IjkMsgState ijk_msgint, int arg1, int arg2);

    CFont m_infoFont;          // 信息显示字体
    CStatic m_infoDisplay;     // 信息显示控件
    CString m_videoInfo;       // 视频信息
    CString m_audioInfo;       // 音频信息

	IjkFfplayDecoder* m_ijk_decoder;
	SDL_Renderer* m_sdl_renderer;
	SDL_Window* m_screen;
	SDL_Texture* m_sdl_texture;
	SDL_Rect     m_sdl_rect;
	char* m_nv12_data;
	bool  m_sdl_init_flag;

    int64_t m_lastPosition; // 记录上次播放位置
    bool m_isSeeking;       // 标记是否正在拖动进度条	


	CInfoDialog m_InfoDlg; // 信息对话框指针
	bool m_bInfoShowing;     // 是否正在显示信息窗口

};
