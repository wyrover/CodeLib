#include "stdafx.h"
#include "NTFS.h"

#include "ntfs.h"
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
using namespace std;
//管理员权限运行
typedef struct
{
    ULONGLONG index;
    ULONGLONG parent;
    WCHAR name[1024];
    int type; //0 file 1 dir
} FILE_INFO, *PFILE_INFO;

//枚举盘符
void OpenNtfsVolume()
{
    WCHAR tDrivers[26 * 4 + 1] = {};
    GetLogicalDriveStrings(26 * 4 + 1, tDrivers);
    WCHAR fileSysBuf[8];
    DWORD dwDri; //0~25

    WCHAR szRootName[40];
    WCHAR szVolumeName[32];
    int iFilterRoot = 0;

    for(WCHAR *p = tDrivers; *p != '\0'; p += 4)
    {
        if(*p >= L'a') *p -= 32; //

        dwDri = *p - L'A';

        if(DRIVE_FIXED == GetDriveTypeW(p))
        {
            DWORD dwMaxComLen, dwFileSysFlag;
            GetVolumeInformationW(p, szVolumeName, 32, NULL, &dwMaxComLen, &dwFileSysFlag, fileSysBuf, 8);

            if(fileSysBuf[0] == L'N' && fileSysBuf[1] == L'T' && fileSysBuf[2] == L'F' && fileSysBuf[3] == L'S')
            {
                swprintf_s(szRootName, L"%s (%c:)", szVolumeName, *p);
                WCHAR szVolumePath[10];
                swprintf_s(szVolumePath, L"\\\\.\\%c:", *p);
                wcout << szVolumePath << endl;
            }
        }
    }
}

int main()
{
    //wcout.imbue(locale(locale(),"",LC_CTYPE));
    char szACP[16];
    sprintf(szACP, ".%d", GetACP());
    //setlocale(LC_CTYPE, szACP);
    wcout.imbue(locale(locale(), szACP, LC_CTYPE));
    OpenNtfsVolume();

    WCHAR driveletter[] = L"\\\\.\\F:";
    HANDLE hVol = CreateFile(driveletter, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    NTFS_VOLUME_DATA_BUFFER ntfsVolData;
    DWORD dwWritten = 0;

    BOOL bDioControl = DeviceIoControl(hVol, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &ntfsVolData, sizeof(ntfsVolData), &dwWritten, NULL);

    if(!bDioControl)
    {
        wcout << L"error" << endl;
        return 1;
    }

    LARGE_INTEGER num;
    num.QuadPart = 1024;
    LONGLONG total_file_count = (ntfsVolData.MftValidDataLength.QuadPart / num.QuadPart);
    wcout << L"total_file_count:" << total_file_count << endl;


    PNTFS_FILE_RECORD_OUTPUT_BUFFER ntfsFileRecordOutput = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)malloc(sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + ntfsVolData.BytesPerFileRecordSegment - 1);

    for(LONGLONG i = 0; i < total_file_count && i < 10; i++)
    {
        NTFS_FILE_RECORD_INPUT_BUFFER mftRecordInput;
        mftRecordInput.FileReferenceNumber.QuadPart = i;
        memset(ntfsFileRecordOutput, 0, sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + ntfsVolData.BytesPerFileRecordSegment - 1);
        DeviceIoControl(hVol, FSCTL_GET_NTFS_FILE_RECORD, &mftRecordInput, sizeof(mftRecordInput), ntfsFileRecordOutput, sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + ntfsVolData.BytesPerFileRecordSegment - 1, &dwWritten, NULL);
        PFILE_RECORD_HEADER pfileRecordheader = (PFILE_RECORD_HEADER)ntfsFileRecordOutput->FileRecordBuffer;

        if(pfileRecordheader->Ntfs.Type != 'ELIF')
            continue;

        for(PATTRIBUTE pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset); pAttribute->AttributeType != -1; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length))
        {
            switch(pAttribute->AttributeType)
            {
                case AttributeFileName:
                {
                    if((0x0002 & pfileRecordheader->Flags) && (0x0001 & pfileRecordheader->Flags))
						PFILENAME_ATTRIBUTE pFileNameAttr = PFILENAME_ATTRIBUTE((PBYTE)pAttribute + PRESIDENT_ATTRIBUTE(pAttribute)->ValueOffset);
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

    USN_JOURNAL_DATA journalData;
    READ_USN_JOURNAL_DATA readData = {0, 0xFFFFFFFF, FALSE, 0, 0};

    PUSN_RECORD usnRecord;
    DWORD dwBytes;
    DWORD dwRetBytes;
    char buffer[USN_PAGE_SIZE];
    bDioControl = DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &journalData, sizeof(journalData), &dwBytes, NULL);

    if(!bDioControl)
    {
        wcout << L"error" << endl;
        return 1;
    }

    readData.UsnJournalID = journalData.UsnJournalID;
    wprintf(L"Journal ID: %I64x\n", journalData.UsnJournalID);
    wprintf(L"FirstUsn: %I64x\n\n", journalData.FirstUsn);

    MFT_ENUM_DATA med;
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = journalData.NextUsn;

    map<ULONGLONG, wstring> namemap;
    map<ULONGLONG, ULONGLONG> frnmap;
    vector<FILE_INFO> fileVect;

    for(;;)
    {
        memset(buffer, 0, sizeof(USN_PAGE_SIZE));
        //FSCTL_ENUM_USN_DATA FSCTL_READ_USN_JOURNAL
        //DeviceIoControl( hVol, FSCTL_READ_USN_JOURNAL, &readData,sizeof(readData),&buffer,USN_PAGE_SIZE,&dwBytes,NULL);
        bDioControl = DeviceIoControl(hVol, FSCTL_ENUM_USN_DATA, &med, sizeof(med), &buffer, USN_PAGE_SIZE, &dwBytes, NULL);

        if(!bDioControl)
        {
            wcout << L"error" << endl;
            break;
        }

        if(dwBytes <= sizeof(USN)) break; //结束!

        dwRetBytes = dwBytes - sizeof(USN);//跳过了1个USN，此USN是下一论查询起点
        usnRecord = (PUSN_RECORD)(((PUCHAR)buffer) + sizeof(USN));

        while(dwRetBytes > 0)
        {
            FILE_INFO fi;
            fi.index = usnRecord->FileReferenceNumber;
            fi.parent = usnRecord->ParentFileReferenceNumber;

            //wprintf(L"USN: %I64x\n", usnRecord->Usn );
            //wprintf(L"File name: %.*s\n", (int)(usnRecord->FileNameLength/2),usnRecord->FileName );
            //wprintf(L"Reason: %x\n", usnRecord->Reason );

            memset(fi.name, 0, sizeof(fi.name));
            swprintf_s(fi.name, L"%.*ws", (int)(usnRecord->FileNameLength / 2), usnRecord->FileName);
            namemap[fi.index] = wstring(fi.name);
            fileVect.push_back(fi);
            frnmap[fi.index] = fi.parent;

            dwRetBytes -= usnRecord->RecordLength;
            usnRecord = (PUSN_RECORD)(((PCHAR)usnRecord) + usnRecord->RecordLength);
        }

        med.StartFileReferenceNumber = *(DWORDLONG*)buffer;
        //readData.StartUsn = *(USN *)&buffer;
    }

    wcout << fileVect.size() << endl;
    map<ULONGLONG, wstring>::iterator it;
    /*for ( it=namemap.begin() ; it != namemap.end(); it++ )
    {
    wcout << (*it).first << " => " << ((*it).second).c_str() <<"==>"<<namemap[(*it).first].c_str()<< endl;
    }*/

    //wofstream ofs(L"data.txt");
    for(LONGLONG i = 0; i < fileVect.size(); i++)
    {
        FILE_INFO fi = fileVect[i];
        wcout << i << ": " << fi.name << "===>";
        ULONGLONG parent = fi.parent;

        while(namemap.find(parent) != namemap.end())
        {
            wstring dir = namemap[parent];
            wcout << L"\\" << dir.c_str();

            if(frnmap.find(parent) != frnmap.end())
            {
                parent = frnmap[parent];
            }
            else
            {
                break;
            }
        }

        wcout << endl;
    }

    wcout << "done" << endl;
    CloseHandle(hVol);
    return 0;
}

/*typedef struct {

DWORD RecordLength;
WORD   MajorVersion;
WORD   MinorVersion;
DWORDLONG FileReferenceNumber;
DWORDLONG ParentFileReferenceNumber;
USN Usn;
LARGE_INTEGER TimeStamp;
DWORD Reason;
DWORD SourceInfo;
DWORD SecurityId;
DWORD FileAttributes;
WORD   FileNameLength;
WORD   FileNameOffset;
WCHAR FileName[1];

} USN_RECORD, *PUSN_RECORD;*/
