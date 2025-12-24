#include "MenuWnd.h"

MenuWnd::MenuWnd( LPCTSTR pszXMLName ) 
: CXMLWnd(pszXMLName),
m_pOwnerPM(NULL)
{

}

MenuWnd::~MenuWnd()
{

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

void MenuWnd::OnFinalMessage( HWND /*hWnd*/ )
{
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
    Close();
    bHandled = FALSE;
    return __super::OnKillFocus(uMsg, wParam, lParam, bHandled); 
}

void MenuWnd::Notify( TNotifyUI& msg )
{
    if(msg.sType == DUI_MSGTYPE_ITEMCLICK)
    {
        if(m_pOwnerPM)
        {
            m_pOwnerPM->SendNotify(msg);
        }

        Close();
    }
    
    __super::Notify(msg); 
}
