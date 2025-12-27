#include "NetworkStreamDialog.h"
#include <windows.h>
#include <algorithm>
#include <string>

// 控件ID定义
const TCHAR* const NetworkStreamDialog::kLabelUrl = _T("label_url");
const TCHAR* const NetworkStreamDialog::kEditUrl = _T("edit_url");
const TCHAR* const NetworkStreamDialog::kLabelError = _T("label_error");
const TCHAR* const NetworkStreamDialog::kBtnOK = _T("btn_ok");
const TCHAR* const NetworkStreamDialog::kBtnCancel = _T("btn_cancel");

namespace {

static inline void TrimInPlace(std::string& s)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}

static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    if (len <= 0) return {};
    std::string out;
    out.resize((size_t)len);
    // 注意：部分 MSVC 标准库下 std::string::data() 返回 const char*；用 &out[0] 获取可写缓冲区
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], len, NULL, NULL);
    return out;
}

static std::string DuiStringToUtf8(const CDuiString& s)
{
#ifdef UNICODE
    std::wstring w = s.GetData();
    return WideToUtf8(w);
#else
    // ANSI build：先按 ACP 转 wide，再转 UTF-8
    std::string a = s.GetData();
    if (a.empty()) return {};
    int wlen = MultiByteToWideChar(CP_ACP, 0, a.c_str(), (int)a.size(), NULL, 0);
    if (wlen <= 0) return {};
    std::wstring w;
    w.resize((size_t)wlen);
    // 同理：wstring::data() 可能返回 const wchar_t*，使用 &w[0]
    MultiByteToWideChar(CP_ACP, 0, a.c_str(), (int)a.size(), &w[0], wlen);
    return WideToUtf8(w);
#endif
}

static bool LooksLikeUrl(const std::string& s)
{
    // 很宽松的校验：至少包含 scheme://
    if (s.size() < 6) return false;
    auto pos = s.find("://");
    if (pos == std::string::npos || pos == 0) return false;
    return true;
}

} // namespace

NetworkStreamDialog::NetworkStreamDialog()
    : m_labelUrl(nullptr)
    , m_editUrl(nullptr)
    , m_labelError(nullptr)
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
    m_labelError = static_cast<CLabelUI*>(m_pm.FindControl(kLabelError));
    m_btnOK = static_cast<CButtonUI*>(m_pm.FindControl(kBtnOK));
    m_btnCancel = static_cast<CButtonUI*>(m_pm.FindControl(kBtnCancel));

    if (m_labelError) {
        m_labelError->SetVisible(false);
    }

    // 设置焦点到编辑框
    if (m_editUrl) {
        m_editUrl->SetFocus();
        // 方便直接输入替换
        m_editUrl->SetSelAll();
    }
}

void NetworkStreamDialog::OnClick(TNotifyUI& msg)
{
    CDuiString name = msg.pSender ? msg.pSender->GetName() : CDuiString();

    auto showError = [&](LPCTSTR text) {
        if (!m_labelError) return;
        m_labelError->SetText(text);
        m_labelError->SetVisible(true);
    };

    auto clearError = [&]() {
        if (!m_labelError) return;
        m_labelError->SetVisible(false);
        m_labelError->SetText(_T(""));
    };

    if (msg.pSender == m_btnCancel || name == _T("closebtn")) {
        m_bOK = false;
        m_networkUrl.clear();
        Close();
        WindowImplBase::OnClick(msg);
        return;
    }

    if (msg.pSender == m_btnOK) {
        clearError();
        if (!m_editUrl) {
            showError(_T("Internal error: missing edit box"));
            return;
        }

        std::string url = DuiStringToUtf8(m_editUrl->GetText());
        TrimInPlace(url);
        if (url.empty()) {
            showError(_T("请输入网络流地址"));
            return;
        }
        if (!LooksLikeUrl(url)) {
            showError(_T("地址格式不正确（示例：rtsp://... 或 http(s)://...）"));
            return;
        }

        m_networkUrl = url;
        m_bOK = true;
        Close();
        WindowImplBase::OnClick(msg);
        return;
    }
    
    WindowImplBase::OnClick(msg);
}

LRESULT NetworkStreamDialog::ResponseDefaultKeyEvent(WPARAM wParam)
{
    // Enter 确定 / Esc 取消
    if (wParam == VK_RETURN) {
        TNotifyUI n{};
        n.pSender = m_btnOK ? static_cast<CControlUI*>(m_btnOK) : nullptr;
        OnClick(n);
        return TRUE;
    }
    if (wParam == VK_ESCAPE) {
        TNotifyUI n{};
        n.pSender = m_btnCancel ? static_cast<CControlUI*>(m_btnCancel) : nullptr;
        OnClick(n);
        return TRUE;
    }
    return WindowImplBase::ResponseDefaultKeyEvent(wParam);
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