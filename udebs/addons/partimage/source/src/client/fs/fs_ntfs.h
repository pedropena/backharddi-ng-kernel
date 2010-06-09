/***************************************************************************
                          cntfspart.h  -  description
                             -------------------
    begin                : Mon May 22 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FS_NTFS_H
#define FS_NTFS_H

#include "partimage.h"
#include "common.h"
#include "cbitmap.h"

#include "fs_base.h"

struct CImage;
struct CVolumeHeader;
struct CMainTail;
struct CMainHeader;
struct COptions;
class CSavingWindow;
class CRestoringWindow;

// format version of the ntfs image
#define CURRENT_NTFS_FORMAT "0.1"
#define MAX_NTFS_BLOCK_SIZE 8192

// for BSD 
#if !defined(__LITTLE_ENDIAN) && !defined(__BIG_ENDIAN)
  #if BYTE_ORDER == LITTLE_ENDIAN
    #define __LITTLE_ENDIAN
  #endif
  #if BYTE_ORDER == BIG_ENDIAN
    #define __BIG_ENDIAN
  #endif
#endif

#ifdef __LITTLE_ENDIAN

#define CPU_TO_LE16(a) (a)
#define CPU_TO_LE32(a) (a)
#define CPU_TO_LE64(a) (a)
#define LE16_TO_CPU(a) (a)
#define LE32_TO_CPU(a) (a)
#define LE64_TO_CPU(a) (a)

#else

/* Routines for big-endian machines */
#ifdef __BIG_ENDIAN

/* We hope its big-endian, not PDP-endian :) */
#define CPU_TO_LE16(a) ((((a)&0xFF) << 8)|((a) >> 8))
#define CPU_TO_LE32(a) ((((a) & 0xFF) << 24) | (((a) & 0xFF00) << 8) | \
			(((a) & 0xFF0000) >> 8) | ((a) >> 24))
#define CPU_TO_LE64(a) ((CPU_TO_LE32(a)<<32)|CPU_TO_LE32((a)>>32)

#define LE16_TO_CPU(a) CPU_TO_LE16(a)
#define LE32_TO_CPU(a) CPU_TO_LE32(a)
#define LE64_TO_CPU(a) CPU_TO_LE64(a)

#else
#error "no __LITTLE_ENDIAN or __BIG_ENDIAN DEFINED"
#endif
#endif

typedef BYTE  ntfs_u8;
typedef WORD ntfs_u16;
typedef DWORD ntfs_u32;
typedef QWORD ntfs_u64;
typedef signed char ntfs_s8;
typedef signed short ntfs_s16;
typedef signed long ntfs_s32;
typedef signed long long ntfs_s64;

#define NTFS_GETU8(p)      (*(ntfs_u8*)(p))
#define NTFS_GETU16(p)     ((ntfs_u16)LE16_TO_CPU(*(ntfs_u16*)(p)))
#define NTFS_GETU24(p)     ((ntfs_u32)NTFS_GETU16(p) | ((ntfs_u32)NTFS_GETU8(((char*)(p))+2)<<16))
#define NTFS_GETU32(p)     ((ntfs_u32)LE32_TO_CPU(*(ntfs_u32*)(p)))
#define NTFS_GETU40(p)     ((ntfs_u64)NTFS_GETU32(p)|(((ntfs_u64)NTFS_GETU8(((char*)(p))+4))<<32))
#define NTFS_GETU48(p)     ((ntfs_u64)NTFS_GETU32(p)|(((ntfs_u64)NTFS_GETU16(((char*)(p))+4))<<32))
#define NTFS_GETU56(p)     ((ntfs_u64)NTFS_GETU32(p)|(((ntfs_u64)NTFS_GETU24(((char*)(p))+4))<<32))
#define NTFS_GETU64(p)     ((ntfs_u64)LE64_TO_CPU(*(ntfs_u64*)(p)))


#define NTFS_GETS8(p)        ((*(ntfs_s8*)(p)))
#define NTFS_GETS16(p)       ((ntfs_s16)LE16_TO_CPU(*(short*)(p)))
#define NTFS_GETS24(p)       (NTFS_GETU24(p) < 0x800000 ? (int)NTFS_GETU24(p) : (int)(NTFS_GETU24(p) - 0x1000000))
#define NTFS_GETS32(p)       ((ntfs_s32)LE32_TO_CPU(*(int*)(p)))
#define NTFS_GETS40(p)       (((ntfs_s64)NTFS_GETU32(p)) | (((ntfs_s64)NTFS_GETS8(((char*)(p))+4)) << 32))
#define NTFS_GETS48(p)       (((ntfs_s64)NTFS_GETU32(p)) | (((ntfs_s64)NTFS_GETS16(((char*)(p))+4)) << 32))
#define NTFS_GETS56(p)       (((ntfs_s64)NTFS_GETU32(p)) | (((ntfs_s64)NTFS_GETS24(((char*)(p))+4)) << 32))
#define NTFS_GETS64(p)	     ((ntfs_s64)NTFS_GETU64(p))

// ================================================
class CNtfsRunList
{
 private:
  QWORD *m_qwOffset;
  QWORD m_qwClustersCount;
  
 public:
  CNtfsRunList();
  ~CNtfsRunList();
  
 public:
  int setCluster(QWORD qwNumber, QWORD qwOffset); // ex: cluser 0 position is 12589
  int init(QWORD qwClustersCount);
  void freeMemory();
  void show();
  QWORD offset(QWORD qwCluster);
  QWORD clustersCount();
};

// ================================================
struct CInfoNtfsHeader // size must be 16384 (adjust the reserved data)
{
  QWORD qwTotalSectorsCount;
  QWORD qwLCNOfMftDataAttrib;
  DWORD dwFileRecordSize;
  DWORD dwClusterSize;
  
  WORD nBytesPerSector;
  WORD nNtfsVersion; 
  BYTE cSectorsPerCluster;

  BYTE cReserved1[3];
  BYTE cReserved[16352]; // Adjust to fit with total header size
};

// ================================================
class CNtfsPart : public CFSBase
{
 public:
  CNtfsPart(char *szDevice, FILE *fDeviceFilek, QWORD qwPartSize);
  ~CNtfsPart();

  static  int detect(BYTE *cBootSect, char *szFileSystem); 
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

  // file reading options
  int readDataAttribute(BYTE *cAttribData, CNtfsRunList *runlist, QWORD *qwBitmapSize, QWORD *qwAllocatedSize);
  int readFilenameAttribute(BYTE *cAttribData, char *szFilename);
  int readFileRecord(BYTE *cRecordData, CNtfsRunList *runlist, QWORD *qwBitmapSize, QWORD *qwAllocatedSize);
  int processRunList(BYTE *cAttribData, CNtfsRunList *runlist, QWORD *qwAllocatedSize, QWORD *qwRealSize);
  int searchForRecordFilename(BYTE *cFileRecord, char *szFilename, char *szUnicodeName, int nUnicodeNameLen);
  int checkFilenameForRecordIs(BYTE *cRecordData, char *szFilename, bool bShowDebug = true);
  int readVolumeLabel();

private:
  // informations
  CInfoNtfsHeader m_info;
};

#endif // FS_NTFS_H
