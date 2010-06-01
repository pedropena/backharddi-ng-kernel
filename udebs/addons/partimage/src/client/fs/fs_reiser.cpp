/***************************************************************************
          creiserpart.cpp  -  Reiser (Journalized) File System Support
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

#include "fs_reiser.h"
#include "partimage.h"
#include "imagefile.h"
#include "misc.h"
#include "gui_text.h"

// =======================================================
CReiserPart::CReiserPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN; 
  RETURN;
}

// =======================================================
CReiserPart::~CReiserPart()
{
  BEGIN; 
  RETURN;
}

// =======================================================
void CReiserPart::fsck()
{
  BEGIN; 
  RETURN;
}

// =======================================================
void CReiserPart::readBitmap(COptions *options)
{
  BEGIN;

  DWORD i;
  DWORD dwBlock;
  BYTE cBuffer[MAX_REISER_BLOCK_SIZE+1];
  int nRes;
  DWORD dwOffset;
  
  // init bitmap size
  m_bitmap.init(m_header.qwBitmapSize);
  
  //printf ("\nBitmap blocks are: ");
  
  // copy bitmap blocks to bitmap in memory
  for (i = 0; i < m_info.dwBitmapBlocksCount; i ++)
    {	
      // We don't use fssek(dwCurrentBlock, SEEK_SET) after bitmap block 1, because fseek is
      // limited to 2 GB (because offset is long int). Then, we try to use an ossfet which can
      // fit in this long int. Then we only seek from the last bitmap block
      
      if (i == 0) 
	{
	  dwBlock = (REISERFS_DISK_OFFSET_IN_BYTES / m_header.qwBlockSize) + 1; // Skipped blocks + super block
	  nRes = fseek(m_fDeviceFile, (long) (dwBlock * m_header.qwBlockSize), SEEK_SET);
	  showDebug(9, "REISER_BITMAP #0: %lld\n", (dwBlock*m_header.qwBlockSize));
	}
      else if (i == 1)
	{
	  dwBlock = i * m_header.qwBlockSize * 8;
	  nRes = fseek(m_fDeviceFile, (long) (dwBlock * m_header.qwBlockSize), SEEK_SET);
	  showDebug(9, "REISER_BITMAP #1: %lld\n", (dwBlock*m_header.qwBlockSize));
	}
      else // (i > 1)
	{
	  dwBlock = i * m_header.qwBlockSize * 8;
	  dwOffset = ((m_header.qwBlockSize * 8) - 1) * m_header.qwBlockSize;		
	  nRes = fseek(m_fDeviceFile, (long) dwOffset, SEEK_CUR);
	  showDebug(9, "REISER_BITMAP #%ld: previous+%lld\n", i, (long) dwOffset);
	}
      
      // go to the beginning of the block
      if (nRes == -1) // result of the fseek()
        THROW(ERR_ERRNO, errno);
      
      // read bitmap from disk
      errno = 0;
      nRes = fread(cBuffer, m_header.qwBlockSize, 1, m_fDeviceFile);
      if (nRes != 1)
        THROW(ERR_READ_BITMAP, i);
      
      // write bitmap in memory
      memcpy(m_bitmap.data()+(i*m_header.qwBlockSize), cBuffer, m_header.qwBlockSize);
    }
  
  calculateSpaceFromBitmap();

  RETURN; 
}			

// =======================================================
void CReiserPart::printfInformations()
{
  char szText[8192];
  
  getStdInfos(szText, sizeof(szText), true);
  g_interface->msgBoxOk(i18n("ReiserFS informations"), szText);
}			

// =======================================================
void CReiserPart::readSuperBlock()
{
  BEGIN;

  reiserfs_super_block sb;
  int nRes;
  
  // init
  memset(&m_info, 0, sizeof(CInfoReiserHeader));

  // 0. go to the beginning of the super block
  nRes = readData(&sb, REISERFS_DISK_OFFSET_IN_BYTES, sizeof(sb));
  if (nRes == -1)
    {
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD) 0, errno);
    }

  // check the magic number is good
  if (strncmp(sb.s_magic, REISERFS_SUPER_MAGIC_STRING, strlen(REISERFS_SUPER_MAGIC_STRING)) == 0)
    m_info.dwVersion = VER_3_5;
  else if (strncmp(sb.s_magic, REISER2FS_SUPER_MAGIC_STRING, strlen(REISER2FS_SUPER_MAGIC_STRING)) == 0)
    m_info.dwVersion = VER_3_6;
  else
    THROW(ERR_WRONG_FS);
  
  // 2. calculate infos
  m_info.dwBitmapBlocksCount = LeToCpu(sb.s_bmap_nr);
  m_header.qwBlockSize = LeToCpu(sb.s_blocksize);
  m_header.qwUsedBlocks = LeToCpu(sb.s_block_count) - LeToCpu(sb.s_free_blocks);
  m_header.qwBlocksCount = LeToCpu(sb.s_block_count);
  m_header.qwBitmapSize = LeToCpu(sb.s_bmap_nr) * m_header.qwBlockSize;
  memset(m_header.szLabel, 0, 64);
  
  // check data
  if ((m_header.qwBlockSize % 512 != 0) || (m_header.qwBlockSize < 512))
    THROW(ERR_WRONG_FS);
  if (m_header.qwUsedBlocks > m_header.qwBlocksCount)
    THROW(ERR_WRONG_FS);
  
  setSuperBlockInfos(true, true, m_header.qwUsedBlocks * m_header.qwBlockSize, (m_header.qwBlocksCount - m_header.qwUsedBlocks) * m_header.qwBlockSize);

  RETURN;
}
