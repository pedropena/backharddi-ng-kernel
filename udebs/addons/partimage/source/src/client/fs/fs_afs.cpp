/***************************************************************************
                   fs_afs.cpp  -  AtheOS File System Support

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

#include "fs_afs.h"
#include "partimage.h"
#include "imagefile.h"

// =======================================================
CAfsPart::CAfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CAfsPart::~CAfsPart() 
{
  BEGIN;
  RETURN;
}

// =======================================================
void CAfsPart::fsck()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CAfsPart::readBitmap(COptions *options)
{
  BEGIN;
 
  //DWORD i, j, k;
  int nRes;
  //char cBuffer[8192];
  DWORD dwWordsPerBlock;
  DWORD dwBlocksPerGroup;
  QWORD qwCurBlock;

  // init
  dwWordsPerBlock = m_header.qwBlockSize/4;
  qwCurBlock = 0LL;
  dwBlocksPerGroup = m_info.dwBlockPerGroup / (m_header.qwBlockSize * 8);

  // init bitmap size
  nRes = m_bitmap.init(m_header.qwBitmapSize);
  if (nRes == -1)
    goto error_readBitmap;

  // init to false: because real code not implemented
  //for (i=0; i < m_header.qwBlocksCount; i++)
  //  m_bitmap.setBit(i, true);

  // ******************************************************************************

  /*for (i=0; i < m_header.qwBlocksCount; ++i)
    {
      int     nGroup = nBlock / psVolume->av_psSuperBlock->as_nBlockPerGroup;
      int	    nStart = nBlock % psVolume->av_psSuperBlock->as_nBlockPerGroup;
      bool    bFree;

      panBlock = afs_alloc_block_buffer( psVolume );
      if ( panBlock == NULL ) {
	printk( "Error: afs_is_block_free() no memory for bitmap block\n" );
	return( false );
      }
  
      if ( afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + nGroup ) < 0 ) {
	printk( "Panic: afs_is_block_free() failed to load bitmap block\n" );
	return( false );
      }
  
      if ( panBlock[ nStart / 32 ] & (1 << (nStart % 32)) ) {
	bFree = false;
      } else {
	bFree = true;
      }
    }*/

  // ******************************************************************************

  /*for ( i = 0 ; i < m_info.dwAllocGroupCount * dwBlocksPerGroup  ; ++i ) 
    {
      nRes = readData(cBuffer, (m_info.qwBitmapStart+i)*m_header.qwBlockSize, m_header.qwBlockSize);

      memcpy(m_bitmap.data()+(i*m_header.qwBlockSize), cBuffer, m_header.qwBlockSize);

      for ( j = 0 ; j < dwWordsPerBlock ; ++j ) 
	{
	  for ( k = 0 ; k < 32 ; ++k ) 
	    {
	      if ( (cBuffer[j] & (1L << k)) != 0 ) 
		{
		  //nCount++;
		  m_bitmap.setBit(qwCurBlock++, true);
		      
		}
	      else
		m_bitmap.setBit(qwCurBlock++, false);
	    }
	}
    }*/

  // ******************************************************************************

  nRes = readData(m_bitmap.data(), m_info.qwBitmapStart, m_header.qwBitmapSize);  
    if (nRes == -1)
    goto error_readBitmap;

  calculateSpaceFromBitmap();
  showDebug(1, "end success\n");  
  RETURN; // auccess

 error_readBitmap:
  showDebug(1, "end error\n");  
  g_interface->msgBoxError(i18n("There was an error while reading the bitmap"));
  THROW(ERR_READ_BITMAP);
}

/*
// =======================================================
bool afs_is_block_free(QWORD nBlock)
{
    uint32* panBlock;
    int     nGroup = nBlock / psVolume->av_psSuperBlock->as_nBlockPerGroup;
    int	    nStart = nBlock % psVolume->av_psSuperBlock->as_nBlockPerGroup;
    bool    bFree;

    panBlock = afs_alloc_block_buffer( psVolume );
    if ( panBlock == NULL ) {
	printk( "Error: afs_is_block_free() no memory for bitmap block\n" );
	return( false );
    }
  
    if ( afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + nGroup ) < 0 ) {
	printk( "Panic: afs_is_block_free() failed to load bitmap block\n" );
	return( false );
    }
  
    if ( panBlock[ nStart / 32 ] & (1 << (nStart % 32)) ) {
	bFree = false;
    } else {
	bFree = true;
    }
    afs_free_block_buffer( psVolume, panBlock );

    return( bFree );
}*/

// =======================================================
/*off_t afs_count_used_blocks( AfsVolume_s* psVolume ) // code from AFS kernel
{
    AfsSuperBlock_s* psSuperBlock = psVolume->av_psSuperBlock;
    const int	   nBlockSize	= psSuperBlock->as_nBlockSize;
    const int 	   nBlocksPerGroup = psSuperBlock->as_nBlockPerGroup / (nBlockSize * 8);
    const int 	   nWordsPerBlock  = nBlockSize / 4;
    uint32* 	   panBlock;
    off_t	   nCount = 0;
    int		   i;

    panBlock = afs_alloc_block_buffer( psVolume );

    for ( i = 0 ; i < psSuperBlock->as_nAllocGroupCount * nBlocksPerGroup  ; ++i ) 
    {
	int     j;
	int nError;

	nError = afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + i );
    
	for ( j = 0 ; j < nWordsPerBlock ; ++j ) {
	    int k;
	    if ( panBlock[j] == 0 ) {
		continue;
	    }
	    if ( panBlock[j] == ~0 && i * nWordsPerBlock * 32 + j*32 + 32 < psSuperBlock->as_nNumBlocks ) {
		nCount += 32;
		continue;
	    }
	    for ( k = 0 ; k < 32 ; ++k ) {
		if ( i * nWordsPerBlock * 32 + j*32 + k >= psSuperBlock->as_nNumBlocks ) {
		    if ( (panBlock[j] & (1L << k)) == 0 ) {
			printk( "Panic: afs_count_used_blocks() block past the partition-end are marked as free\n" );
		    }
//		    break;
		} else {
		    if ( (panBlock[j] & (1L << k)) != 0 ) {
			nCount++;
		    }
		}
	    }
	}
    }
    afs_free_block_buffer( psVolume, panBlock );

    return( nCount );
}*/

// =======================================================
void CAfsPart::readSuperBlock() 
{
  BEGIN;

  QWORD qwFreeBlocks;
  CAfsSuper sb;
  int nRes;

  // init
  memset(&m_info, 0, sizeof(CInfoAfsHeader));

  // 0. go to the beginning of the super block
  nRes = readData(&sb, AFS_SUPERBLOCK_OFFSET, sizeof(sb));
  if (nRes == -1)
    goto error_readSuperBlock;
  
  if (CpuToLe32(sb.as_nMagic1) != SUPER_BLOCK_MAGIC1)
    goto error_readSuperBlock;

  if (CpuToLe32(sb.as_nMagic2) != SUPER_BLOCK_MAGIC2)
    goto error_readSuperBlock;

  // take infos from superblock
  m_info.dwByteOrder = sb.as_nByteOrder;
  m_info.dwBlockShift = sb.as_nBlockShift;
  m_info.dwBlockPerGroup = sb.as_nBlockPerGroup;
  m_info.dwAllocGrpShift = sb.as_nAllocGrpShift;
  m_info.dwAllocGroupCount = sb.as_nAllocGroupCount;
  m_info.dwFlags = sb.as_nFlags;
  m_info.dwBootLoaderSize = sb.as_nBootLoaderSize;

  // virtual blocks used in the abstract CFSBase
  m_header.qwBlocksCount = (DWORD)sb.as_nNumBlocks;
  showDebug(1, "qwBlocksCount=%llu\n", m_header.qwBlocksCount);
  m_header.qwBlockSize = (DWORD)sb.as_nBlockSize;
  showDebug(1, "qwBlockSize=%llu\n", m_header.qwBlockSize);
  m_header.qwBitmapSize = ((m_header.qwBlocksCount+7)/ 8)+1024;
  m_header.qwUsedBlocks = sb.as_nUsedBlocks;
  showDebug(1, "qwUsedBlocks%llu\n", m_header.qwUsedBlocks);

  // get bitmap position
  /*DWORD a, b, c;
  a = (m_info.dwBootLoaderSize*m_header.qwBlockSize + AFS_SUPERBLOCK_SIZE + 1024 + m_header.qwBlockSize - 1) / m_header.qwBlockSize;
  b = (m_info.dwBootLoaderSize*m_header.qwBlockSize + AFS_SUPERBLOCK_SIZE + 1024 + m_header.qwBlockSize) / m_header.qwBlockSize;
  c = (m_info.dwBootLoaderSize*m_header.qwBlockSize + AFS_SUPERBLOCK_SIZE + 1024 + m_header.qwBlockSize + 1) / m_header.qwBlockSize;
  showDebug(1, "a=%lu\n", a);
  showDebug(1, "b=%lu\n", b);
  showDebug(1, "c=%lu\n", c);
*/

  m_info.qwBitmapStart = (m_info.dwBootLoaderSize*m_header.qwBlockSize + AFS_SUPERBLOCK_SIZE + 1024 + m_header.qwBlockSize - 1) / m_header.qwBlockSize;

  qwFreeBlocks = m_header.qwBlocksCount-m_header.qwUsedBlocks;
  setSuperBlockInfos(false, false, m_header.qwUsedBlocks*m_header.qwBlockSize, qwFreeBlocks * m_header.qwBlockSize);

  showDebug(1, "end success\n");  
  RETURN;

 error_readSuperBlock:
  g_interface -> ErrorReadingSuperblock(errno);
  THROW(ERR_WRONG_FS);
}

// =======================================================
void CAfsPart::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  
  *(szText) = '\0';
  getStdInfos(szText, sizeof(szText), false); 
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Allocation groups count:......%lu\n"
			    "Byte order:...................%lu\n"
			    "Blocks per group:.............%lu\n"
			    "Boot loader size:.............%lu blocks\n"),
	   szText, m_info.dwAllocGroupCount, m_info.dwByteOrder,
	   m_info.dwBlockPerGroup, m_info.dwBootLoaderSize);
  
  g_interface->msgBoxOk(i18n("AFS informations"), szFullText);
}	

