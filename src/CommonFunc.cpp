#include "stdafx.h"
#include "CommonFunc.h"
#include <tlhelp32.h>

DWORD GetMainThreadID( DWORD dwPID )
{
	DWORD dwThreadID = dwPID;
	THREADENTRY32 te32 = {sizeof(te32)};
	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);

	if(Thread32First(hThreadSnap, &te32))
	{
		do
		{
			if(dwPID == te32.th32OwnerProcessID)
			{
				dwThreadID = te32.th32ThreadID;
				break;
			}
		}
		while(Thread32Next(hThreadSnap, &te32));
	}

	return dwThreadID;
}

HMODULE WINAPI ModuleFromAddress(PVOID pv) 
{
	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(pv, &mbi, sizeof(mbi)) != 0)
	{
		return (HMODULE)mbi.AllocationBase;
	}
	else
	{
		return NULL;
	}
}

