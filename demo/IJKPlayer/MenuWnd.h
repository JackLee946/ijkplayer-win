#pragma once
#include "duilib.h"

class MenuWnd: public CXMLWnd
{
public:
    explicit MenuWnd(LPCTSTR pszXMLName);

protected:
    virtual ~MenuWnd();   // 私有化析构函数，这样此对象只能通过new来生成，而不能直接定义变量。就保证了delete this不会出错

public:
    void Init(CPaintManagerUI *pOwnerPM, POINT ptPos);
    virtual void    OnFinalMessage(HWND hWnd);
    virtual LRESULT HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnKillFocus   (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    virtual void    Notify(TNotifyUI& msg);

private:
    CPaintManagerUI *m_pOwnerPM;
};
