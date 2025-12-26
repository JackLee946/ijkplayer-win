#include "NetworkStreamDialog.h"
#include <windows.h>

// 控件ID定义
const TCHAR* const NetworkStreamDialog::kLabelUrl = _T("label_url");
const TCHAR* const NetworkStreamDialog::kEditUrl = _T("edit_url");
const TCHAR* const NetworkStreamDialog::kBtnOK = _T("btn_ok");
const TCHAR* const NetworkStreamDialog::kBtnCancel = _T("btn_cancel");

NetworkStreamDialog::NetworkStreamDialog()
    : m_labelUrl(nullptr)
    , m_editUrl(nullptr)
    , m_btnOK(nullptr)
    , m_btnCancel(nullptr)
    , m_bOK(false)
{
}

NetworkStreamDialog::~NetworkStreamDialog()
{
}

CDuiString NetworkStreamDialog::GetSkinFile()
{
    return _T("NetworkStreamDialog.xml");
}

LPCTSTR NetworkStreamDialog::GetWindowClassName(void) const
{
    return _T("NetworkStreamDialog");
}

void NetworkStreamDialog::InitWindow()
{
    // 获取控件指针
    m_labelUrl = static_cast<CLabelUI*>(m_pm.FindControl(kLabelUrl));
    m_editUrl = static_cast<CEditUI*>(m_pm.FindControl(kEditUrl));
    m_btnOK = static_cast<CButtonUI*>(m_pm.FindControl(kBtnOK));
    m_btnCancel = static_cast<CButtonUI*>(m_pm.FindControl(kBtnCancel));

    // 添加调试信息
    if (m_btnOK) {
        OutputDebugString(_T("m_btnOK initialized successfully\n"));
    } else {
        OutputDebugString(_T("m_btnOK initialization failed\n"));
    }

    if (m_btnCancel) {
        OutputDebugString(_T("m_btnCancel initialized successfully\n"));
    } else {
        OutputDebugString(_T("m_btnCancel initialization failed\n"));
    }

    // 设置焦点到编辑框
    if (m_editUrl) {
        m_editUrl->SetFocus();
    }
}

void NetworkStreamDialog::Notify(TNotifyUI& msg)
{
    OutputDebugString(_T("Notify function called\n"));
    WindowImplBase::Notify(msg);
}

void NetworkStreamDialog::OnClick(TNotifyUI& msg)
{
    OutputDebugString(_T("OnClick function called\n"));
    
    if (msg.pSender == m_btnOK) {
        OutputDebugString(_T("OK button clicked\n"));
        // 确定按钮被点击
        if (m_editUrl) {
            CDuiString text = m_editUrl->GetText();
            
            // 将宽字符转换为多字节字符串
            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)text.GetData(), -1, NULL, 0, NULL, NULL);
            if (bufferSize > 0) {
                char* buffer = new char[bufferSize];
                WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)text.GetData(), -1, buffer, bufferSize, NULL, NULL);
                m_networkUrl = buffer;
                delete[] buffer;
            }
        }
        m_bOK = true;
        Close();
    } else if (msg.pSender == m_btnCancel) {
        OutputDebugString(_T("Cancel button clicked\n"));
        // 取消按钮被点击
        m_bOK = false;
        Close();
    } else {
        OutputDebugString(_T("Unknown button clicked\n"));
    }
    
    WindowImplBase::OnClick(msg);
}

bool NetworkStreamDialog::GetResult(std::string& networkUrl)
{
    if (m_bOK) {
        networkUrl = m_networkUrl;
    }
    return m_bOK;
}

DUI_BEGIN_MESSAGE_MAP(NetworkStreamDialog, WindowImplBase)
    DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK, OnClick)
DUI_END_MESSAGE_MAP()