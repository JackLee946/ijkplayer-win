#include "MenuWnd.h"

MenuWnd::MenuWnd( LPCTSTR pszXMLName ) 
: CXMLWnd(pszXMLName),
m_pOwnerPM(NULL)
{

}

MenuWnd::~MenuWnd()
{

}

void MenuWnd::RequestClose()
{
    // 避免 Close() 触发同步 Destroy + delete this，导致当前消息栈继续执行时 use-after-free
    if (m_closing) return;
    m_closing = true;
    if (::IsWindow(m_hWnd)) {
        ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
    }
}

void MenuWnd::Init(CPaintManagerUI *pOwnerPM, POINT ptPos)
{
    if( pOwnerPM == NULL ) 
    {
        return;
    }

    m_pOwnerPM = pOwnerPM;
    Create(pOwnerPM->GetPaintWindow(), _T("MenuWnd"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
    ::ClientToScreen(pOwnerPM->GetPaintWindow(), &ptPos);
    ::SetWindowPos(*this, NULL, ptPos.x, ptPos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void MenuWnd::OnFinalMessage( HWND hWnd )
{
    // 关键：先让 WindowImplBase 清理 m_pm 中的 notifier / message filter 等资源，
    // 否则 MenuWnd delete 后，CPaintManagerUI 仍可能回调到悬空指针导致崩溃。
    WindowImplBase::OnFinalMessage(hWnd);
    delete this;
}

LRESULT MenuWnd::HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT lRes = 0;
    BOOL bHandled = TRUE;

    switch( uMsg )
    {
    case WM_KILLFOCUS:    
        lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); 
        break; 

    default:
        bHandled = FALSE;
    }

    if(bHandled || m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) 
    {
        return lRes;
    }

    return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT MenuWnd::OnKillFocus( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    RequestClose();
    // 我们已经处理了“失焦即关闭”，不再让基类继续链式处理，避免重复关闭
    bHandled = TRUE;
    return 0;
}

void MenuWnd::Notify( TNotifyUI& msg )
{
    if(msg.sType == DUI_MSGTYPE_ITEMCLICK)
    {
        // 重要：不要把 msg 直接转发给主窗口（msg.pSender 属于 MenuWnd，自身可能在弹系统对话框时被销毁，
        // 导致主窗口或后续处理访问到悬空指针而崩溃）。
        // 这里改为投递 WM_COMMAND（只传递整数 ID），由主窗口在自己的消息循环中处理。
        if (m_pOwnerPM) {
            HWND hOwner = m_pOwnerPM->GetPaintWindow();
            if (::IsWindow(hOwner) && msg.pSender) {
                const CDuiString name = msg.pSender->GetName();
                int cmd = 0;
                if (name == _T("menu_OpenFile")) cmd = 2001;
                else if (name == _T("menu_OpenFolder")) cmd = 2002;
                else if (name == _T("menu_OpenNetworkStream")) cmd = 2003;
                else if (name == _T("menu_NoFrame")) cmd = 2004;
                else if (name == _T("menu_SetFullScreen")) cmd = 2005;
                else if (name == _T("menu_ExitFullScreen")) cmd = 2006;
                else if (name == _T("menu_AddToPlaylist")) cmd = 2007;
                else if (name == _T("menu_Exit")) cmd = 2008;

                if (cmd != 0) {
                    ::PostMessage(hOwner, WM_COMMAND, (WPARAM)cmd, 0);
                }
            }
        }

        RequestClose();
    }
    
    __super::Notify(msg); 
}
