#pragma once
#include <windows.h>

class CSparseFiles
{
public:
	CSparseFiles();
	virtual ~CSparseFiles();
	static BOOL VolumeSupportsSparseFiles(LPCTSTR lpRootPathName);
	static BOOL IsSparseFile(LPCTSTR lpFileName);
	static BOOL GetSparseFileSize(LPCTSTR lpFileName);
	static HANDLE CreateSparseFile(LPCTSTR lpFileName);
	static void SetSparseRange(HANDLE hSparseFile, LONGLONG start, LONGLONG size);
	static BOOL GetSparseRanges(LPCTSTR lpFileName);
};