#pragma once
#include <windows.h>
#include "IVolumeParse.h"

class CNtfsVolumeParse : public IVolumeParse
{
public:
	CNtfsVolumeParse();
	virtual ~CNtfsVolumeParse(void);

	virtual BOOL ScanVolume( LPCTSTR lpszVolumeName );

	virtual BOOL ScanFileChange(LPCTSTR lpszVolume);

protected:

	LONGLONG GetRecordTotalSize(LPCTSTR lpszVolume);
};

