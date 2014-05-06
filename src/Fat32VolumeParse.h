#pragma once
#include "IVolumeParse.h"

class CFat32VolumeParse : public IVolumeParse
{
public:
	CFat32VolumeParse(void);
	virtual ~CFat32VolumeParse(void);

	virtual BOOL ScanVolume( LPCTSTR lpszVolumeName );

	virtual BOOL ScanFileChange( LPCTSTR lpszVolume );

};

