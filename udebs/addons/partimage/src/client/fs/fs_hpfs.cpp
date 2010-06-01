/***************************************************************************
         chpfspart.cpp  -  HPFS (High Perf OS/2) File System Support
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

#include "fs_hpfs.h"
#include "partimage.h"
#include "imagefile.h"

// =======================================================
CHpfsPart::CHpfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  showDebug(9, "begin\n");  
  showDebug(9, "end\n");  
}

// =======================================================
CHpfsPart::~CHpfsPart()
{
  showDebug(9, "begin\n");  
  showDebug(9, "end\n");  
}

// =======================================================
void CHpfsPart::fsck()
{
// SUCCESS
}

// =======================================================
void CHpfsPart::readBitmap(COptions *options)
{
  BEGIN;

  BYTE cBitmapList[4096+1];
  BYTE cBuffer[2048+1];
  DWORD dwOffset;
  QWORD qwOffset;
  DWORD dwPos;
  DWORD dwByte;
  DWORD *dwBuffer;
  QWORD qwBit;
  DWORD i, j;
  int nRes;
  
  qwOffset = ((QWORD)m_info.dwBitmapPointer) * ((QWORD)m_header.qwBlockSize);
  nRes = bigFseek(m_fDeviceFile, qwOffset); // SEEK_SET forced
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
  
  nRes = fread(cBitmapList, 4096, 1, m_fDeviceFile);
  if (nRes != 1)
    THROW(ERR_ERRNO, errno);
  
  showDebug(1, "\n\n======== begin of Bitmap sectors list ============\n");
  
  // calculates bitmap blocks count
  m_info.dwBitmapQuadBlocksCount = 0L;
  do
    {
      // ---- Read entry of the list
      memcpy(&dwOffset, cBitmapList+(m_info.dwBitmapQuadBlocksCount*sizeof(DWORD)), sizeof(DWORD));
      if (dwOffset)
	{	
	  showDebug(1, "%lu, ", dwOffset);
	  m_info.dwBitmapQuadBlocksCount++;
	}
      
    } while ((m_info.dwBitmapQuadBlocksCount < 512) && (dwOffset));
  // The 4KB bitmap sectors list can contain 512 entries. Error if partition > 4 GB...
  
  showDebug(1, "\n======== end of Bitmap sectors list ============\n\n\n");
  
  m_header.qwBitmapSize = m_info.dwBitmapQuadBlocksCount * 4 * 512;

  //nRes = m_bitmap.init(m_dwBitmapQuadBlocksCount * 4 * 512 * 8);
  nRes = m_bitmap.init(m_header.qwBitmapSize);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
  
  dwBuffer = (DWORD *)cBuffer;
  qwBit = 0L;
  
  for (dwPos=0; dwPos < m_info.dwBitmapQuadBlocksCount; dwPos++)
    {
      // ---- Read entry of the list
      memcpy(&dwOffset, cBitmapList+(dwPos*sizeof(DWORD)), sizeof(DWORD));
      
      // ---- Read bitmap block
      qwOffset = ((QWORD)dwOffset) * ((QWORD)m_header.qwBlockSize);
      nRes = bigFseek(m_fDeviceFile, qwOffset);
      if (nRes == -1)
        THROW(ERR_ERRNO, errno);
      
      nRes = fread(cBuffer, 2048, 1, m_fDeviceFile); // 2048 bytes = QuadBitmapBlock
      if (nRes != 1)
        THROW(ERR_ERRNO, errno);
      
      // analyse buffer
      for (i=0; i < 2048 / sizeof(DWORD); i++)
	{	
	  dwByte = dwBuffer[i];
				
	  for (j=0; j < sizeof(DWORD) * 8; j++)
	    {
	      if (dwByte & 1) // block is free
		m_bitmap.setBit(qwBit, false);
	      else // block is used
		m_bitmap.setBit(qwBit, true);
	      
	      dwByte>>=1;
	      qwBit++;
	    }
	}
    }
  
  // Free/Used blocks count
  m_header.qwUsedBlocks = 0L;
  for (dwPos=0; dwPos < m_header.qwBlocksCount; dwPos++)
    {
      if (m_bitmap.isBitSet(dwPos))
	m_header.qwUsedBlocks++;
    }	

  RETURN;
}

// =======================================================
void CHpfsPart::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  
  getStdInfos(szText, sizeof(szText), true);
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "HPFS version:.................%d\n"), 
	   szText, m_info.cHpfsVersion);

  g_interface->msgBoxOk(i18n("HPFS informations"), szFullText);
}			
			
// =======================================================
void CHpfsPart::readSuperBlock()
{
  BEGIN;

  int nRes;
  hpfs_boot_block boot;
  hpfs_super_block super;
  hpfs_spare_block spare;
  
  // init
  memset(&m_info, 0, sizeof(CInfoHpfsHeader));

  // read BOOT block
  nRes = fseek(m_fDeviceFile, 0L, SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
	
  nRes = fread(&boot, sizeof(hpfs_boot_block), 1, m_fDeviceFile);
  if (nRes != 1)
    {
      g_interface->msgBoxError(i18n("Can't read boot block"));
      THROW(ERR_READ_SUPERBLOCK);
    }
  
  // read SUPER block
  nRes = fseek(m_fDeviceFile, (long)(16 * 512), SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
  
  nRes = fread(&super, sizeof(hpfs_super_block), 1, m_fDeviceFile);
  if (nRes != 1)
    {
      g_interface->msgBoxError(i18n("Can't read super block"));
      THROW(ERR_READ_SUPERBLOCK);
    }
  
  // read SPARE block
  nRes = fseek(m_fDeviceFile, (long)(17 * 512), SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
  
  nRes = fread(&spare, sizeof(hpfs_spare_block), 1, m_fDeviceFile);
  if (nRes != 1)
    {	
      g_interface->msgBoxError(i18n("Can't read spare block"));
      THROW(ERR_ERRNO, errno);
    }
  
  // check magic numbers
  if ((super.magic != 0xf995e849) || (spare.magic != 0xf9911849))
    {
      g_interface->msgBoxError(i18n("Invalid HPFS magic numbers. Not a valid HPFS partition."));
      THROW(ERR_WRONG_FS);
    }
  
  // analyze informations
  m_header.qwBlockSize = (DWORD) (boot.bytes_per_sector[1] * 256 + boot.bytes_per_sector[0]);
  m_header.qwBlocksCount = (DWORD) boot.n_sectors_l - 1; // TODO: why -1 ? (if not, error at the end of the volume)
  m_info.cHpfsVersion = super.version;
  memcpy(m_header.szLabel, boot.vol_label, 11);
  m_info.dwBitmapPointer = super.bitmaps;
  showDebug(1, "m_dwBitmapPointer = %lu\n", m_info.dwBitmapPointer);
  
  // check data
  if ((m_header.qwBlockSize % 512 != 0) || (m_header.qwBlockSize < 512))
    THROW(ERR_ERRNO, errno);
  
  setSuperBlockInfos(false, false, 0, 0);

  RETURN;
}
