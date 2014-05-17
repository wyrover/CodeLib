#include "stdafx.h"
#include "Keyboard.h"
#include "CommonFunc.h"

HHOOK CKeyboardHook::m_hHook = NULL;
CKeyboardHook::CKeyboardHook(ICallBack* pCallback): m_pCallback(pCallback), m_pRequest(NULL)
{
    m_pRequest = new CKeyboardHookRequest;
}

CKeyboardHook::~CKeyboardHook()
{
    if(NULL != m_pRequest)
        delete m_pRequest;

    m_pRequest = NULL;
}

BOOL CKeyboardHook::Install(DWORD dwPID)
{
    if(NULL == m_hHook)
    {
        if(-1 != dwPID)
        {
            m_hHook =::SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)_HookProc, ModuleFromAddress(_HookProc), GetMainThreadID(dwPID));
        }
        else
            m_hHook =::SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)_HookProc, ModuleFromAddress(_HookProc), 0);
    }

    PrintDebugInfo(_T("Keyboard Install\r\n"));

    return (NULL != m_hHook);
}

BOOL CKeyboardHook::UnInstall()
{
    BOOL bRet = FALSE;

    if(NULL != m_hHook)
    {
        bRet =::UnhookWindowsHookEx(m_hHook);
        m_hHook = NULL;
    }

    PrintDebugInfo(_T("Keyboard UnInstall\r\n"));
    return bRet;
}

LRESULT CALLBACK CKeyboardHook::_HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode < 0 || nCode == HC_NOREMOVE)
        return ::CallNextHookEx(m_hHook, nCode, wParam, lParam);

    if(lParam & 0x40000000)     // Check the previous key state
    {
        return ::CallNextHookEx(m_hHook, nCode, wParam, lParam);
    }

    return ::CallNextHookEx(m_hHook, nCode, wParam, lParam);
}

void CKeyboardHook::PrintDebugInfo(LPCTSTR lpszInfo)
{
    m_pRequest->SetDebugInfo(lpszInfo);

    if(NULL != m_pCallback)
        m_pCallback->HandleRequest(m_pRequest);
}

CKeyboardHookRequest::CKeyboardHookRequest()
{
    ZeroMemory(m_szDebugInfo, sizeof(m_szDebugInfo));
}

CKeyboardHookRequest::~CKeyboardHookRequest()
{

}

REQUEST_TYPE CKeyboardHookRequest::GetType()
{
    return REQUEST_KEYBOARDHOOK;
}

void CKeyboardHookRequest::SetDebugInfo(LPCTSTR lpszInfo)
{
    _tcscpy_s(m_szDebugInfo, sizeof(m_szDebugInfo) / sizeof(TCHAR), lpszInfo);
}

LPCTSTR CKeyboardHookRequest::GetDebugInfo()
{
    return m_szDebugInfo;
}
