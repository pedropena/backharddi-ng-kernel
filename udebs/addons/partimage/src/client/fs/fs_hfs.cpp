/***************************************************************************
              fs_hfs.cpp  - HFS/HFS+: MacOs, Hierarchical File System
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

#include "fs_hfs.h"
#include "partimage.h"
#include "imagefile.h"
#include "misc.h"
#include "gui_text.h"

#include <stdio.h>

// =======================================================
CHfsPart::CHfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CHfsPart::~CHfsPart()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CHfsPart::fsck()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CHfsPart::readSuperBlock()
{
  BEGIN;

  QWORD qwBlocksInRegularAlloc;
  hfsSuperBlock sb, sb2;
  int nRes;

  // read superblock
  nRes = readData(&sb, 1024, sizeof(sb));
  if (nRes == -1)
    {
      showDebug(10, "ERROR 1\n");
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  if (Be16ToCpu(GET_WORD(&sb.drSigWord)) != HFS_SUPER_MAGIC)
    {
      showDebug(10, "ERROR 2\n");
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  // init
  memset(&m_info, 0, sizeof(CInfoHfsHeader));

  m_info.qwAllocCount = BeToCpu(GET_WORD(&sb.drNmAlBlks)); 
  m_info.dwAllocSize = BeToCpu(GET_DWORD(&sb.drAlBlkSize)); 
  m_info.dwBlocksPerAlloc = m_info.dwAllocSize / 512; 
  m_info.qwBitmapSectLocation = (QWORD)BeToCpu(GET_WORD(&sb.drVBMSt));
  m_info.qwFirstAllocBlock = (QWORD)BeToCpu(GET_WORD(&sb.drAlBlSt));
  m_info.qwFreeAllocs = (QWORD)BeToCpu(GET_WORD(&sb.drFreeBks));
  
  // if not a 512 multiple
  if ((m_info.dwAllocSize % 512) != 0)
    THROW(ERR_READING, (DWORD)0, errno);

  // check the last but one block is a superblock
  qwBlocksInRegularAlloc = m_info.dwBlocksPerAlloc * m_info.qwAllocCount;
  nRes = readData(&sb2, (m_info.qwFirstAllocBlock+qwBlocksInRegularAlloc)*512LL, sizeof(sb2));
  if (nRes == -1)
    {
      showDebug(10, "ERROR 3\n");
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  if (Be16ToCpu(GET_WORD(&sb.drSigWord)) != HFS_SUPER_MAGIC)
    {
      showDebug(10, "ERROR 4\n");
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  // show infos
  showDebug(10, "m_info.qwAllocCount=%llu\n", m_info.qwAllocCount);
  showDebug(10, "m_info.dwAllocSize=%lu=%lu sectors\n", m_info.dwAllocSize, m_info.dwBlocksPerAlloc);
  showDebug(10, "m_info.qwBitmapSectLocation=%llu\n", m_info.qwBitmapSectLocation);
  showDebug(10, "m_info.qwFirstAllocBlock=%llu\n", m_info.qwFirstAllocBlock);
  showDebug(10, "m_info.qwFreeAllocs=%llu\n", m_info.qwFreeAllocs);

  // label == pascal string
  memset(m_header.szLabel, 0, 64);
  memcpy(m_header.szLabel, ((char*)&sb.drVN)+1, my_min(((BYTE*)&sb.drVN)[0], 63));
  showDebug(10, "label=[%s]\n", m_header.szLabel);

  // calculate sectors count
  m_header.qwBlocksCount = m_info.qwFirstAllocBlock // first reserved (2 boot blocks, 1 superblock, ...)
    + (m_info.dwBlocksPerAlloc * m_info.qwAllocCount) // allocation blocks
    + 1 // alternate superblock
    + 1; // last volume of sector
  showDebug(10, "m_header.qwBlocksCount = %llu\n", m_header.qwBlocksCount);
   
  m_header.qwBlockSize = 512;

  m_header.qwBitmapSize = (m_header.qwBlocksCount+7)/8;
  
  QWORD qwUsedBlocks = m_info.qwFirstAllocBlock + 2 + ((m_info.qwAllocCount-m_info.qwFreeAllocs) * m_info.dwBlocksPerAlloc);
  QWORD qwFreeBlocks = m_info.qwFreeAllocs * m_info.dwBlocksPerAlloc;
  setSuperBlockInfos(true, true, qwUsedBlocks*512, qwFreeBlocks*512);

  RETURN;  
}

// =======================================================
void CHfsPart::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  char szTemp1[1024];
  
  getStdInfos(szText, sizeof(szText), true);
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Allocation Group count:.......%llu\n"
			    "Blocks per Allocation Group...%lu\n"
			    "Allocation Group size:........%s\n"
			    "First allocation block:.......%llu\n"),
	   szText, m_info.qwAllocCount, m_info.dwBlocksPerAlloc,
	   formatSize(m_info.dwAllocSize,szTemp1), m_info.qwFirstAllocBlock);
    
  g_interface->msgBoxOk(i18n("HFS informations"), szFullText);
}			

// =======================================================
void CHfsPart::readBitmap(COptions *options)
{
  BEGIN;

  QWORD qwBlocksInRegularAlloc;
  QWORD i, j, k;
  BYTE *cTempData;
  DWORD *dwTempData;
  QWORD qwCurSector;
  QWORD qwLastAllocSector;
  DWORD dwBitmapData;
  int nRes;
  
  // init bitmap
  const DWORD dwHighBit= 1<<31;
  showDebug(10, "m_header.qwBitmapSize=%llu\n",m_header.qwBitmapSize);
  m_bitmap.init(m_header.qwBitmapSize+1024);

  // special sectors (begin and end of volume) are used
  qwBlocksInRegularAlloc = m_info.dwBlocksPerAlloc * m_info.qwAllocCount;
  qwLastAllocSector = m_info.qwFirstAllocBlock+qwBlocksInRegularAlloc;

  for (i=0; i < m_info.qwFirstAllocBlock; i++)
    {
      showDebug(10, "Block [%llu] is special\n", i);
      m_bitmap.setBit(i, true);
    }
  showDebug(10, "Blocks [%llu] and [%llu] are special\n", qwLastAllocSector, qwLastAllocSector+1);
  m_bitmap.setBit(qwLastAllocSector, true); // alternate superblock
  m_bitmap.setBit(qwLastAllocSector+1LL, true); // last volume of sector

  // regular allocation blocks
  cTempData = (BYTE*)malloc(m_header.qwBitmapSize+1024);
  if (!cTempData)
    THROW(ERR_NOMEM);

  nRes = readData(cTempData, (QWORD)(3*512), m_header.qwBitmapSize+512);
  if (nRes == -1)
    {
      showDebug(10, "ERROR 5\n");
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  dwTempData = (DWORD*)cTempData;
  qwCurSector = m_info.qwFirstAllocBlock;

  for (i=0; i < m_info.qwAllocCount; i+=32)
    {
      dwBitmapData = Be32ToCpu(*dwTempData);
      dwTempData++;
      
      for (j=0; j < 32; j++)
	{
	  if (dwBitmapData & dwHighBit) // block is free
	    {
	      for (k=0; k < m_info.dwBlocksPerAlloc; k++, qwCurSector++)
		{
		  if (qwCurSector < qwLastAllocSector)
		    {
		      m_bitmap.setBit(qwCurSector, true);
		      showDebug(10, "used -> %llu\n", qwCurSector);
		    }
		}
	    }
	  else // block is used
	    {
	      for (k=0; k < m_info.dwBlocksPerAlloc; k++, qwCurSector++)
		{
		  if (qwCurSector < qwLastAllocSector)
		    {
		      m_bitmap.setBit(qwCurSector, false);
		      showDebug(10, "free -> %llu\n", qwCurSector);

		    }		
		}
	    }	  
	  dwBitmapData<<=1;
	}
    }
  
  free(cTempData);

  calculateSpaceFromBitmap();

  RETURN;
}			

