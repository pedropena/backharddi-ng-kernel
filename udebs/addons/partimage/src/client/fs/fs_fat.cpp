/***************************************************************************
     cfatpart.cpp  -  FAT (File Allocation Table DOS) File System Support
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

#include "fs_fat.h"
#include "partimage.h"

#include <string.h>


// =======================================================
CFatPart::CFatPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CFatPart::~CFatPart()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CFatPart::fsck()
{
  BEGIN;

  try { compareFatCopies(); }
  catch ( CExceptions * excep )
    { throw excep; }

  RETURN;
}

// =======================================================
// -1: error
// false: no differences between FATs
// true: differences between FATs
void CFatPart::compareFatCopies()
{	
  BEGIN;

  int nResult;
  int nRes;
  DWORD dwReservedBytes;
  const DWORD dwBufSize = 256*1024;
  char cCompBuf1[dwBufSize];
  char cCompBuf2[dwBufSize];
  QWORD qwFatSize;
  QWORD qwPtr1, qwPtr2;
  DWORD dwTemp;
  QWORD qwToDo;
  
  if (m_info.cNumberOfFATs < 2)
    RETURN;

  // init
  nResult = 0;
  dwReservedBytes = m_info.wReservedSectorsCount * m_info.wBytesPerSector;
  
  if (m_info.dwFileSystem == FS_FAT16) // if FAT16
    qwFatSize = (QWORD)m_info.dwClustersCount*2LL;
  else if (m_info.dwFileSystem == FS_FAT32) // if FAT32
    qwFatSize = (QWORD)m_info.dwClustersCount*4LL;
  else
    THROW(ERR_WRONG_FS);
  
  qwPtr1 = (QWORD)dwReservedBytes;
  qwPtr2 = (QWORD)dwReservedBytes+(m_info.dwSectorsPerFAT*m_info.wBytesPerSector);
  qwToDo = qwFatSize;

  while (qwToDo > 0)
    {
      dwTemp = (DWORD)my_min((QWORD)dwBufSize, qwToDo);
      
      nRes = readData(cCompBuf1, qwPtr1, dwTemp);
      if (nRes == -1)
	THROW(ERR_READING, (DWORD)qwPtr1, errno);

      nRes = readData(cCompBuf2, qwPtr2, dwTemp);
      if (nRes == -1)
	THROW(ERR_READING, (DWORD)qwPtr2, errno);

      if (memcmp(cCompBuf1, cCompBuf2, dwTemp) != 0)
	{
	  showDebug(1, "FAT_COMPARE: differences found\n");
	  THROW(ERR_FAT_MISMATCH);
	}

      qwPtr1 += (QWORD)dwTemp;
      qwPtr2 += (QWORD)dwTemp;
      qwToDo -= (QWORD)dwTemp;
    }

  showDebug(1, "FAT_COMPARE: no differences found\n");

  RETURN;
}

// =======================================================
void CFatPart::printfInformations()
{
  char szText[8192];
  char szFullText[8192];

  getStdInfos(szText, sizeof(szText), false);

  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Sector Size...................%u\n"
			    "Sectors per cluster...........%u\n"
			    "Reserved Sectors count........%u\n"
			    "Root directory sectors........%lu\n"
			    "FAT tables count..............%u\n"
			    "Total Sectors Count...........%lu\n"
			    "Sectors per FAT...............%lu\n"
			    "Clusters count:...............%lu\n"
			    "Used clusters.................%lu\n"
			    "Free clusters.................%lu\n"
			    "Damaged clusters..............%lu\n"),
	   szText, m_info.wBytesPerSector, m_info.cSectorsPerCluster,
	   m_info.wReservedSectorsCount, m_info.dwRootDirSectors,
	   m_info.cNumberOfFATs,m_info.dwTotalSectorsCount,
	   m_info.dwSectorsPerFAT, m_info.dwClustersCount,
	   m_info.dwUsedClusters, m_info.dwFreeClusters,
	   m_info.dwDamagedClusters);

  g_interface->msgBoxOk(i18n("FAT informations"), szFullText);
}			

// =======================================================
void CFatPart::readBitmap(COptions *options)
{
  BEGIN;

  DWORD i, j;
  DWORD dwCurSect;
  int nRes;
  DWORD dwReservedBytes;
  WORD wFat16Entry;
  DWORD dwFat32Entry;

  // init
  m_bitmap.init(/*m_info.dwTotalSectorsCount*/m_header.qwBitmapSize+1024);

  dwCurSect = 0;
  m_info.dwUsedClusters = 0; 
  m_info.dwFreeClusters = 0;
    
  // ---- A) the reserved sectors are used
  for (i=0; i < m_info.wReservedSectorsCount; i++,dwCurSect++)
    m_bitmap.setBit(dwCurSect, true);
  
  // ---- B) the FAT tables are on used sectors
  for (j=0; j < m_info.cNumberOfFATs; j++)
    for (i=0; i < m_info.dwSectorsPerFAT; i++,dwCurSect++)
      m_bitmap.setBit(dwCurSect, true);

  // ---- C) The rootdirectory is on used sectors
  if (m_info.dwRootDirSectors > 0) // no rootdir sectors on FAT32
    for (i=0; i < m_info.dwRootDirSectors; i++,dwCurSect++)
      m_bitmap.setBit(dwCurSect, true);

  // ---- D) The clusters

  // init
  dwReservedBytes = m_info.wReservedSectorsCount * m_info.wBytesPerSector;
  m_info.dwDamagedClusters = 0L;
  
  // skip reserved data and two first FAT entries
  if (m_info.dwFileSystem == FS_FAT16)
    fseek(m_fDeviceFile, dwReservedBytes+(2*2), SEEK_SET);
  else if (m_info.dwFileSystem == FS_FAT32) 
    fseek(m_fDeviceFile, dwReservedBytes+(2*4), SEEK_SET);
  else
    THROW(ERR_WRONG_FS);
  
  // for each FAT entry...
  for (i=0; i < m_info.dwClustersCount; i++)
    {
      // If FAT16
      if (m_info.dwFileSystem == FS_FAT16)
	{
	  nRes = fread(&wFat16Entry, sizeof(WORD), 1, m_fDeviceFile);
	  if (nRes != 1)
            THROW(ERR_READING, i, errno);
	  wFat16Entry = Le16ToCpu(wFat16Entry);
	  
	  if (wFat16Entry == 0xFFF7) // bad FAT16 cluster
	    {
	      m_info.dwDamagedClusters++;
	      for (j=0; j < m_info.cSectorsPerCluster; j++,dwCurSect++) 
		m_bitmap.setBit(dwCurSect, false);
	    }
	  else if (wFat16Entry == 0x0000) // free
	    {
	      m_info.dwFreeClusters++;
	      for (j=0; j < m_info.cSectorsPerCluster; j++,dwCurSect++) 
		m_bitmap.setBit(dwCurSect, false);
	    }
	  else // used
	    {
	      m_info.dwUsedClusters++;
	      for (j=0; j < m_info.cSectorsPerCluster; j++,dwCurSect++) 
		m_bitmap.setBit(dwCurSect, true);
	    }
	}
      
      // If FAT32
      else if (m_info.dwFileSystem == FS_FAT32)
	{
	  nRes = fread(&dwFat32Entry, sizeof(DWORD), 1, m_fDeviceFile);
	  if (nRes != 1)
            THROW(ERR_READING, (DWORD) 0, errno);
	  dwFat32Entry = Le32ToCpu(dwFat32Entry);
	  
	  if (fat32to28(dwFat32Entry) == 0x0FFFFFF7) // bad FAT32 cluster
	    {	
	      m_info.dwDamagedClusters++;
	      for (j=0; j < m_info.cSectorsPerCluster; j++,dwCurSect++) 
		m_bitmap.setBit(dwCurSect, false);
	    }
	  else if (fat32to28(dwFat32Entry) == 0x00000000) // free
	    {	
	      m_info.dwFreeClusters++;
	      for (j=0; j < m_info.cSectorsPerCluster; j++,dwCurSect++) 
		m_bitmap.setBit(dwCurSect, false);
	    }
	  else // used
	    {
	      m_info.dwUsedClusters++;
	      for (j=0; j < m_info.cSectorsPerCluster; j++,dwCurSect++) 
		m_bitmap.setBit(dwCurSect, true);
	    }
	}
      
      // If other file system
      else
        THROW(ERR_WRONG_FS);
      
    }

  // mark final sectors as used
  while (dwCurSect < m_info.dwTotalSectorsCount)
    {
      m_bitmap.setBit(dwCurSect, true);
      dwCurSect++;
    }

  // ---- end
  calculateSpaceFromBitmap();

  RETURN;
}

// =======================================================
void CFatPart::readSuperBlock()
{
  BEGIN;

  BYTE cBoot[MAX_BOOT_SIZE];
  WORD wTemp;
  DWORD dwTemp;
  BYTE cTemp;
  int nRes;
  
  // init
  memset(&m_info, 0, sizeof(CInfoFatHeader));

  // 1. read and print important informations
  nRes = readData(&cBoot, 0, MAX_BOOT_SIZE);
  if (nRes == -1)
    {
      g_interface->msgBoxError(i18n("Can't read boot sector"));
      THROW(ERR_READING, (DWORD) 0, errno);
    }
  
  // 2. calculate infos
  memcpy(&m_info.wBytesPerSector, cBoot+11, 2); m_info.wBytesPerSector = Le16ToCpu(m_info.wBytesPerSector);
  memcpy(&m_info.cSectorsPerCluster, cBoot+13, 1);
  memcpy(&m_info.wReservedSectorsCount, cBoot+14, 2); m_info.wReservedSectorsCount = Le16ToCpu(m_info.wReservedSectorsCount);
  memcpy(&m_info.cNumberOfFATs, cBoot+16, 1);
  memcpy(&m_info.wRootEntriesCount, cBoot+17, 2); m_info.wRootEntriesCount = Le16ToCpu(m_info.wRootEntriesCount);
  memcpy(&m_info.wFsInfoSector, cBoot+48, 2); m_info.wFsInfoSector = Le16ToCpu(m_info.wFsInfoSector);

  // check data
  if ((m_info.wBytesPerSector % 512 != 0) || (m_info.wBytesPerSector < 512))
    THROW(ERR_WRONG_FS, errno);
  if (m_info.cSectorsPerCluster == 0)
    THROW(ERR_WRONG_FS, errno);
  if (m_info.wReservedSectorsCount ==0) 
    THROW(ERR_WRONG_FS, errno);
  if (m_info.cNumberOfFATs == 0)
    THROW(ERR_WRONG_FS, errno);
  
  // Label
  memcpy(&cTemp, cBoot+38, 1);
  if (cTemp == 0x29) // there is a label
    memcpy(m_header.szLabel, cBoot+43, 11);
  else
    memset(m_header.szLabel, 0, 11);
  
  // Total number of sectors
  memcpy(&wTemp, cBoot+19, 2); wTemp = Le16ToCpu(wTemp);
  memcpy(&dwTemp, cBoot+32, 4); dwTemp = Le32ToCpu(dwTemp);
  if (wTemp)
    m_info.dwTotalSectorsCount = (DWORD)wTemp;
  else
    m_info.dwTotalSectorsCount = dwTemp;
  
  // avoid bug: cannot read last sector
  m_info.dwTotalSectorsCount--;

  // Count of sectors occupied by one FAT
  memcpy(&wTemp, cBoot+22, 2); wTemp = Le16ToCpu(wTemp);
  memcpy(&dwTemp, cBoot+36, 4); dwTemp = Le32ToCpu(dwTemp);
  if (wTemp)
    m_info.dwSectorsPerFAT = (DWORD)wTemp;
  else
    m_info.dwSectorsPerFAT = dwTemp;		
  
  // calculate the FAT Type with the cluster count (FAT12 or FAT16 or FAT32)
  m_info.dwRootDirSectors = ((m_info.wRootEntriesCount * 32) + (m_info.wBytesPerSector-1)) / m_info.wBytesPerSector;
  
  m_info.dwDataSectors = m_info.dwTotalSectorsCount - (m_info.wReservedSectorsCount + (m_info.cNumberOfFATs * m_info.dwSectorsPerFAT) + m_info.dwRootDirSectors);
  
  if (m_info.cSectorsPerCluster == 0)
    {
      g_interface->msgBoxError(i18n("m_cSectorsPerCluster can't be null. Check the partition to save is a FAT one.") );
      THROW(ERR_WRONG_FS);
    }
  
  m_info.dwClustersCount = m_info.dwDataSectors / m_info.cSectorsPerCluster;
  
  setSuperBlockInfos(false, false, 0, 0);
  if (m_info.dwClustersCount < 4085)
      m_info.dwFileSystem = FS_FAT12;
  else if (m_info.dwClustersCount < 65525)
    {
      m_info.dwBytesPerFatEntry = 2;
      m_info.dwFileSystem = FS_FAT16;
    }
  else
    {
      m_info.dwBytesPerFatEntry = 4;
      m_info.dwFileSystem = FS_FAT32;
    }

  // data for the abstract file system
  m_header.qwBlockSize = m_info.wBytesPerSector;
  m_header.qwBitmapSize = (m_info.dwTotalSectorsCount+7)/8;
  m_header.qwBlocksCount = m_info.dwTotalSectorsCount;

  // FS info sector
  showDebug(1, "FSInfo=%u\n", m_info.wFsInfoSector);
  
  if (m_info.wFsInfoSector)
    nRes = readFSInfo(m_info.wFsInfoSector);
  else
    nRes = -1;

  if ((nRes == -1) && (m_info.dwFileSystem == FS_FAT32))
    {
      showDebug(1, "Can't read FSInfo sector\n");
      g_interface->msgBoxError(i18n("Can't read FSInfo sector"));
      setSuperBlockInfos(false, false, 0, 0);
    }

  RETURN;
}

// =======================================================
int CFatPart::readFSInfo(WORD wFsInfoSector)
{
  BEGIN;

  QWORD qwFreeSectors;
  QWORD qwUsedSectors;
  CFatFsInfoSector sectFsInfo;
  int nRes;

  nRes = readData(&sectFsInfo, m_info.wFsInfoSector*m_info.wBytesPerSector, sizeof(sectFsInfo));
  if (nRes == -1)
    {
      showDebug(1, "Can't read FSInfo sector\n");
      return -1;
    }

  if (sectFsInfo.dwMagic != CpuToLe32(0x41615252))
    {
      showDebug(1, "FSInfo sector: bad magic\n");
      return -1;
    }

  
  qwFreeSectors = m_info.cSectorsPerCluster * Le32ToCpu(sectFsInfo.dwFreeClusters);
  qwUsedSectors =  m_info.dwTotalSectorsCount - qwFreeSectors;

  setSuperBlockInfos(false, false, qwUsedSectors * ((QWORD)m_info.wBytesPerSector), qwFreeSectors * ((QWORD)m_info.wBytesPerSector), true);

  RETURN_int(0);
}

