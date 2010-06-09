/***************************************************************************
           fs_ufs.cpp  - UFS (Unix File System) File System Support
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

/*
  + Be aware: (the block for partimage) == (a frag for FFS)
    For example, (FFS blocks are 8192 bytes) while (FFS frags == pi blocks == 1024 bytes)
  + Mounting FFS under Linux:
    mount -t ufs -o ro,ufstype=44bsd /dev/xxx /mnt/point
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fs_ufs.h"
#include "partimage.h"


// =======================================================
CUfsPart::CUfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CUfsPart::~CUfsPart()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CUfsPart::fsck()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CUfsPart::readSuperBlock()
{
  BEGIN;

  ufsSuperBlock sb;
  int nRes;
  
  // init
  memset(&m_info, 0, sizeof(CInfoUfsHeader));

  // 0. read super block
  nRes = readData(&sb, SBOFF, sizeof(sb));
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);

  showDebug(10, "sizeof(sb)=%d\n", sizeof(sb));
  showDebug(10, "sb.fs_sblkno=%llu\n",(QWORD)sb.fs_sblkno);
  showDebug(10, "sb.fs_cblkno=%llu // offset of cyl-block in filesys\n", (QWORD)sb.fs_cblkno);
  showDebug(10, "sb.fs_iblkno=%llu // offset of inode-blocks in filesys\n", (QWORD)sb.fs_iblkno);
  showDebug(10, "sb.fs_dblkno=%llu // offset of first data after cg\n", (QWORD)sb.fs_dblkno);
  showDebug(10, "sb.fs_cgoffset=%llu // cylinder group offset in cylinder\n", (QWORD)sb.fs_cgoffset);
  showDebug(10, "sb.fs_size=%llu // number of blocks (frag ?) in fs\n", (QWORD)sb.fs_size);
  showDebug(10, "sb.fs_dsize=%llu // number of data blocks (frag ?) in fs\n", (QWORD)sb.fs_dsize);
  showDebug(10, "sb.fs_ncg=%llu // number of cylinder groups\n", (QWORD)sb.fs_ncg);
  showDebug(10, "sb.fs_bsize=%llu // size of basic blocks in fs\n", (QWORD)sb.fs_bsize);
  showDebug(10, "sb.fs_fsize=%llu // size of frag blocks in fs\n", (QWORD)sb.fs_fsize);
  showDebug(10, "sb.fs_frag=%llu // number of frags in a block in fs\n", (QWORD)sb.fs_frag);
  showDebug(10, "sb.fs_cssize=%llu\n", (QWORD)sb.fs_cssize);
  showDebug(10, "sb.fs_fshift=%llu\n", (QWORD)sb.fs_fshift);
  showDebug(10, "sb.fs_csaddr=%llu // blk addr of cyl grp summary area\n", (QWORD)sb.fs_csaddr);
  showDebug(10, "sb.fs_maxbpg=%llu // max number of blks per cyl group\n", (QWORD)sb.fs_maxbpg);
  showDebug(10, "sb.fs_cgsize=%llu // cylinder group size (in basic blocks)\n", (QWORD)sb.fs_cgsize);
  showDebug(10, "\n");
  //showDebug(10, "sb.=%llu\n", (QWORD)sb.);
  //showDebug(10, "sb.=%llu\n", (QWORD)sb.);

  m_header.qwBlockSize = sb.fs_fsize;
  m_header.qwBlocksCount = sb.fs_size;
  m_header.qwBitmapSize = (m_header.qwBlocksCount+7)/8; 

  m_info.dwCylinderGroupsCount = sb.fs_ncg;
  m_info.fs_fpg = sb.fs_fpg;
  m_info.fs_cgoffset = sb.fs_cgoffset;
  m_info.fs_cgmask = sb.fs_cgmask;
  m_info.fs_cblkno = sb.fs_cblkno;
  m_info.dwFragsPerBlock = sb.fs_frag;
  m_info.dwCylinderGroupSize = sb.fs_cgsize;
  m_info.dwBasicBlockSize = sb.fs_bsize;
  m_info.qwDataFrags = sb.fs_dsize;

  RETURN;  
}

// =======================================================
void CUfsPart::readBitmap(COptions *options)
{
  BEGIN;

  cylinderGroupHeader *cgp;
  BYTE cBuffer[65536];
  QWORD qwStatsFreeFrags;
  QWORD qwStatsUsedFrags;  
  DWORD dwCurBmpSize;
  DWORD dwCgBmpSize;
  QWORD qwUgcMin;
  DWORD i, j;
  int nRes;

  // init bitmap -> all blocks marked as used (1)
  m_bitmap.init(m_header.qwBitmapSize+1024);
  for (i=0; i < m_header.qwBlocksCount; i++)
    m_bitmap.setBit(i, true);

  cgp = new cylinderGroupHeader[m_info.dwCylinderGroupsCount];
  if (!cgp)
    THROW(ERR_NOMEM);

  dwCurBmpSize = 0;
  QWORD qwCurStart = 0;
  qwStatsFreeFrags = 0;

  // process all cylinder groups
  for (i=0; i < m_info.dwCylinderGroupsCount; i++)
    {
      qwUgcMin = ufs_cgcmin(m_info.fs_fpg,m_info.fs_cgoffset,m_info.fs_cgmask,m_info.fs_cblkno,i);
      showDebug(10, "\nCGC[%d] stored in block (frag ?) %llu\n", i, qwUgcMin);
      
      nRes = readData(&(cgp[i]), qwUgcMin*m_header.qwBlockSize, sizeof(cylinderGroupHeader));
      if (nRes == -1)
	THROW(ERR_ERRNO, errno);
      
      if ((DWORD)cgp[i].cg_magic != (DWORD)CG_MAGIC)
	{
	  showDebug(10, "ERROR: %d not a cylinder group header\n", i);
	  THROW(ERR_ERRNO, errno);
	}

      showDebug(10, "   from %llu to %llu\n", (QWORD)qwCurStart, (QWORD)qwCurStart+cgp[i].cg_ndblk);

      //showDebug(10, "cgh[%d].cg_magic=%.8x (CG_MAGIC=%.8x)\n",i,(DWORD)cgp[i].cg_magic, (DWORD)CG_MAGIC);
      //showDebug(10, "cgh[%d].cg_cgx=%lu // CG number\n",i,(DWORD)cgp[i].cg_cgx);
      showDebug(10, "cgh[%d].cg_ndblk=%lu // blocks (frag) in this CG\n",i,(DWORD)cgp[i].cg_ndblk);
      showDebug(10, "cgh[%d].cg_freeoff=%lu // free block map\n",i,(DWORD)cgp[i].cg_freeoff);      
      showDebug(10, "cgh[%d].cg_cs.cs_ndir=%lu // stats: number of directories\n",i,(DWORD)cgp[i].cg_cs.cs_ndir);      
      showDebug(10, "cgh[%d].cg_cs.cs_nbfree=%lu // stats: number of free blocks\n",i,(DWORD)cgp[i].cg_cs.cs_nbfree);      
      showDebug(10, "cgh[%d].cg_cs.cs_nifree=%lu // stats: number of free inodes\n",i,(DWORD)cgp[i].cg_cs.cs_nifree);      
      showDebug(10, "cgh[%d].cs_cs.cs_nffree=%lu // stats: number of free frags\n",i,(DWORD)cgp[i].cg_cs.cs_nffree);      
      showDebug(10, "cgh[%d].cg_rotor=%lu // position of last used block\n",i,(DWORD)cgp[i].cg_rotor);      
      showDebug(10, "cgh[%d].cg_frotor=%lu // position of last used frag\n",i,(DWORD)cgp[i].cg_frotor);      
      showDebug(10, "cgh[%d].cg_irotor=%lu // position of last used inode \n",i,(DWORD)cgp[i].cg_irotor);      
      showDebug(10, "cgh[%d].cg_nclusterblks=%lu // number of clusters this cg\n",i,(DWORD)cgp[i].cg_nclusterblks);      
      //showDebug(10, "cgh[%d].=%lu // \n",i,(DWORD)cgp[i].);      
      //showDebug(10, "cgh[%d].=%lu // \n",i,(DWORD)cgp[i].);      
      
      qwStatsFreeFrags += (m_info.dwFragsPerBlock * cgp[i].cg_cs.cs_nbfree) + cgp[i].cg_cs.cs_nffree;

      dwCgBmpSize = (((DWORD)cgp[i].cg_ndblk)+7)/8;
      if (dwCgBmpSize > sizeof(cBuffer))
	{
	  showDebug(1, "UFS: buffer too small: have %lu, need %lu\n", sizeof(cBuffer), dwCgBmpSize);
	  delete []cgp;
	  THROW(ERR_ERRNO);
	}

      showDebug(10, "dwCgBmpSize=%lu\n",(DWORD)dwCgBmpSize);

      // copy and convert bitmap
      nRes = readData(cBuffer, ((qwUgcMin*m_header.qwBlockSize) + (DWORD)cgp[i].cg_freeoff), dwCgBmpSize);
      for (j=0; j < dwCgBmpSize; j++)
	cBuffer[j] = ~cBuffer[j];
      memcpy(m_bitmap.data()+dwCurBmpSize, cBuffer, dwCgBmpSize);

      dwCurBmpSize += dwCgBmpSize;
      qwCurStart += (QWORD)cgp[i].cg_ndblk;

    }

  qwStatsUsedFrags = m_header.qwBlocksCount - qwStatsFreeFrags;
  setSuperBlockInfos(true, true, qwStatsUsedFrags*m_header.qwBlockSize, qwStatsFreeFrags*m_header.qwBlockSize);

  delete []cgp;

  // fill informations about free/used clusters count
  calculateSpaceFromBitmap();

  RETURN;
}			

// =======================================================
void CUfsPart::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  char szTemp1[1024];
  
  getStdInfos(szText, sizeof(szText), true);
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Cylinder groups count:........%lu\n"
			    "Cylinder group size:..........%s\n"
			    "Basic blocks per CG:..........%lu\n"
			    "Basic block size:.............%lu\n"
			    "Data frags count:.............%llu\n"),
	   szText, m_info.dwCylinderGroupsCount, 
	   formatSize(m_info.dwCylinderGroupSize*m_info.dwBasicBlockSize,szTemp1),
	   m_info.dwCylinderGroupSize, m_info.dwBasicBlockSize, m_info.qwDataFrags);
    
  g_interface->msgBoxOk(i18n("UFS informations"), szFullText);
}			

