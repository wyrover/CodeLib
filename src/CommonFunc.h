#pragma once
#include <windows.h>


//************************************
// Method:    获取主线程ID
// FullName:  GetMainThreadID
// Access:    public 
// Returns:   DWORD
// Qualifier:
// Parameter: DWORD dwPID
//************************************
DWORD GetMainThreadID(DWORD dwPID);
HMODULE WINAPI ModuleFromAddress(PVOID pv);