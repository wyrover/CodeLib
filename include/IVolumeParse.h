#pragma once
#include <windows.h>

class IVolumeParse
{
public:
	virtual ~IVolumeParse()=0{};

	virtual BOOL ScanFile(LPCTSTR lpszVolumeName)=0;

	virtual BOOL ScanFileChange(LPCTSTR lpszVolume)=0;
};