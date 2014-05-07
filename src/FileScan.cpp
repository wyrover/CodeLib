#include "stdafx.h"
#include "FileScan.h"
#include "NtfsVolumeParse.h"
#include "Fat32VolumeParse.h"
#include "Fat16VolumeParse.h"
#include "CdfsVolumeParse.h"


CFileScan::CFileScan(): m_dwFileTotalNum(0)
{

}

CFileScan::~CFileScan()
{

}

BOOL CFileScan::_GetLocalFixedDriver(std::vector<std::wstring>& vecDriver)
{
    DWORD dwDriverNameBufLen = GetLogicalDriveStrings(0, NULL);
    TCHAR* sDriverName = new TCHAR[dwDriverNameBufLen + 1];
    ZeroMemory(sDriverName, dwDriverNameBufLen + 1);
    GetLogicalDriveStrings(dwDriverNameBufLen, sDriverName);

    for(TCHAR *p = sDriverName; *p != '\0'; p += 4)
    {
        if(*p >= L'a') *p -= 32;

        if(DRIVE_FIXED == GetDriveTypeW(p))
        {
            TCHAR szVolumePath[10] = {0};
            swprintf_s(szVolumePath, _T("%c:\\"), *p);
//            swprintf_s(szVolumePath, L"\\\\.\\%c:", *p);
            vecDriver.push_back(szVolumePath);
        }

    }

    delete sDriverName;
    sDriverName = NULL;
    return TRUE;
}
//
// BOOL CFileScan::_ScanFileFromDir(LPCTSTR lpszDir)
// {
//     WIN32_FIND_DATA findData = {0};
//
//     TCHAR szFindFileName[MAX_PATH] = {0};
//     _tcscpy_s(szFindFileName, lpszDir);
//     _tcscat_s(szFindFileName, _T("\\"));
//     _tcscat_s(szFindFileName, _T("*"));
//     BOOL bFind = TRUE;
//     HANDLE hFileFind = NULL;
//
//     for(hFileFind = FindFirstFile(szFindFileName, &findData) ;
//             bFind && hFileFind != INVALID_HANDLE_VALUE;
//             bFind = FindNextFile(hFileFind, &findData))
//     {
//         if(_tcsicmp(_T("."), findData.cFileName) == 0 ||
//                 _tcsicmp(_T(".."), findData.cFileName) == 0)
//         {
//             continue;
//         }
//
//         TCHAR szFullPaht[MAX_PATH] = {0};
//         _tcscpy_s(szFullPaht, lpszDir);
//         _tcscat_s(szFullPaht, _T("\\"));
//         _tcscat_s(szFullPaht,  findData.cFileName);
//
//         //如果是目录
//         if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//         {
//             _ScanFileFromDir(szFullPaht);
//         }
//         else
//         {
//             m_dwFileTotalNum++;
//         }
//     }
//
//     FindClose(hFileFind);
//     return S_OK;
// }

DWORD CFileScan::GetVolumeNum()
{
    return m_vecDriver.size();
}

VOLUME_FS_TYPE CFileScan::GetVolumeFS(LPCTSTR lpszVolume)
{
    VOLUME_FS_TYPE volumeFs = FS_UKNOWN;
    TCHAR sysNameBuf[MAX_PATH] = {0};
    int status = GetVolumeInformation(lpszVolume,  NULL, 0,  NULL,  NULL,  NULL,  sysNameBuf, MAX_PATH);

    if(0 == _tcscmp(sysNameBuf, _T("NTFS")))
    {
        volumeFs = FS_NTFS;
    }
    else if(0 == _tcscmp(sysNameBuf, _T("FAT")))
    {
        volumeFs = FS_FAT32;
    }

    return volumeFs;
}

BOOL CFileScan::ExecCommand(VOLUME_FS_CMD fsCmd)
{
    BOOL bRet = FALSE;
    VecDriver sysDriver;

    if(_GetLocalFixedDriver(sysDriver))
    {
        for(VecDriver::const_iterator cit = sysDriver.begin(); cit != sysDriver.end(); cit++)
        {
            std::wstring sDir = *cit;

            if(!sDir.empty())
                bRet = _ExecCommand(sDir.c_str(), fsCmd);
        }
    }

    return bRet;
}

BOOL CFileScan::_ExecCommand(LPCTSTR lpszVolumeName, VOLUME_FS_CMD fsCmd)
{
    VOLUME_FS_TYPE volumeFs = GetVolumeFS(lpszVolumeName);
    IVolumeParse* pVolumeParse = NULL;

    switch(volumeFs)
    {
        case FS_NTFS:
            pVolumeParse = new CNtfsVolumeParse;
            break;

        case FS_FAT32:
            pVolumeParse = new CFat32VolumeParse;

        case FS_FAT16:
            pVolumeParse = new CFat16VolumeParse;
            break;

        case FS_CDFS:
            pVolumeParse = new CCdfsVolumeParse;
            break;

        default:
            break;
    }

    if(NULL == pVolumeParse)
        return FALSE;

    BOOL bRet = FALSE;

    switch(fsCmd)
    {
        case VOLUME_FS_SCANFILE:
            bRet = pVolumeParse->ScanFile(lpszVolumeName);
            break;

        case VOLUME_FS_SCANFILECHANGE:
            bRet = pVolumeParse->ScanFileChange(lpszVolumeName);
            break;

        default:
            break;
    }

    if(NULL != pVolumeParse)
        delete pVolumeParse;

    pVolumeParse = NULL;

    return bRet;
}
