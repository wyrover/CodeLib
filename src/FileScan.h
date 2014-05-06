#pragma once
#include <windows.h>
#include <vector>

// 文件系统类型
typedef enum VOLUME_FS_TYPE
{
	FS_UKNOWN,
	FS_NTFS,				
	FS_FAT32,
	FS_FAT16,
	FS_CDFS
};

typedef enum VOLUME_FS_CMD
{
	VOLUME_FS_SCANFILE,
	VOLUME_FS_SCANFILECHANGE
};

class CFileScan
{
public:
	CFileScan();
	virtual ~CFileScan();

	BOOL ExecCommand(VOLUME_FS_CMD fsCmd);

protected:

	BOOL _ExecCommand(LPCTSTR lpszVolumeName,VOLUME_FS_CMD fsCmd);

	// 获取本地磁盘个数
	DWORD GetVolumeNum();

	// 获取卷的文件系统格式
	VOLUME_FS_TYPE GetVolumeFS(LPCTSTR lpszVolume);

	BOOL _GetLocalFixedDriver(std::vector<std::wstring>& vecDriver);
private:
	typedef std::vector<std::wstring> VecDriver;
	VecDriver m_vecDriver;
	DWORD m_dwFileTotalNum;
};