/***************************************************************************
         cjfspart.cpp  -  JFS (IBM XIA Journalized) File System Support
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

#include "fs_jfs.h"
#include "partimage.h"
#include "imagefile.h"
#include "misc.h"
#include "gui_text.h"

void showInodeInfos(CJfsDiskInode ino);
void showBmapInfos(dbmap_t cntl_page);
void printLocation(DWORD dwAdress, DWORD dwBlockSize, char *szObject);

// =======================================================
CJfsPart::CJfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CJfsPart::~CJfsPart()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CJfsPart::fsck()
{
  BEGIN;
  RETURN;
  // TODO: check for bad blocks
  // SUCCESS
}

// =======================================================
/* NAME: ujfs_rwdaddr
 * FUNCTION: read/write offset from/to an inode
 *	lbno	- logical block number
*/
int CJfsPart::ujfs_rwdaddr(s64 *offset, xtpage_t *page, s64 lbno)
{
  BEGIN;

  s32	lim, base, index;
  s32	cmp, rc;
  char buffer[PSIZE];
  s64	offset64;
  
 descend:
  // Binary search 
  for(base = XTENTRYSTART, lim = page->header.nextindex - XTENTRYSTART; lim; lim >>=1) 
    {
      //showDebug(10, " lim = %ld\n", lim);
      
      index = base + (lim >> 1);
      offset64 = offsetXAD(&(page->xad[index]));
      cmp = (lbno >= offset64 + lengthXAD(&(page->xad[index]))) ? 1 : (lbno < offset64) ? -1 : 0;
      if (cmp == 0) 
	{
	  // HIT! 
	  if (page->header.flag & BT_LEAF) 
	    {
	      *offset = (addressXAD(&(page->xad[index])) + (lbno - offsetXAD(&(page->xad[index])))) * m_header.qwBlockSize; //fs_block_size;
	      RETURN_int(0);
	    } 
	  else 
	    {
	      rc = readData(buffer, addressXAD(&(page->xad[index])) * m_header.qwBlockSize, PSIZE);
	      
	      if (rc) 
		RETURN_int(-1);

	      page = (xtpage_t *)buffer;
	      goto descend;
	    }
	}
      else if (cmp > 0) 
	{
	  base = index + 1;
	  --lim;
	}
    }
  
  if (page->header.flag & BT_INTERNAL) 
    {
      /* Traverse internal page, it might hit down there
       * If base is non-zero, decrement base by one to get the parent
       * entry of the child page to search.
       */
	index = base ? base - 1 : base;
	
	//rc = ujfs_rw_diskblocks(addressXAD(&(page->xad[index])) * m_header.qwBlockSize, PSIZE, buffer);
	rc = readData(buffer, addressXAD(&(page->xad[index])) * m_header.qwBlockSize, PSIZE);

	if (rc) 
	  {
	    //fprintf(stderr, "Internal error: %s(%d): Error reading btree node\n", __FILE__,  __LINE__);
	    RETURN_int(rc);
	  }
	page = (xtpage_t *)buffer;
	goto descend;
      }
    
    /* Not found! */
    //fprintf(stderr, "Internal error: %s(%d): Block %lld not found!\n", __FILE__,  __LINE__, lbno);
    RETURN_int(EINVAL);
}

// =======================================================
int CJfsPart::find_iag(u32 iagnum, s64 *address, DWORD dwType)
{
  BEGIN;

  s32       base;
  char      buffer[PSIZE];
  s32       cmp;
  CJfsDiskInode fileset_inode;
  s64       fileset_inode_address;
  s64       iagblock;
  s32       index;
  s32       lim;
  xtpage_t  *page;
  s32       rc;

  //int  l2bsize = log2(m_header.qwBlockSize);
  //#define IAGTOLBLK(iagno,l2nbperpg)	(((iagno) + 1) << (l2nbperpg))
  //iagblock = (((iagnum) + 1) << (L2PSIZE-l2bsize))                  //IAGTOLBLK(iagnum, L2PSIZE-l2bsize);

  //iagblock = IAGTOLBLK(iagnum, L2PSIZE-l2bsize); // vraie definition
  iagblock = (((iagnum) + 1) * (PSIZE/m_header.qwBlockSize)); // def sans log2

  /*if ( which_table == AGGREGATE_2ND_I ) {
    fileset_inode_address = AIT_2nd_offset + sizeof(dinode_t);
    } else*/
  fileset_inode_address = AGGR_INODE_TABLE_START + (/*which_table*/ /*FILESYSTEM_I*/ /* AGGREGATE_I*/ dwType * sizeof(CJfsDiskInode));

  //showDebug(10, "fileset_inode_address=%lld\n",fileset_inode_address);
  //showDebug(10, "AGGR_INODE_TABLE_START=%ld\n",(long)AGGR_INODE_TABLE_START);
  //showDebug(10, "FILESYSTEM_I=%ld\n",(long)FILESYSTEM_I);
  //showDebug(10, "sizeof(CJfsDiskInode)=%ld\n",(long)sizeof(CJfsDiskInode));

  //OLDCALL: rc = xRead(fileset_inode_address, sizeof(CJfsDiskInode), (char *)&fileset_inode); 
  rc = readData((char *)&fileset_inode, fileset_inode_address, sizeof(CJfsDiskInode));
  if (rc) 
    RETURN_int(-1);

  showDebug(10, "----------Fileset inode infos (offset: %lld)-----------\n", fileset_inode_address);
  showInodeInfos(fileset_inode);

  page = (xtpage_t *)&(fileset_inode.di_btroot);

 descend:
  // Binary search 
  for (base = XTENTRYSTART, lim = __le16_to_cpu(page->header.nextindex) - XTENTRYSTART; lim; lim >>=1) 
    {
      showDebug(10, "lim=%ld\n",lim);

      index = base + (lim >> 1);
      XT_CMP(cmp, iagblock, &(page->xad[index]));
      if (cmp == 0) 
	{
	  // HIT! 
	  if (page->header.flag & BT_LEAF) 
	    {
	      *address = (addressXAD(&(page->xad[index]))
			  + (iagblock - offsetXAD(&(page->xad[index]))))
		* m_header.qwBlockSize; //<< l2bsize;
	      RETURN_int(0); // success
	    } 
	  else 
	    {
	      //OLDCALL: rc = xRead( addressXAD(&(page->xad[index])) * m_header.qwBlockSize /*<< l2bsize*/, PSIZE, buffer);
	      rc = readData(buffer, addressXAD(&(page->xad[index])) * m_header.qwBlockSize /*<< l2bsize*/, PSIZE);
	      if (rc) 
		RETURN_int(-1);
	      page = (xtpage_t *)buffer;
	      
	      goto descend;
	    }
	} 
      else 
	if (cmp > 0) 
	  {
	    base = index + 1;
	    --lim;
	  }
    }
  
  if (page->header.flag & BT_INTERNAL) 
    {
      // Traverse internal page, it might hit down there
      // If base is non-zero, decrement base by one to get the parent
      // entry of the child page to search.
      index = base ? base - 1 : base;
      
      //OLDCALL: rc = xRead(addressXAD(&(page->xad[index])) * m_header.qwBlockSize /*<< l2bsize*/, PSIZE, buffer);
      rc = readData(buffer, addressXAD(&(page->xad[index])) * m_header.qwBlockSize /*<< l2bsize*/, PSIZE);
      if (rc) 
	RETURN_int(-1);

      page = (xtpage_t *)buffer;
      
      goto descend;
    }
  
  // Not found!   
  RETURN_int(-1);
}

// =======================================================
int CJfsPart::find_inode(u32 inum, QWORD *qwAddress, DWORD dwType)
{
  BEGIN;

  showDebug(10, "findInode(%lu, %llu)\n", inum, qwAddress);

  s32       extnum;
  iag_t     iag;
  s32       iagnum;
  s64       map_address;
  s32       rc;

  iagnum = INOTOIAG(inum);
  extnum = (inum & (INOSPERIAG -1)) >> L2INOSPEREXT;
  if (find_iag(iagnum, &map_address, dwType) == -1)
    RETURN_int(-1); // error

  showDebug(10, "extnum=%ld and map_address=%lld\n", extnum, map_address);

  //rc = ujfs_rw_diskblocks(fd, map_address, sizeof(iag_t),  &iag, GET);
  rc = readData(&iag, map_address, sizeof(iag_t));
  if (rc) 
    RETURN_int(-1); // error

  if (iag.inoext[extnum].len == 0)
    RETURN_int(-1); // error

  *qwAddress = (addressPXD(&iag.inoext[extnum]) * m_header.qwBlockSize /*<< l2bsize*/) + ((inum & (INOSPEREXT-1)) * sizeof(CJfsDiskInode/*dinode_t*/));
  RETURN_int(0); // success
}

// =======================================================
int CJfsPart::readDmap(QWORD *qwPage, xtpage_t *page, QWORD qwBlocksToProcess)
{
  BEGIN;

  dmap_t dmap;
  int nRes;
  s64 addr;

  static DWORD dwCurPos = 0;
  QWORD qwTemp;
      
  while (qwBlocksToProcess > 0)
    {
      nRes = ujfs_rwdaddr(&addr, page, (s64)(*qwPage));
      if (nRes != 0)
	RETURN_int(-1);
      
      //OLDCALL: nRes = xRead(addr, sizeof(dmap), (char *)&dmap);
      nRes = readData((char *)&dmap, addr, sizeof(dmap));
      if (nRes != 0)
	RETURN_int(-1);
      
      qwTemp = my_min(qwBlocksToProcess, (QWORD)8192LL);
      showDebug(10, "--- Dmap page %lu - *qwPage=%llu and qwProcess=%llu\n",(DWORD)(dwCurPos/1024), *qwPage, qwTemp);
      showDebug(10, "nblocks=%lu\n",(DWORD)dmap.nblocks);
      showDebug(10, "nfree=%lu\n",(DWORD)dmap.nfree);
      if (dmap.nblocks != dmap.nfree) 
	showDebug(10, "nused=%lu\n",(DWORD)(dmap.nblocks-dmap.nfree));
      showDebug(10, "start=%llu\n",(QWORD)dmap.start);

      // write bitmap in memory
      copyAndTransformDword((DWORD*)(m_bitmap.data()+dwCurPos), (DWORD*)dmap.wmap, 256);

      dwCurPos += 1024;
      (*qwPage)++; 

      qwBlocksToProcess -= qwTemp;
    }

  RETURN_int(0); // success
}

// =======================================================
int CJfsPart::browseBitmap(QWORD *qwPage, xtpage_t *page, DWORD dwLevel, QWORD qwBlocksToProcess)
{
  BEGIN;

  dmapctl_t cntl_page;
  s64       cntl_addr;
  int nRes;
  DWORD i;
  QWORD qwTemp;
  QWORD qwHandledBlocksSubLevel;

  // calculate blocks count handled by the sublevel
  qwHandledBlocksSubLevel = 8192; // handled blocks by level 0
  for (i=0; i < dwLevel; i++)
    qwHandledBlocksSubLevel <<= 10; // dwHandledBlocks *= 1024;

  showDebug(10, "\n----\nEnter in level %lu and qwPage=%llu and qwHandledBlocksSubLevel=%llu and qwBlocksToProcess=%llu\n", 
	     dwLevel, *qwPage, qwHandledBlocksSubLevel, qwBlocksToProcess);

  //while (qwBlocksToProcess > 0)
  //  {
  // Read first child
  nRes = ujfs_rwdaddr(&cntl_addr, page, (s64)(*qwPage));
  if (nRes != 0)
    RETURN_int(0);
      
  printLocation(cntl_addr, m_header.qwBlockSize, "control_page");

  //OLDCALL: nRes = xRead(cntl_addr, sizeof(cntl_page), (char *)&cntl_page);
  nRes = readData((char *)&cntl_page, cntl_addr, sizeof(cntl_page));
  if (nRes != 0)
    RETURN_int(0);

  (*qwPage)++; 
      
  showDebug(10, "\n-----\n");
  showDebug(10, "nleafs=%ld\n", cntl_page.nleafs);
  showDebug(10, "l2nleafs=%ld\n", cntl_page.l2nleafs);
  showDebug(10, "leafidx=%ld\n", cntl_page.leafidx);
  showDebug(10, "height=%ld\n", cntl_page.height);
  showDebug(10, "budmin=%d\n", cntl_page.budmin);

  showDebug(10, "stree[0]=%d\n", cntl_page.stree[0]);
  showDebug(10, "-----\n\n");

  /*for (j=341; j < (1024+341); j++)
    {
    if (cntl_page.stree[j]>0)
    {
    showDebug(10, "stree[%lu]=%d\n", j, cntl_page.stree[j]);
    }
    }
  */

  while (qwBlocksToProcess > 0)
    {
      if (dwLevel == 0) // current level is a control 0 level
	{
	  nRes = readDmap(qwPage, page, qwBlocksToProcess);
	  qwBlocksToProcess = 0;
	  if (nRes == -1)
	    RETURN_int(-1);
	}
      else // current level is a control (1 or 2) level
	{
	  qwTemp = my_min(qwBlocksToProcess, qwHandledBlocksSubLevel);
	  nRes = browseBitmap(qwPage, page, dwLevel-1, qwTemp); 
	  qwBlocksToProcess -= qwTemp;
	  if (nRes == -1)
	    RETURN_int(-1);
	}
    }

  RETURN_int(0); // success
}

// =======================================================
void CJfsPart::readBitmap(COptions *options)
{
  BEGIN;

  CJfsDiskInode ino;
  QWORD address;
  xtpage_t *page;
  s64       cntl_addr;
  dbmap_t   cntl_page;
  QWORD i;
  int nRes;

  // Read block allocation map Inode
  //OLDCALL: if (find_inode(BMAP_I, &address, AGGREGATE_I) || xRead(address, sizeof(CJfsDiskInode), (char *)&ino)) 
  if (find_inode(BMAP_I, &address, AGGREGATE_I) || readData((char *)&ino, address, sizeof(CJfsDiskInode))) 
    {
      showDebug(1, "JFS Error in find_inode(BMAP_I, &address, AGGREGATE_I) or readData((char *)&ino, address, sizeof(CJfsDiskInode))\n");
      THROW(ERR_READ_BITMAP);
    }
  showDebug(10, "ADRESS of Inode2 = %llu\n", address);

  showDebug(10, "----------Bitmap inode infos-----------\n");
  showInodeInfos(ino);
  
  page = (xtpage_t *)&(ino.di_btroot);

  // Read overall control page for the map
  //OLDCALL: if (ujfs_rwdaddr(&cntl_addr, page, (s64)0) || xRead(cntl_addr, sizeof(dbmap_t), (char *)&cntl_page)) 
  if (ujfs_rwdaddr(&cntl_addr, page, (s64)0) || readData((char *)&cntl_page, cntl_addr, sizeof(dbmap_t))) 
    {
      showDebug(1, "JFS Error in ujfs_rwdaddr(&cntl_addr, page, (s64)0) || readData((char *)&cntl_page, %lld, sizeof(dbmap_t))\n", cntl_addr);
      THROW(ERR_READ_BITMAP);
    }

  showDebug(10, "\nOverall block allocation map control page at block %llu\n\n", cntl_addr / m_header.qwBlockSize /*>> l2bsize*/);
  readData(&cntl_page, cntl_addr, sizeof(cntl_page));
  showBmapInfos(cntl_page);
  m_info.qwMappedBlocksByBitmap = cntl_page.dn_mapsize;

  // init bitmap
  m_bitmap.init(m_header.qwBitmapSize+1024);

  //m_info.qwOfficialBlocksCount = (sb.s_size * sb.s_pbsize) / sb.s_bsize;
  //m_info.qwOfficialBitmapSize = (m_info.qwOfficialBlocksCount+7)/8; // round up

  m_info.dwAllocTreeMaxLevel = cntl_page.dn_maxlevel; //cntl_page.dn_aglevel;
  showDebug(10, "m_info.dwAllocTreeMaxLevel=%lu\n", m_info.dwAllocTreeMaxLevel);
  QWORD qwRootCtrlPagePos = 1+(2 - m_info.dwAllocTreeMaxLevel); // page 0 = bmap, page 1 = Level2Root, page 2 = Level1Root, page 3 = Lavel0Root
  showDebug(10, "qwRootCtrlPagePos=%llu\n", qwRootCtrlPagePos);
  nRes = browseBitmap(&qwRootCtrlPagePos, 
		      //1 /* tree root = 1 node only */, 
		      page,
		      m_info.dwAllocTreeMaxLevel /* level */,
		      m_info.qwMappedBlocksByBitmap);
  if (nRes)
    {
      showDebug(1, "JFS Error in browseBitmap(&qwRootCtrlPagePos, page, %lu, %llu\n", m_info.dwAllocTreeMaxLevel, m_info.qwMappedBlocksByBitmap);
      THROW(ERR_READ_BITMAP);
    }

  // copy data at the end of the partition (unofficial blocks)
  for (i=m_info.qwMappedBlocksByBitmap; i < m_header.qwBlocksCount; i++) 
    m_bitmap.setBit(i, true);

  // fill informations about free/used clusters count
  calculateSpaceFromBitmap();

  RETURN;
}			

// =======================================================
void CJfsPart::readSuperBlock()
{
  BEGIN;

  jfs_superblock sb;
  int nRes;
  
  // init
  memset(&m_info, 0, sizeof(CInfoJfsHeader));

  // 0. go to the beginning of the super block
  nRes = fseek(m_fDeviceFile, SUPER1_OFF, SEEK_SET);
  if (nRes == -1)
    {
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD) 0L, errno);
    }

  // 1. read and print important informations
  nRes = fread(&sb, sizeof(jfs_superblock), 1, m_fDeviceFile);
  if (nRes != 1)
    {
      g_interface -> ErrorReadingSuperblock(errno);
      THROW(ERR_READING, (DWORD) 0L, errno);
    }
  
  // check the magic number is good
  if (strncmp(sb.s_magic, JFS_MAGIC_STRING, strlen(JFS_MAGIC_STRING)) != 0)
    THROW(ERR_WRONG_FS);
  
  // check for block size
  if (sb.s_bsize != 4096)
    {
      showDebug(1, "Only 4096 bytes per block volumes are supported. Current one is %lu bytes/block", (DWORD)sb.s_bsize);
      //g_interface -> msgBoxError(szTemp);
      THROW(ERR_BLOCKSIZE, "JFS", (DWORD)sb.s_bsize);
    }

  // Label
  memcpy(m_header.szLabel, sb.s_fpack, 11);

  // block informations
  m_header.qwBlockSize = sb.s_bsize;
  m_info.qwOfficialBlocksCount = (sb.s_size * sb.s_pbsize) / sb.s_bsize;
  m_header.qwBlocksCount = (m_qwPartSize) / m_header.qwBlockSize;
  m_header.qwBitmapSize = (m_header.qwBlocksCount+7)/8; // round up

  showDebug(10, "JFS Hardware blocks count: [%llu]\n", m_header.qwBlocksCount);
  //showDebug(10, "JFS Official blocks count: [%llu]\n", m_info.qwOfficialBlocksCount);
  showDebug(10, "JFS Block size: [%llu]\n", m_header.qwBlockSize);

  showDebug(10, "JFS Label:[%s]\n", m_header.szLabel);
  //m_info.qwHardwareBlocksCount = (m_qwPartSize) / m_header.qwBlockSize;
  //showDebug(10, "JFS Hardware blocks count: [%llu]\n", (m_qwPartSize) / m_header.qwBlockSize); //m_info.qwHardwareBlocksCount);
  showDebug(10, "JFS Official blocks count: [%llu]\n", m_info.qwOfficialBlocksCount);
  //showDebug(10, "JFS Block size: [%llu]\n", m_header.qwBlockSize);

  showDebug(10, "\n=========== SUPER BLOCK =========\n");
  showDebug(10, "totalUsable.....%llu\n",(QWORD)sb.totalUsable);
  showDebug(10, "minFree.........%llu\n", (QWORD)sb.minFree);
  showDebug(10, "realFree........%llu\n", (QWORD)sb.realFree);

  setSuperBlockInfos(false, false, 0, (QWORD)(sb.totalUsable*((QWORD)1024)));

  RETURN;
}

// =======================================================
void showInodeInfos(CJfsDiskInode ino)
{
  showDebug(10, "di_inostamp:........0x%.8x\n",(DWORD)ino.di_inostamp);
  showDebug(10, "di_fileset:.........%ld\n",ino.di_fileset);
  showDebug(10, "di_number:..........%lu\n",(DWORD)ino.di_number);
  showDebug(10, "di_gen:.............%lu\n",(DWORD)ino.di_gen);
  showDebug(10, "di_ixpxd.len:.......%lu\n",(DWORD)ino.di_ixpxd.len);
  showDebug(10, "di_ixpxd.addr1:.....0x%.2x\n",(BYTE)ino.di_ixpxd.addr1);
  showDebug(10, "di_ixpxd.addr1:.....0x%.8x\n",(DWORD)ino.di_ixpxd.addr2);
  showDebug(10, "di_ixpxd.address:...%llu\n",(QWORD)addressXAD2(ino.di_ixpxd));
  showDebug(10, "di_size:............0x%.16x\n",(QWORD)ino.di_size);
  showDebug(10, "di_nblocks:.........0x%.16x\n",(QWORD)ino.di_nblocks);
  showDebug(10, "di_nlink::..........%lu\n",(DWORD)ino.di_nlink);
  showDebug(10, "di_uid:.............%lu\n",(QWORD)ino.di_uid);
  showDebug(10, "di_gid:.............%lu\n",(QWORD)ino.di_gid);
  showDebug(10, "di_mode:............0x%.8x\n",(DWORD)ino.di_mode);
}

// =======================================================
void showBmapInfos(dbmap_t cntl_page)
{
  showDebug(10, "[1] dn_mapsize:\t\t0x%011llx = %llu\t", cntl_page.dn_mapsize, cntl_page.dn_mapsize);
  showDebug(10, "[9] dn_agheigth:\t%d\n", cntl_page.dn_agheigth);
  showDebug(10, "[2] dn_nfree:\t\t0x%011llx\t", cntl_page.dn_nfree);
  showDebug(10, "[10] dn_agwidth:\t%d\n", cntl_page.dn_agwidth);
  showDebug(10, "[3] dn_l2nbperpage:\t%d\t\t", cntl_page.dn_l2nbperpage);
  showDebug(10, "[11] dn_agstart:\t%d\n", cntl_page.dn_agstart);
  showDebug(10, "[4] dn_numag:\t\t%d\t\t", cntl_page.dn_numag);
  showDebug(10, "[12] dn_agl2size:\t%d\n", cntl_page.dn_agl2size);
  showDebug(10, "[5] dn_maxlevel:\t%d\t\t", cntl_page.dn_maxlevel);
  showDebug(10, "[13] dn_agfree:\t\ttype 'f'\n");
  showDebug(10, "[6] dn_maxag:\t\t%d\t\t", cntl_page.dn_maxag);
  showDebug(10, "[14] dn_agsize:\t\t%lld\n", cntl_page.dn_agsize);
  showDebug(10, "[7] dn_agpref:\t\t%d\t\t", cntl_page.dn_agpref);
  showDebug(10, "[15] pad:\t\tNot Displayed\n");
  showDebug(10, "[8] dn_aglevel:\t\t%d\n", cntl_page.dn_aglevel);
}

// =======================================================
void CJfsPart::printfInformations()
{
  BEGIN;

  char szText[8192];
  
  getStdInfos(szText, sizeof(szText), true);
  g_interface->msgBoxOk(i18n("JFS informations"), szText);

  RETURN;
}			

// =======================================================
void printLocation(DWORD dwAdress, DWORD dwBlockSize, char *szObject)
{
  DWORD dwBlock, dwOff;
  
  dwOff = dwAdress % dwBlockSize;
  dwBlock = (dwAdress - dwOff) / dwBlockSize;
  showDebug(10, "adress of %s is %lu = block %lu + offset %lu\n", szObject, dwAdress, dwBlock, dwOff);
}

// =======================================================
void copyAndTransformDword(DWORD *dwDest, DWORD *dwSource, DWORD dwElemCount)
{
  BEGIN;

  DWORD i, j;
  DWORD dwTmp, dwTmp2;

  for (i=0; i < dwElemCount; i++)
    {
      dwTmp2 = 0;
      dwTmp = dwSource[i];
      for (j=0; j < 32; j++)
	{
	  dwTmp2 <<= 1;
	  if (dwTmp & 1)
	    dwTmp2 |= 1;
	  dwTmp  >>= 1;
	}
      dwDest[i] = dwTmp2;
      //showDebug(10, "tansf %.8X to %.8X\n", dwSource[i], dwDest[i]);
    }

  RETURN;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% END %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// =======================================================
void CJfsPart::printInodeWhoseOwnerIs(DWORD dwOwner) // FOR DEBUG ONLY
{
  BEGIN;

  DWORD i;
  CJfsDiskInode ino;
  QWORD address;
  xtpage_t *page;
  s64       cntl_addr;
  
  for (i=0; i < 512; i++)
    {
      // Read block allocation map Inode
      //OLDCALL: if (find_inode(i, &address, FILESYSTEM_I) || xRead(address, sizeof(CJfsDiskInode), (char *)&ino)) 
      if (find_inode(i, &address, FILESYSTEM_I) || readData((char *)&ino, address, sizeof(CJfsDiskInode))) 
	{
	  debugWin("Error 1");
	  RETURN;
	}
      
      if (ino.di_uid == dwOwner)
	{
	  showDebug(10, "ADRESS of Inode = %llu\n", address);
	  
	  showDebug(10, "----------Debug: inode infos-----------\n");
	  showInodeInfos(ino);
	  
	  page = (xtpage_t *)&(ino.di_btroot);
	  
	  for (i=0; i < 1000; i++)
	    {
	      if (ujfs_rwdaddr(&cntl_addr, page, (s64)i) == 0)
		showDebug(10, "BLOCK_ADRESS[%lu]: offset %lld = block %lu\n",(DWORD)i, cntl_addr, (cntl_addr / m_header.qwBlockSize));
	    }
	  RETURN;
	}
    }

  RETURN;
}


// =======================================================
void CJfsPart::printFileBlocks(DWORD dwInode) // FOR DEBUG ONLY
{
  BEGIN;

  CJfsDiskInode ino;
  QWORD address;
  xtpage_t *page;
  s64       cntl_addr;
  DWORD i;

   // Read block allocation map Inode
  //OLDCALL: if (find_inode(dwInode, &address, FILESYSTEM_I) || xRead(address, sizeof(CJfsDiskInode), (char *)&ino)) 
  if (find_inode(dwInode, &address, FILESYSTEM_I) || readData((char *)&ino, address, sizeof(CJfsDiskInode))) 
    {
      debugWin("Error 1");
      RETURN;
    }
  showDebug(10, "ADRESS of Inode = %llu\n", address);

  showDebug(10, "----------Debug: inode infos-----------\n");
  showInodeInfos(ino);
  
  page = (xtpage_t *)&(ino.di_btroot);

  for (i=0; i < 1000; i++)
    {
      if (ujfs_rwdaddr(&cntl_addr, page, (s64)i) == 0)
	showDebug(10, "BLOCK_ADRESS[%lu]: offset %lld = block %lu\n",(DWORD)i, cntl_addr, (cntl_addr / m_header.qwBlockSize));
    }

  RETURN;
}			


