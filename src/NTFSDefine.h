#pragma once

#include <windows.h>

#define MAXIMUM_VOLUME_LABEL_LENGTH (32 * sizeof(WCHAR))

typedef struct _BIOS_PARAMETERS_BLOCK
{
    USHORT    BytesPerSector;         // 0x0B
    UCHAR     SectorsPerCluster;      // 0x0D
    UCHAR     Unused0[7];             // 0x0E, checked when volume is mounted
    UCHAR     MediaId;                // 0x15
    UCHAR     Unused1[2];             // 0x16
    USHORT    SectorsPerTrack;        // 0x18
    USHORT    Heads;                  // 0x1A
    UCHAR     Unused2[4];             // 0x1C
    UCHAR     Unused3[4];             // 0x20, checked when volume is mounted
} BIOS_PARAMETERS_BLOCK, *PBIOS_PARAMETERS_BLOCK;

typedef struct _EXTENDED_BIOS_PARAMETERS_BLOCK
{
    USHORT    Unknown[2];             // 0x24, always 80 00 80 00
    ULONGLONG SectorCount;            // 0x28
    ULONGLONG MftLocation;            // 0x30
    ULONGLONG MftMirrLocation;        // 0x38
    CHAR      ClustersPerMftRecord;   // 0x40
    UCHAR     Unused4[3];             // 0x41
    CHAR      ClustersPerIndexRecord; // 0x44
    UCHAR     Unused5[3];             // 0x45
    ULONGLONG SerialNumber;           // 0x48
    UCHAR     Checksum[4];            // 0x50
} EXTENDED_BIOS_PARAMETERS_BLOCK, *PEXTENDED_BIOS_PARAMETERS_BLOCK;

typedef struct _BOOT_SECTOR
{
    UCHAR     Jump[3];                // 0x00
    UCHAR     OEMID[8];               // 0x03
    BIOS_PARAMETERS_BLOCK BPB;
    EXTENDED_BIOS_PARAMETERS_BLOCK EBPB;
    UCHAR     BootStrap[426];         // 0x54
    USHORT    EndSector;              // 0x1FE
} BOOT_SECTOR, *PBOOT_SECTOR;

typedef struct _NTFS_INFO
{
    ULONG BytesPerSector;
    ULONG SectorsPerCluster;
    ULONG BytesPerCluster;
    ULONGLONG SectorCount;
    ULARGE_INTEGER MftStart;
    ULARGE_INTEGER MftMirrStart;
    ULONG BytesPerFileRecord;

    ULONGLONG SerialNumber;
    USHORT VolumeLabelLength;
    WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH];
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT Flags;

} NTFS_INFO, *PNTFS_INFO;

#define NTFS_TYPE_CCB         '20SF'
#define NTFS_TYPE_FCB         '30SF'
#define NTFS_TYPE_VCB         '50SF'
#define NTFS_TYPE_IRP_CONTEST '60SF'
#define NTFS_TYPE_GLOBAL_DATA '70SF'

typedef struct
{
    ULONG Type;
    ULONG Size;
} NTFSIDENTIFIER, *PNTFSIDENTIFIER;

typedef enum
{
    AttributeStandardInformation = 0x10,
    AttributeAttributeList = 0x20,
    AttributeFileName = 0x30,
    AttributeObjectId = 0x40,
    AttributeSecurityDescriptor = 0x50,
    AttributeVolumeName = 0x60,
    AttributeVolumeInformation = 0x70,
    AttributeData = 0x80,
    AttributeIndexRoot = 0x90,
    AttributeIndexAllocation = 0xA0,
    AttributeBitmap = 0xB0,
    AttributeReparsePoint = 0xC0,
    AttributeEAInformation = 0xD0,
    AttributeEA = 0xE0,
    AttributePropertySet = 0xF0,
    AttributeLoggedUtilityStream = 0x100
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;


typedef struct
{
    ULONG Type;             /* Magic number 'FILE' */
    USHORT UsaOffset;       /* Offset to the update sequence */
    USHORT UsaCount;        /* Size in words of Update Sequence Number & Array (S) */
    ULONGLONG Lsn;          /* $LogFile Sequence Number (LSN) */
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

/* NTFS_RECORD_HEADER.Type */
#define NRH_FILE_TYPE  0x454C4946  /* 'FILE' */


typedef struct
{
    NTFS_RECORD_HEADER Ntfs;
    USHORT SequenceNumber;        /* Sequence number */
    USHORT LinkCount;             /* Hard link count */
    USHORT AttributeOffset;       /* Offset to the first Attribute */
    USHORT Flags;                 /* Flags */
    ULONG BytesInUse;             /* Real size of the FILE record */
    ULONG BytesAllocated;         /* Allocated size of the FILE record */
    ULONGLONG BaseFileRecord;     /* File reference to the base FILE record */
    USHORT NextAttributeNumber;   /* Next Attribute Id */
    USHORT Pading;                /* Align to 4 UCHAR boundary (XP) */
    ULONG MFTRecordNumber;        /* Number of this MFT Record (XP) */
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

/* Flags in FILE_RECORD_HEADER */

#define FRH_IN_USE    0x0001    /* Record is in use */
#define FRH_DIRECTORY 0x0002    /* Record is a directory */
#define FRH_UNKNOWN1  0x0004    /* Don't know */
#define FRH_UNKNOWN2  0x0008    /* Don't know */

typedef struct
{
    ATTRIBUTE_TYPE AttributeType;
    ULONG Length;
    BOOLEAN Nonresident;
    UCHAR NameLength;
    USHORT NameOffset;
    USHORT Flags;
    USHORT AttributeNumber;
} ATTRIBUTE, *PATTRIBUTE;

typedef struct
{
    ATTRIBUTE Attribute;
    ULONG ValueLength;
    USHORT ValueOffset;
    UCHAR Flags;
//  UCHAR Padding0;
} RESIDENT_ATTRIBUTE, *PRESIDENT_ATTRIBUTE;

typedef struct
{
    ATTRIBUTE Attribute;
    ULONGLONG StartVcn; // LowVcn
    ULONGLONG LastVcn; // HighVcn
    USHORT RunArrayOffset;
    USHORT CompressionUnit;
    ULONG  Padding0;
    UCHAR  IndexedFlag;
    ULONGLONG AllocatedSize;
    ULONGLONG DataSize;
    ULONGLONG InitializedSize;
    ULONGLONG CompressedSize;
} NONRESIDENT_ATTRIBUTE, *PNONRESIDENT_ATTRIBUTE;


typedef struct
{
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONG FileAttribute;
    ULONG AlignmentOrReserved[3];
#if 0
    ULONG QuotaId;
    ULONG SecurityId;
    ULONGLONG QuotaCharge;
    USN Usn;
#endif
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;


typedef struct
{
    ATTRIBUTE_TYPE AttributeType;
    USHORT Length;
    UCHAR NameLength;
    UCHAR NameOffset;
    ULONGLONG StartVcn; // LowVcn
    ULONGLONG FileReferenceNumber;
    USHORT AttributeNumber;
    USHORT AlignmentOrReserved[3];
} ATTRIBUTE_LIST, *PATTRIBUTE_LIST;


typedef struct
{
    ULONGLONG DirectoryFileReferenceNumber;
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONGLONG AllocatedSize;
    ULONGLONG DataSize;
    ULONG FileAttributes;
    ULONG AlignmentOrReserved;
    UCHAR NameLength;
    UCHAR NameType;
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

typedef struct
{
    ULONGLONG Unknown1;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT Flags;
    ULONG Unknown2;
} VOLINFO_ATTRIBUTE, *PVOLINFO_ATTRIBUTE;

