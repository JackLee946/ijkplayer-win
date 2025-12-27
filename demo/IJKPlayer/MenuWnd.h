#pragma once
#include "duilib.h"

class MenuWnd: public CXMLWnd
{
public:
    explicit MenuWnd(LPCTSTR pszXMLName);

protected:
    virtual ~MenuWnd();   // ˽�л����������������˶���ֻ��ͨ��new�����ɣ�������ֱ�Ӷ���������ͱ�֤��delete this�������

public:
    void Init(CPaintManagerUI *pOwnerPM, POINT ptPos);
    virtual void    OnFinalMessage(HWND hWnd);
    virtual LRESULT HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnKillFocus   (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    virtual void    Notify(TNotifyUI& msg);

private:
    void RequestClose();

    CPaintManagerUI *m_pOwnerPM;
    bool m_closing{ false };
};
