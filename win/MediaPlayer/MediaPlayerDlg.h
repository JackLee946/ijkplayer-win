
// MediaPlayerDlg.h: 头文件
//

#pragma once

extern "C"
{
#include "../win/ijkplayer/ijk_ffplay_decoder.h"
#include "libavformat/url.h"
#include "SDL.h"
}

#include "logging.h"
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
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonFileBrowse();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonInfo();

private:
	int Init();

	friend void video_callback(void* opaque, IjkVideoFrame* frame_callback);
	friend void msg_callback(void* opaque, IjkMsgState ijk_msgint, int arg1, int arg2);

	IjkFfplayDecoder* m_ijk_decoder;
	SDL_Renderer* m_sdl_renderer;
	SDL_Window* m_screen;
	SDL_Texture* m_sdl_texture;
	SDL_Rect     m_sdl_rect;
	char* m_nv12_data;
	bool  m_sdl_init_flag;
};
