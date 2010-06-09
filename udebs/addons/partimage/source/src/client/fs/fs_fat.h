/***************************************************************************
                          cfatpart.h  -  description
                             -------------------
    begin                : Thu Jul 13 2000
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

#ifndef FS_FAT_H
#define FS_FAT_H

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

#define MAX_BOOT_SIZE		      4096
#define fat32to28(A) (A & 0x0FFFFFFF)

#define LOGICAL_FAT_BLKSIZE 512 // the size of the abstract block used in CFSBase

// ================================================
struct CFatFsInfoSector
{
    DWORD       dwMagic;          // Magic for info sector ('RRaA') 
    BYTE        junk[0x1dc];
    DWORD       reserved1;      // Nothing as far as I can tell 
    DWORD       signature;      // 0x61417272 ('rrAa') 
    DWORD       dwFreeClusters;  // Free cluster count.  -1 if unknown 
    DWORD       dwNextCluster;   // Most recently allocated cluster. 
    DWORD       reserved2[3];
    WORD       reserved3;
    WORD       boot_sign;
};

// ================================================
struct CInfoFatHeader // size must be 16384 (adjust the reserved data)
{
  DWORD dwTotalSectorsCount;
  DWORD dwClustersCount;
  DWORD dwRootDirSectors;
  DWORD dwRootEntriesCount;
  DWORD dwSectorsPerFAT;
  DWORD dwDataSectors;
  DWORD dwFileSystem;
  DWORD dwUsedClusters;
  DWORD dwDamagedClusters;
  DWORD dwFreeClusters;
  DWORD dwBytesPerFatEntry;
  DWORD dwReserved;		// reserved to align data
  WORD wBytesPerSector;
  WORD wReservedSectorsCount;
  WORD wRootEntriesCount;
  BYTE cSectorsPerCluster;
  BYTE cNumberOfFATs;
  WORD wFsInfoSector;
  BYTE cReserved[16326]; // Adjust to fit with total header size
};

// ================================================
class CFatPart: public CFSBase
{
 public:
  CFatPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CFatPart();
  
  static  int detect(BYTE *cBootSect, char *szFileSystem); 
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

  void compareFatCopies();
  int readFSInfo(WORD wFsInfoSector);

  //int checkFatDamagedClusters(DWORD *dwDamagedClusters, DWORD *dwUsedClusters, DWORD *dwFreeClusters);
  
 private:
  CInfoFatHeader m_info;
};

#endif // FS_FAT_H
