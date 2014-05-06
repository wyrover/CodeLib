#pragma once
#include "IVolumeParse.h"
class CCdfsVolumeParse : public IVolumeParse
{
public:
	CCdfsVolumeParse(void);
	virtual ~CCdfsVolumeParse(void);

	virtual BOOL ScanVolume( LPCTSTR lpszVolumeName );

	virtual BOOL ScanFileChange( LPCTSTR lpszVolume );

};

