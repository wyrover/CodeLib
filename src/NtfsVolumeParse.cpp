#include "stdafx.h"
#include "NtfsVolumeParse.h"
#include <winioctl.h>
#include <iostream>
//#include "NTFS.h"

typedef struct _MULTI_SECTOR_HEADER
{
    UCHAR  Signature[4];
    USHORT UpdateSequenceArrayOffset;
    USHORT UpdateSequenceArraySize;

} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;

typedef struct _MFT_SEGMENT_REFERENCE
{
    ULONG  SegmentNumberLowPart;
    USHORT SegmentNumberHighPart;
    USHORT SequenceNumber;

} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;

typedef struct _UPDATE_SEQUENCE_ARRAY
{
    USHORT updateSequenceID;
    USHORT updateSequence1;
    USHORT updateSequence2;

} UPDATE_SEQUENCE_ARRAY, *PUPDATE_SEQUENCE_ARRAY;

typedef struct _FILE_RECORD_SEGMENT_HEADER
{
    MULTI_SECTOR_HEADER MultiSectorHeader;
    USN Lsn; /* $LogFile Sequence Number (LSN) */
    USHORT SequenceNumber; /* Sequence number */
    USHORT LinkCount; /* Hard link count */
    USHORT FirstAttributeOffset; /* Offset to the first attribute resident in the current file record. */
    USHORT Flags; /* Flags: 0x01 = in use, 0x02 = name index = directory, 0x04 or 0x08 = unknown. */
    ULONG BytesInUse; /* Used size for this FILE record */
    ULONG BytesAllocated; /* Total allocated size for this FILE record */
    MFT_SEGMENT_REFERENCE BaseFileRecordSegment; /* File reference to the base FILE record */
    USHORT NextAttributeNumber;
    USHORT Padding; /* for alignment to next 32-bit boundary */
    PUPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} FILE_RECORD_SEGMENT_HEADER, *PFILE_RECORD_SEGMENT_HEADER;

typedef enum
{
    //诸如只读、存档等文件属性；
    //时间戳：文件创建时、最后一次修改时；
    //多少目录指向该文件（硬链接计数hard link count）
    AttributeStandardInformation = 0x10, //Resident_Attributes 常驻属性

    //?????????????????????????????????
    //当一个文件要求多个MFT文件记录时 会有该属性
    //属性列表，包括构成该文件的这些属性，以及每个属性所在的MFT文件记录的文件引用
    //?????????????????????????????????
    AttributeAttributeList = 0x20,//由于属性值可能会增长，可能是非驻留属性

    //文件名属性可以有多个：
    //1.长文件名自动为其短文件名(以便MS-DOS和16位程序访问)
    //2.当该文件存在硬链接时
    AttributeFileName = 0x30, //常驻

    //一个文件或目录的64字节标识符，其中低16字节对于该卷来说是唯一的
    //链接-跟踪服务将对象ID分配给外壳快捷方式和OLE链接源文件。
    //NTFS提供了相应的API，因为文件和目录可以通过其对象ID，而不是通过其文件名打开
    AttributeObjectId = 0x40, //常驻

    //为与NTFS以前版本保持向后兼容
    //所有具有相同安全描述符的文件或目录共享同样的安全描述
    //以前版本的NTFS将私有的安全描述符信息与每个文件和目录存储在一起
    AttributeSecurityDescriptor = 0x50,//出现于$Secure元数据文件中

    //保存了该卷的版本和label信息
    AttributeVolumeName = 0x60, //仅出现于$Volume元数据文件中
    AttributeVolumeInformation = 0x70,//仅出现于$Volume元数据文件中

    //文件内容，一个文件仅有一个未命名的数据属性，但可有额外多个命名数据属性
    //即一个文件可以有多个数据流，目录没有默认的数据属性，但可有多个可选的命名的数据属性
    AttributeData = 0x80,//由于属性值可能会增长，可能是非驻留属性

    //以下三个用于实现大目录的文件名分配和位图索引
    AttributeIndexRoot = 0x90,//常驻
    AttributeIndexAllocation = 0xA0,
    AttributeBitmap = 0xB0,

    //存储了一个文件的重解析点数据，NTFS的交接(junction)和挂载点包含此属性
    AttributeReparsePoint = 0xC0,

    //以下两个为扩展属性，现已不再被主动使用，之所以提供是为与OS/2程序保持向后兼容
    AttributeEAInformation = 0xD0,
    AttributeEA = 0xE0,

    AttributePropertySet = 0xF0,
    AttributeLoggedUtilityStream = 0x100,
    AttributeEnd = 0xFFFFFFFF
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct
{
    ATTRIBUTE_TYPE AttributeType;
    ULONG Length; //本属性长度（包含属性值）
    BOOLEAN Nonresident; //本属性不是 驻留 属性么？
    UCHAR NameLength; //属性名的名称长度
    USHORT NameOffset;//属性名偏移
    USHORT Flags; // 0x0001 压缩 0x4000 加密 0x8000稀疏文件
    USHORT AttributeNumber;
} ATTRIBUTE, *PATTRIBUTE;

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

BOOL CNtfsVolumeParse::ScanVolume(LPCTSTR lpszVolumeName)
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

        PFILE_RECORD_SEGMENT_HEADER pfileRecordheader = (PFILE_RECORD_SEGMENT_HEADER)ntfsFileRecordOutput->FileRecordBuffer;

        UCHAR* headerSignature = pfileRecordheader->MultiSectorHeader.Signature;

        if(headerSignature[0] != 'F' || headerSignature[1] != 'I' || headerSignature[2] != 'L' || headerSignature[3] != 'E')
            continue;

        for(PATTRIBUTE pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->FirstAttributeOffset); pAttribute->AttributeType != -1; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length))
        {

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
