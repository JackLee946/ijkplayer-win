#include "IJKPlayerWindow.h"
#include "logging.h"
#include "Core/UIManager.h"
#include <windows.h>
#include <tchar.h>
#include <dbghelp.h>
#include <io.h>

static LONG WINAPI IJKPlayerUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
    HANDLE hFile = CreateFile(
        _T("IJKPlayer.dmp"),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pExceptionInfo;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpNormal,
            &mdei,
            NULL,
            NULL);

        CloseHandle(hFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    // 初始化日志
    Log::Initialise("ijkplayer.log");
    Log::SetThreshold(Log::LOG_TYPE_INFO);

    // 捕获未处理异常，生成 dump 方便定位“闪退”问题
    SetUnhandledExceptionFilter(IJKPlayerUnhandledExceptionFilter);

    // 自测模式下打开更详细日志
    if (_access("./selftest.maximize", 0) == 0) {
        Log::SetThreshold(Log::LOG_TYPE_DEBUG);
        Log::Info("SelfTest: log threshold set to DEBUG");
    }

    // 初始化COM
    CoInitialize(NULL);

    // 初始化duilib
    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("res/"));

    // 创建主窗口
    IJKPlayerWindow* pFrame = new IJKPlayerWindow();
    if (pFrame == NULL) {
        CoUninitialize();
        Log::Finalise();
        return 0;
    }

    pFrame->Create(NULL, _T("IJKPlayer"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
    pFrame->CenterWindow();
    pFrame->ShowWindow(true);

    // 消息循环
    CPaintManagerUI::MessageLoop();

    // 清理
    delete pFrame;
    CPaintManagerUI::Term();
    CoUninitialize();
    Log::Finalise();

    return 0;
}


