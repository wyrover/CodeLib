#pragma once
#include <windows.h>
#include "ICallBack.h"

// 该类一般应用于DLL中

class CKeyboardHookRequest : public IRequest
{
public:
    CKeyboardHookRequest();
    virtual ~CKeyboardHookRequest();
    virtual REQUEST_TYPE GetType();
    virtual LPCTSTR GetDebugInfo();
protected:
    void SetDebugInfo(LPCTSTR lpszInfo);
private:
    friend class CKeyboardHook;
    TCHAR m_szDebugInfo[MAX_PATH];
};

class CKeyboardHook
{
public:
    CKeyboardHook(ICallBack* pCallback);
    virtual ~CKeyboardHook();

    BOOL Install(DWORD dwPID);
    BOOL UnInstall();
protected:
	void PrintDebugInfo(LPCTSTR lpszInfo);
	static LRESULT CALLBACK _HookProc(int nCode, WPARAM wParam, LPARAM lParam);
private:
    ICallBack* m_pCallback;
    CKeyboardHookRequest* m_pRequest;
	static HHOOK m_hHook;
};