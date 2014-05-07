#include "stdafx.h"
#include "NtfsVolumeParse.h"
#include <winioctl.h>
#include <iostream>
#include "NTFSDefine.h"
#include <tchar.h>

CNtfsVolumeParse::CNtfsVolumeParse()
{

}


CNtfsVolumeParse::~CNtfsVolumeParse(void)
{
}

LONGLONG CNtfsVolumeParse::GetRecordTotalSize(LPCTSTR lpszVolume)
{
    if(NULL == lpszVolume)
        return FALSE;

    TCHAR sVolumeName[MAX_PATH] = {0};
    _stprintf_s(sVolumeName, _T("\\\\.\\%c:"), lpszVolume[0]);

    HANDLE hVolume = CreateFile(sVolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if(INVALID_HANDLE_VALUE == hVolume)
        return FALSE;

    NTFS_VOLUME_DATA_BUFFER ntfsVolData;
    DWORD dwWritten = 0;

    if(!DeviceIoControl(hVolume, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &ntfsVolData, sizeof(ntfsVolData), &dwWritten, NULL))
    {
        CloseHandle(hVolume);
        hVolume = INVALID_HANDLE_VALUE;
        return 0;
    }

    LONGLONG total_file_count = (ntfsVolData.MftValidDataLength.QuadPart / ntfsVolData.BytesPerFileRecordSegment);
    CloseHandle(hVolume);
    hVolume = INVALID_HANDLE_VALUE;
    return total_file_count;
}

BOOL CNtfsVolumeParse::ScanFile(LPCTSTR lpszVolumeName)
{
    if(NULL == lpszVolumeName)
        return FALSE;

    TCHAR sVolumeName[MAX_PATH] = {0};
    _stprintf_s(sVolumeName, _T("\\\\.\\%c:"), lpszVolumeName[0]);

    HANDLE hVolume = CreateFile(sVolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if(INVALID_HANDLE_VALUE == hVolume)
        return FALSE;

    USN_JOURNAL_DATA journalData;
    DWORD dwBytes = 0;

    if(!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &journalData, sizeof(journalData), &dwBytes, NULL))
    {
        CloseHandle(hVolume);
        hVolume = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    NTFS_VOLUME_DATA_BUFFER ntfsVolData;
    DWORD dwWritten = 0;

    BOOL bDioControl = DeviceIoControl(hVolume, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &ntfsVolData, sizeof(ntfsVolData), &dwWritten, NULL);

    if(!bDioControl)
    {
        CloseHandle(hVolume);
        hVolume = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    DWORD dwFileRecordOutputBufferSize = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + ntfsVolData.BytesPerFileRecordSegment - 1;
    PNTFS_FILE_RECORD_OUTPUT_BUFFER ntfsFileRecordOutput = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)malloc(dwFileRecordOutputBufferSize);
    LONGLONG fileTotalCount = GetRecordTotalSize(lpszVolumeName);

    for(LONGLONG i = 0; i < fileTotalCount; i++)
    {
        NTFS_FILE_RECORD_INPUT_BUFFER mftRecordInput;
        mftRecordInput.FileReferenceNumber.QuadPart = i;

        memset(ntfsFileRecordOutput, 0, dwFileRecordOutputBufferSize);

        if(!DeviceIoControl(hVolume, FSCTL_GET_NTFS_FILE_RECORD, &mftRecordInput, sizeof(mftRecordInput), ntfsFileRecordOutput, dwFileRecordOutputBufferSize, &dwWritten, NULL))
        {
            CloseHandle(hVolume);
            hVolume = INVALID_HANDLE_VALUE;
            return FALSE;
        }

        PFILE_RECORD_HEADER pfileRecordheader = (PFILE_RECORD_HEADER)ntfsFileRecordOutput->FileRecordBuffer;

        if(pfileRecordheader->Ntfs.Type != NRH_FILE_TYPE)
            continue;

        if(pfileRecordheader->Flags != FRH_IN_USE)
            continue;

        for(PATTRIBUTE pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset); pAttribute->AttributeType != -1; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length))
        {
            switch(pAttribute->AttributeType)
            {
                case AttributeFileName:
                {
                    PFILENAME_ATTRIBUTE pFileNameAttr = PFILENAME_ATTRIBUTE((PBYTE)pAttribute + PRESIDENT_ATTRIBUTE(pAttribute)->ValueOffset);

                    // 当前记录是文件
                    if(0x0001 & pfileRecordheader->Flags)
                    {
                        WCHAR sFileName[MAX_PATH] = {0};
                        swprintf_s(sFileName, L"%.*ws\r\n", pFileNameAttr->NameLength , pFileNameAttr->Name);
                        wprintf(sFileName);
                    }
                }
					break;

                case AttributeStandardInformation:
                    break;

                case AttributeAttributeList:
                    break;

                case AttributeObjectId:
                    break;
            }
        }
    }

    free(ntfsFileRecordOutput);

    CloseHandle(hVolume);
    hVolume = INVALID_HANDLE_VALUE;

    return TRUE;
}

BOOL CNtfsVolumeParse::ScanFileChange(LPCTSTR lpszVolume)
{
    if(NULL == lpszVolume)
        return FALSE;

    TCHAR sVolumeName[MAX_PATH] = {0};
    _stprintf_s(sVolumeName, _T("\\\\.\\%c:"), lpszVolume[0]);

    HANDLE hVolume = CreateFile(sVolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if(INVALID_HANDLE_VALUE == hVolume)
        return FALSE;

    USN_JOURNAL_DATA journalData;
    DWORD dwBytes = 0;

    if(!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &journalData, sizeof(journalData), &dwBytes, NULL))
    {
        CloseHandle(hVolume);
        hVolume = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    MFT_ENUM_DATA med;
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = journalData.NextUsn;

    PUSN_RECORD usnRecord;
    DWORD dwRetBytes;
    char buffer[USN_PAGE_SIZE];
    BOOL bDioControl = FALSE;
    WCHAR sFileName[USN_PAGE_SIZE] = {0};

    for(;;)
    {
        memset(buffer, 0, sizeof(USN_PAGE_SIZE));
        bDioControl = DeviceIoControl(hVolume, FSCTL_ENUM_USN_DATA, &med, sizeof(med), &buffer, USN_PAGE_SIZE, &dwBytes, NULL);

        if(!bDioControl)
        {
            CloseHandle(hVolume);
            hVolume = INVALID_HANDLE_VALUE;
            break;
        }

        if(dwBytes <= sizeof(USN)) break;

        dwRetBytes = dwBytes - sizeof(USN);
        usnRecord = (PUSN_RECORD)(((PUCHAR)buffer) + sizeof(USN));

        while(dwRetBytes > 0)
        {
            _tsetlocale(LC_ALL, _T("chs"));
            swprintf_s(sFileName, L"%.*ws\r\n", (int)(usnRecord->FileNameLength / 2), usnRecord->FileName);
            wprintf(sFileName);
            dwRetBytes -= usnRecord->RecordLength;
            usnRecord = (PUSN_RECORD)(((PCHAR)usnRecord) + usnRecord->RecordLength);
        }

        med.StartFileReferenceNumber = *(DWORDLONG*)buffer;
    }

    CloseHandle(hVolume);
    hVolume = INVALID_HANDLE_VALUE;

    return TRUE;
}
