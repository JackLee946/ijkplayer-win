#ifndef NETWORK_STREAM_DIALOG_H
#define NETWORK_STREAM_DIALOG_H

#include "Utils/WinImplBase.h"
#include <string>

using namespace DuiLib;

class NetworkStreamDialog : public WindowImplBase {
public:
    NetworkStreamDialog();
    virtual ~NetworkStreamDialog();

    // WindowImplBase接口实现
    virtual CDuiString GetSkinFile() override;
    virtual LPCTSTR GetWindowClassName(void) const override;
    virtual void InitWindow() override;
    virtual void OnClick(TNotifyUI& msg) override;

    // 获取对话框结果
    bool GetResult(std::string& networkUrl);

protected:
    virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam) override;

    // 控件ID定义
    static const TCHAR* const kLabelUrl;
    static const TCHAR* const kEditUrl;
    static const TCHAR* const kLabelError;
    static const TCHAR* const kBtnOK;
    static const TCHAR* const kBtnCancel;

    // 控件指针
    CLabelUI* m_labelUrl;
    CEditUI* m_editUrl;
    CLabelUI* m_labelError;
    CButtonUI* m_btnOK;
    CButtonUI* m_btnCancel;

    // 对话框结果
    bool m_bOK;
    std::string m_networkUrl;

    // 消息映射
    DUI_DECLARE_MESSAGE_MAP()
};

#endif // NETWORK_STREAM_DIALOG_H