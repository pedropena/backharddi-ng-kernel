/***************************************************************************
                          creiserpart.h  -  description
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

#ifndef FS_REISER_H
#define FS_REISER_H

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

#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"
#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)

#define MAX_REISER_BLOCK_SIZE						8192

#define VER_3_5 1
#define VER_3_6 2

// ================================================
struct reiserfs_super_block
{
  /*__uu32*/ DWORD s_block_count;			/* blocks count         */
  /*__uu32*/ DWORD s_free_blocks;                  /* free blocks count    */
  /*__uu32*/ DWORD s_root_block;           	/* root block number    */
  /*__uu32*/ DWORD s_journal_block;           	/* journal block number    */
  /*__uu32*/ DWORD s_journal_dev;           	/* journal device number  */
  /*__uu32*/ DWORD s_orig_journal_size; 		/* size of the journal on FS creation.  used to make sure they don't overflow it */
  /*__uu32*/ DWORD s_journal_trans_max ;           /* max number of blocks in a transaction.  */
  /*__uu32*/ DWORD s_journal_block_count ;         /* total size of the journal. can change over time  */
  /*__uu32*/ DWORD s_journal_max_batch ;           /* max number of blocks to batch into a trans */
  /*__uu32*/ DWORD s_journal_max_commit_age ;      /* in seconds, how old can an async commit be */
  /*__uu32*/ DWORD s_journal_max_trans_age ;       /* in seconds, how old can a transaction be */
  /*__uu16*/ WORD s_blocksize;                   	/* block size           */
  /*__uu16*/ WORD s_oid_maxsize;			/* max size of object id array, see get_objectid() commentary  */
  /*__uu16*/ WORD s_oid_cursize;			/* current size of object id array */
  /*__uu16*/ WORD s_state;                       	/* valid or error       */
  char s_magic[12];                     /* reiserfs magic string indicates that file system is reiserfs */
  /*__uu32*/ DWORD s_hash_function_code;		/* indicate, what hash fuction is being use to sort names in a directory*/
  /*__uu16*/ WORD s_tree_height;                  /* height of disk tree */
  /*__uu16*/ WORD s_bmap_nr;                      /* amount of bitmap blocks needed to address each block of file system */
  /*__uu16*/ WORD s_reserved;
};

#define SB_SIZE (sizeof(struct reiserfs_super_block))

// ================================================
struct CInfoReiserHeader // size must be 16384 (adjust the reserved data)
{
  DWORD dwVersion;
  DWORD dwBitmapBlocksCount;
  
  BYTE cReserved[16376]; // Adjust to fit with total header size
};

// ================================================
class CReiserPart: public CFSBase
{
public:
  CReiserPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CReiserPart();
  
  static  int detect(BYTE *cBootSect, char *szFileSystem); 
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

 private:
  CInfoReiserHeader m_info;
};

#endif // FS_REISER_H
