/***************************************************************************
                cxfspart.cpp  - XFS (SGI IRIX) File System Support
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

#include <stdio.h>

#include "fs_xfs.h"
#include "partimage.h"
#include "imagefile.h"
#include "misc.h"
#include "gui_text.h"

// =======================================================
CXfsPart::CXfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CXfsPart::~CXfsPart()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CXfsPart::fsck()
{
  BEGIN;
  RETURN;
}

// =======================================================
QWORD CXfsPart::convertAgToDaddr(DWORD dwAgNo, DWORD dwOffset)
{
  return (convertAgbToDaddr(dwAgNo, 0)+((QWORD)dwOffset)*((QWORD)BBSIZE));
}

// =======================================================
QWORD CXfsPart::convertAgbToDaddr(DWORD dwAgNo, DWORD dwAgBlockNo)
{
  QWORD qwFSBlock;
  qwFSBlock = (((QWORD)dwAgNo)*((QWORD)m_info.dwAgBlocksCount))+((QWORD)dwAgBlockNo);
  return (qwFSBlock * m_header.qwBlockSize);
}

// =======================================================
void CXfsPart::readBitmap(COptions *options)
{
  BEGIN;

  int nRes;
  DWORD i;
  QWORD j;
  xfs_agf agf;

  // init bitmap -> all blocks marked as used (1)
  m_bitmap.init(m_header.qwBitmapSize+1024);
  for (j=0; j < m_header.qwBlocksCount; j++)
    m_bitmap.setBit(j, true);

  // ---- process allocation groups
  
  // 1. go to the beginning of the disk
  nRes = fseek(m_fDeviceFile, 0, SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);

  // 2. loop for each AG of the file system
  for (i=0; i < m_info.dwAgCount; i++)
    {
      nRes = readData(&agf, convertAgToDaddr(i, XFS_AGF_DADDR), sizeof(xfs_agf));
      if (nRes == -1)
	THROW(ERR_ERRNO, errno);

      if (((DWORD)BeToCpu(agf.agf_magicnum)) != XFS_AGF_MAGIC)
	{
	  g_interface -> ErrorReadingSuperblock(errno);
	  THROW(ERR_READING, (DWORD)0, errno);
	}

      showDebug(3, "\n\n== ALLOCATION GROUP %lu ===\nagf_flfirst=%lu\nagf_fllast=%lu\nagf_flcount=%lu\nagf_freeblks=%lu\nagf_longest=%lu\n"
		"agf_roots[0]=%lu and agf_roots[1]=%lu\nagf_levels[0]=%lu and agf_levels[1]=%lu\n", (DWORD)i, 
		(DWORD)BeToCpu(agf.agf_flfirst), (DWORD)BeToCpu(agf.agf_fllast), 
		(DWORD)BeToCpu(agf.agf_flcount), (DWORD)BeToCpu(agf.agf_freeblks), 
		(DWORD)BeToCpu(agf.agf_longest), (DWORD)BeToCpu(agf.agf_roots[0]), 
		(DWORD)BeToCpu(agf.agf_roots[1]), (DWORD)BeToCpu(agf.agf_levels[0]), 
		(DWORD)BeToCpu(agf.agf_levels[1]));

      scanFreelist(&agf);
      scanSbtree(&agf, BeToCpu(agf.agf_roots[XFS_BTNUM_BNO]), BeToCpu(agf.agf_levels[XFS_BTNUM_BNO]));
    }
  
  // fill informations about free/used clusters count
  calculateSpaceFromBitmap();

  RETURN;
}			

// =======================================================
void CXfsPart::scanFreelist(xfs_agf *agf)
{
  DWORD dwAgNo = BeToCpu(agf->agf_seqno);
  xfs_agfl	agfl;
  DWORD dwBlockNo;
  DWORD		i;

  if (BeToCpu(agf->agf_flcount) == 0)
    return;

  readData(&agfl, convertAgToDaddr(dwAgNo, XFS_AGFL_DADDR), sizeof(agfl));

  // process list
  i = BeToCpu(agf->agf_flfirst);
  for (;;) 
    {
      dwBlockNo = BeToCpu(agfl.agfl_bno[i]);
      addToHist(dwAgNo, dwBlockNo, 1);
      if (i == BeToCpu(agf->agf_fllast))
	break;
      if (++i == XFS_AGFL_SIZE)
	i = 0;
    }
}

// =======================================================
void CXfsPart::scanSbtree(xfs_agf *agf, DWORD dwRoot, DWORD dwLevels) 
{
  char cBuffer[MAX_XFSBLOCKSIZE]; 

  readData(cBuffer, convertAgbToDaddr(BeToCpu(agf->agf_seqno), dwRoot), m_header.qwBlockSize); 
  scanfuncBno(cBuffer, dwLevels - 1, agf);
}

// =======================================================
void CXfsPart::scanfuncBno(char *cBlockData, DWORD dwLevel, xfs_agf *agf)
{
  xfs_alloc_block	*block = (xfs_alloc_block *)cBlockData; //ablock;
  xfs_alloc_ptr  	*pp;
  xfs_alloc_rec		*rp;
  DWORD i;

  //showDebug(10, "scanfuncBno(cBlockData, dwLevel=%lu);\n", dwLevel);

  if (dwLevel == 0)
    {
      //rp = XFS_BTREE_REC_ADDR(mp->m_sb.sb_blocksize, xfs_alloc, block, 1, mp->m_alloc_mxr[0]);
      rp = (xfs_alloc_rec*) (cBlockData+sizeof(xfs_alloc_block));

      for (i = 0; i < BeToCpu(block->bb_numrecs); i++)
	addToHist(BeToCpu(agf->agf_seqno), BeToCpu(rp[i].ar_startblock), BeToCpu(rp[i].ar_blockcount));
    }
  else
    { 
      //pp = XFS_BTREE_PTR_ADDR(mp->m_sb.sb_blocksize, xfs_alloc, block, 1, mp->m_alloc_mxr[1]);
      showDebug(1, "TREE: multilevel: dwLevel=%lu\n",dwLevel);
      showDebug(10, "TREE: multilevel: dwLevel=%lu\n",dwLevel);
      int nVal = (int)((m_header.qwBlockSize - (uint)sizeof(xfs_alloc_block)) / ((uint)sizeof(xfs_alloc_key) + (uint)sizeof(xfs_alloc_ptr)));
      pp = (xfs_alloc_ptr*) (cBlockData+sizeof(xfs_alloc_block)+(nVal*sizeof(xfs_alloc_key))); // /*mp->m_alloc_mxr[1]*/
      showDebug(1, "offset=%lu\n",sizeof(xfs_alloc_block)+(nVal*sizeof(xfs_alloc_key)));
      showDebug(10, "offset=%lu\n",sizeof(xfs_alloc_block)+(nVal*sizeof(xfs_alloc_key)));
      for (i = 0; i < BeToCpu(block->bb_numrecs); i++)
	{
	  showDebug(10, "LEVELRECUR: i=%lu, pp=%lu\n", i, (DWORD)pp[i]);
	  scanSbtree(agf, BeToCpu(pp[i]), dwLevel);
	}
    }
}

// =======================================================
void CXfsPart::addToHist(DWORD dwAgNo, DWORD dwAgBlockNo, QWORD qwLen)
{
  BEGIN;

  QWORD i;
  QWORD qwBase;

  showDebug(3, "%8d %8d %8llu\n", dwAgNo, dwAgBlockNo, qwLen);
  qwBase = (((QWORD)dwAgNo) * ((QWORD)m_info.dwAgBlocksCount)) + ((QWORD)dwAgBlockNo);

  for (i=0; i < qwLen; i++)
    m_bitmap.setBit(qwBase+i, false);

  RETURN;
}

// =======================================================
void CXfsPart::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  char szTemp1[1024];
  
  getStdInfos(szText, sizeof(szText), true);
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Allocation Group count:.......%lu\n"
			    "Blocks per Allocation Group...%lu\n"
			    "Allocation Group size:........%s\n"),
	   szText, m_info.dwAgCount, m_info.dwAgBlocksCount,
	   formatSize(m_info.dwAgBlocksCount*m_header.qwBlockSize,szTemp1));
    
  g_interface->msgBoxOk(i18n("XFS informations"), szFullText);
}			

// =======================================================
void CXfsPart::readSuperBlock()
{
  BEGIN;
  xfs_superblock sb;
  int nRes;
  char szTemp1[4096], szTemp2[4096], szTemp3[4096];
  QWORD qwFreeSpace, qwUsedSpace;
  
  // init
  memset(&m_info, 0, sizeof(CInfoXfsHeader));

  // 0. go to the beginning of the super block
  nRes = fseek(m_fDeviceFile, 0, SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
  
  // 1. read and print important informations
  nRes = fread(&sb, sizeof(xfs_superblock), 1, m_fDeviceFile);
  if (nRes != 1)
    {
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  // 2. check it's an XFS file system
  if (BeToCpu(sb.sb_magicnum) != XFS_SUPER_MAGIC)
    {
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD)0, errno);
    }

  // ---- blocks 
  m_header.qwBlockSize = (QWORD)BeToCpu(sb.sb_blocksize);
  showDebug(3, "SUPER: sb_blocksize=m_header.qwBlockSize=%llu\n", m_header.qwBlockSize); 

  // check for block size
  if (m_header.qwBlockSize != ((QWORD)4096))
    {
      showDebug(1, "Only 4096 bytes per block volumes are supported. Current one is %lu bytes/block", (DWORD)m_header.qwBlockSize);
      //g_interface -> msgBoxError(szTemp);
      THROW(ERR_BLOCKSIZE, "XFS", (DWORD)m_header.qwBlockSize);
    }

  m_header.qwBlocksCount = (QWORD)BeToCpu(sb.sb_dblocks);
  showDebug(3, "SUPER: sb_dblocks=m_header.qwBlocksCount=%llu\n", m_header.qwBlocksCount); 

  m_header.qwUsedBlocks = m_header.qwBlocksCount - (QWORD)BeToCpu(sb.sb_fdblocks); //swapNb((QWORD)sb.sb_fdblocks);
  showDebug(3, "SUPER: m_header.qwUsedBlocks=%llu\n", m_header.qwUsedBlocks);

  m_info.dwAgCount = (DWORD)BeToCpu(sb.sb_agcount);
  showDebug(3, "SUPER: sb_agcount=m_info.dwAgCount=%lu\n", m_info.dwAgCount); 

  m_info.dwAgBlocksCount = (DWORD)BeToCpu(sb.sb_agblocks);
  showDebug(3, "SUPER: m_info.dwAgBlocksCount=%lu\n", m_info.dwAgBlocksCount); 

  showDebug(3, "SUPER: Allocation group size: %lu blocks = %s\n", m_info.dwAgBlocksCount, formatSize(m_info.dwAgBlocksCount*m_header.qwBlockSize,szTemp1));
  
  // ---- space informations
  qwFreeSpace = (m_header.qwBlocksCount - m_header.qwUsedBlocks) * m_header.qwBlockSize;
  qwUsedSpace = m_header.qwUsedBlocks * m_header.qwBlockSize;
  showDebug(3, "SUPER: total: %s\nused: %s\nfree: %s\n",
	    formatSize(m_header.qwBlockSize * m_header.qwBlocksCount, szTemp1),
	    formatSize(qwUsedSpace, szTemp2),
	    formatSize(qwFreeSpace, szTemp3));
  showDebug(3, "SUPER: sb_rbmblocks=%lu\n",(DWORD)BeToCpu(sb.sb_rbmblocks));

  // ---- calculate infos
  //m_header.qwBlockSize = (QWORD)m_info.dwBlockSize;
  //m_header.qwUsedBlocks = m_info.qwBlocksCount - m_info.qwFreeBlocksCount; //swapNb((QWORD)sb.sb_fdblocks);
  //m_header.qwBlocksCount = m_info.qwBlocksCount;
  m_header.qwBitmapSize = (m_header.qwBlocksCount+7)/8; //sb.s_bmap_nr * m_header.qwBlockSize;
  memset(m_header.szLabel, 0, 64);

  setSuperBlockInfos(true, true, qwUsedSpace, qwFreeSpace);

  RETURN;  
}

// =======================================================
char* blocknum(char *szBuffer, DWORD dwBn)
{
  if (dwBn != NULLAGBLOCK)
    sprintf(szBuffer, "%lu", dwBn);
  else
    sprintf(szBuffer, "NULLAGBLOCK");
  return szBuffer;
}

