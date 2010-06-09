/***************************************************************************
          fs_ext2.cpp  -  EXT2FS (Linux standard) File System Support

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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
  #include <sys/stat.h>
#endif

#include "fs_ext2.h"
#include "partimage.h"
#include "imagefile.h"

// =======================================================
CExt2Part::CExt2Part(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CExt2Part::~CExt2Part() 
{
  BEGIN;
  RETURN;
}

// =======================================================
void CExt2Part::fsck()
{
  BEGIN;

  char szCommand[1024];
  int nRes;
  struct stat fStat;

// check if /sbin/e2fsck can be executed
  nRes = stat("/sbin/e2fsck", &fStat);
  if (nRes == -1)
    {
      nRes = g_interface -> askIgnoreNoFschk("/sbin/e2fsck");
      if (nRes == MSGBOX_CANCEL)
        THROW(ERR_CANCELED);
      else
        RETURN;
    }

  if ( !S_ISREG(fStat.st_mode) ||
       !(fStat.st_mode & S_IXOTH) &&
       ( getgid() != fStat.st_gid || !(fStat.st_mode & S_IXGRP) ) &&
       ( getuid() != fStat.st_uid || !(fStat.st_mode & S_IXUSR) ) )
    {
      nRes = g_interface -> askIgnoreDeniedFschk("/sbin/e2fsck");
      if (nRes == MSGBOX_CANCEL)
        THROW(ERR_CANCELED);
      else
        RETURN;
    }

  SNPRINTF(szCommand, "/sbin/e2fsck -nf -C 0 %s > /dev/null 2>/dev/null",
     m_szDevice);
  
  showDebug(1, "FSCK: [%s]\n", szCommand);

  nRes = system(szCommand);
  showDebug(1, "FSCK: return is %d\n", nRes); 
  if (nRes != 0)
    {	
      nRes = g_interface -> askIgnoreFSError("e2fsck");
      if (nRes == MSGBOX_CANCEL)
        THROW(ERR_FSCHK);
    }

  RETURN;
}

// =======================================================
void CExt2Part::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  
  *(szText) = '\0';
  getStdInfos(szText, sizeof(szText), false); 
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Number of groups..............%lu\n"
			    "First block...................%lu\n"
			    "Ext3fs journalization.........%s\n"
			    "Sparse super block............%s\n"
			    "Large files support...........%s\n"
			    "File system revision..........%lu\n"),
	   szText, m_info.dwGroupsCount, m_info.dwFirstBlock, 
	   (m_info.dwFeatureCompat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) ? i18n("yes") : i18n("no"),
	   (m_info.dwFeatureRoCompat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) ? i18n("yes") : i18n("no"),
	   (m_info.dwFeatureRoCompat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE) ? i18n("yes") : i18n("no"),
	   m_info.dwRevLevel);
  
  if (ISEXT3FS)
    g_interface->msgBoxOk(i18n("Ext3fs informations"), szFullText);
  else
    g_interface->msgBoxOk(i18n("Ext2fs informations"), szFullText);
}			

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%% begin of CODE WITHOUT LIBEXT2FS %%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// =======================================================
void CExt2Part::readSuperBlock() 
{
  BEGIN;

  CExt2Super sb;
  int nRes;

  // init
  memset(&m_info, 0, sizeof(CInfoExt2Header));

  // 0. go to the beginning of the super block
  nRes = fseek(m_fDeviceFile, 1024, SEEK_SET);
  if (nRes == -1)
    goto error_readSuperBlock;
  
  // 1. read and print important informations
  nRes = fread(&sb, sizeof(sb), 1, m_fDeviceFile);
  if (nRes != 1)
    goto error_readSuperBlock;

  // check partition has an ext2 file system
  if (sb.s_magic != Le16ToCpu(EXT2_SUPER_MAGIC))
    goto error_readSuperBlock;

  // read and print important informations	
  m_info.dwBlockSize = (EXT2_MIN_BLOCK_SIZE << Le32ToCpu(sb.s_log_block_size)); //sb.s_blocksize; //EXT2_BLOCK_SIZE(m_fs->super);
  showDebug(1, "blksize=%lu\n", m_info.dwBlockSize);
  m_info.dwFirstBlock = Le32ToCpu(sb.s_first_data_block);
  showDebug(1, "first=%lu\n", m_info.dwFirstBlock);
  m_info.dwTotalBlocksCount = Le32ToCpu(sb.s_blocks_count);
  showDebug(1, "total blocks=%lu\n", m_info.dwTotalBlocksCount);
  m_info.dwBlocksPerGroup = Le32ToCpu(sb.s_blocks_per_group);

  m_info.dwGroupsCount = (m_info.dwTotalBlocksCount - m_info.dwFirstBlock + m_info.dwBlocksPerGroup - 1) / Le32ToCpu(sb.s_blocks_per_group);
  showDebug(1, "groups=%lu\n", m_info.dwGroupsCount);
  m_info.dwLogicalBlocksPerExt2Block =  m_info.dwBlockSize / LOGICAL_EXT2_BLKSIZE;
  showDebug(1, "m_info.dwLogicalBlocksPerExt2Block=%lu\n",m_info.dwLogicalBlocksPerExt2Block);
  showDebug(1, "logperblok=%lu\n", m_info.dwLogicalBlocksPerExt2Block);
  
  m_info.dwDescPerBlock = m_info.dwBlockSize / sizeof(CExt2GroupDesc);
  m_info.dwDescBlocks = (m_info.dwGroupsCount + m_info.dwDescPerBlock - 1) / m_info.dwDescPerBlock;
  //debugWin("m_info.dwDescBlocks=%lu",(DWORD)m_info.dwDescBlocks);

  // virtual blocks used in the abstract CFSBase
  m_header.qwBlocksCount = m_info.dwTotalBlocksCount * m_info.dwLogicalBlocksPerExt2Block;
  m_header.qwBlockSize = LOGICAL_EXT2_BLKSIZE;
  m_header.qwBitmapSize = ((m_header.qwBlocksCount+7) / 8)+16;
  m_header.qwUsedBlocks = (m_info.dwTotalBlocksCount - Le32ToCpu(sb.s_free_blocks_count)) * m_info.dwLogicalBlocksPerExt2Block; 

  //debugWin("freeblks=%lu", sb.s_free_blocks_count);
  strncpy(m_header.szLabel, sb.s_volume_name, 64);

  // features
  m_info.dwFeatureCompat = Le32ToCpu(sb.s_feature_compat);
  m_info.dwFeatureIncompat = Le32ToCpu(sb.s_feature_incompat);
  m_info.dwFeatureRoCompat = Le32ToCpu(sb.s_feature_ro_compat);

  // misc infos
  m_info.dwRevLevel = Le32ToCpu(sb.s_rev_level);	// Revision level 
  memcpy(m_info.cUuid, sb.s_uuid, 16); // 128-bit uuid for volume

  //success_readSuperBlock:
  setSuperBlockInfos(true, true, m_header.qwUsedBlocks*m_header.qwBlockSize, ((QWORD)Le32ToCpu(sb.s_free_blocks_count)) * ((QWORD)m_info.dwBlockSize));
  showDebug(1, "end success\n");  
  RETURN;

 error_readSuperBlock:
  g_interface -> ErrorReadingSuperblock(errno);
  THROW(ERR_WRONG_FS);
}

// =======================================================
void CExt2Part::readBitmap(COptions *options) // FULLY WORKING
{
  BEGIN;
 
  DWORD i, j;
  CExt2GroupDesc *desc;
  int nRes;
  DWORD dwBlocksInThisGroup;
  DWORD dwBootBlocks;
  char *cTempBitmap;
  DWORD dwBit, dwByte;
  DWORD dwExt2DataBlock;
  char *cPtr;

  // debug
  DWORD dwUsed;
  DWORD dwFree;

  dwBootBlocks = m_info.dwFirstBlock / m_info.dwLogicalBlocksPerExt2Block;
  //debugWin("dwBootBlocks=%lu and m_info.dwLogicalBlocksPerExt2Block=%lu",dwBootBlocks,m_info.dwLogicalBlocksPerExt2Block);

  cTempBitmap = new char[((m_info.dwTotalBlocksCount+7)/8)+4096];
  if (!cTempBitmap)
  {  
     showDebug(1, "CExt2Part::readBitmap(): Error 001\n");  
     goto error_readBitmap;
  }

  // init bitmap size
  nRes = m_bitmap.init(m_header.qwBitmapSize);
  if (nRes == -1)
  { 
     showDebug(1, "CExt2Part::readBitmap(): Error 002\n");  
     goto error_readBitmap;
  }

  // load group descriptors
  desc = new CExt2GroupDesc[m_info.dwGroupsCount+m_info.dwDescPerBlock];
  if (!desc)
  { 
     showDebug(1, "CExt2Part::readBitmap(): Error 003\n");  
     goto error_readBitmap;
  }

  // for each descriptor BLOCK (not group descriptor!)
  for (cPtr=(char*)desc, i=0; i < m_info.dwDescBlocks; i++,cPtr+=m_info.dwBlockSize)
    {
      nRes = readData(cPtr, ((QWORD)m_info.dwBlockSize) * ((QWORD)(m_info.dwFirstBlock+1+i)), m_info.dwBlockSize);
      if (nRes == -1)
      { 
	 showDebug(1, "CExt2Part::readBitmap(): Error 004\n");  
	 goto error_readBitmap;
      }
    }

  dwUsed=0;
  dwFree=0;
  
  for (i = 0; i < m_info.dwGroupsCount; i++) 
    {
      if (m_info.dwFirstBlock+((i+1)*m_info.dwBlocksPerGroup) > m_info.dwTotalBlocksCount)
	dwBlocksInThisGroup = (m_info.dwTotalBlocksCount-m_info.dwFirstBlock) - (i*m_info.dwBlocksPerGroup);
      else
	dwBlocksInThisGroup = m_info.dwBlocksPerGroup;

      if (Le32ToCpu(desc[i].bg_block_bitmap))
	{
	  // -- read the bitmap block
	   errno = 0;
	  nRes = readData(cTempBitmap+(i*m_info.dwBlockSize), ((QWORD)m_info.dwBlockSize) * 
			  ((QWORD)Le32ToCpu(desc[i].bg_block_bitmap)), m_info.dwBlockSize);
	  if (nRes == -1)
	  {
	     showDebug(1, "CExt2Part::readBitmap(): Error 005\n");  
	     showDebug(1, "CExt2Part::readBitmap(): err=%d=%d\n", errno, strerror(errno));  
	     goto error_readBitmap;
	  }
	}
      else
	{
	  memset(cTempBitmap+(i*m_info.dwBlockSize), 0, m_info.dwBlockSize);
	}
    }

  // convert bitmap to little endian
  DWORD *dwPtr;
  DWORD dwLen;
  dwLen = sizeof(cTempBitmap) / sizeof(DWORD);
  dwPtr = (DWORD *)cTempBitmap;
  for (i=0; i < dwLen; i++, dwPtr++)
    *dwPtr = CpuToLe32(*dwPtr);

  // bitmap is full of 0 at init, then we just have to
  // write 1 for used blocks

  // the boot block of 1024 bytes = used
  for (i=0; i < dwBootBlocks; i++)
    m_bitmap.setBit(i, true);

  dwUsed=0; dwFree=0;
  for (i=dwBootBlocks, dwExt2DataBlock=0; dwExt2DataBlock < ( m_info.dwTotalBlocksCount- m_info.dwFirstBlock); dwExt2DataBlock++)
    {	
      dwBit = dwExt2DataBlock % 8;
      dwByte = (dwExt2DataBlock - dwBit) / 8;
       
      if ((cTempBitmap[dwByte] & (1 << dwBit)) != 0)
	{
	  for (j=0; j <  m_info.dwLogicalBlocksPerExt2Block; j++, i++)
	    m_bitmap.setBit(i, true);
	  dwUsed++;
	}
      else
	{
	  for (j=0; j <  m_info.dwLogicalBlocksPerExt2Block; j++, i++)
	    m_bitmap.setBit(i, false);
	  dwFree++; 
	}
    }

  //debugWin("used=%lu\nfree=%lu\ntotal=%lu",dwUsed,dwFree,dwUsed+dwFree);
  calculateSpaceFromBitmap();

  //success_readBitmap:
  delete []cTempBitmap;
  showDebug(1, "end success\n");  
  RETURN; // auccess

 error_readBitmap:
  delete []cTempBitmap;
  showDebug(1, "end error\n");  
  g_interface->msgBoxError(i18n("There was an error while reading the bitmap"));
  THROW(ERR_READ_BITMAP);
}

