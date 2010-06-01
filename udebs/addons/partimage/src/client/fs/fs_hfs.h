/***************************************************************************
               fs_hfs.h  - HFS/HFS+: MacOs, Hierarchical File System
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

#ifndef FS_HFS_H
#define FS_HFS_H

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

/* offsets to various blocks */
#define HFS_DD_BLK		0 /* Driver Descriptor block */
#define HFS_PMAP_BLK		1 /* First block of partition map */
#define HFS_MDB_BLK		2 /* Block (w/i partition) of MDB */

/* magic numbers for various disk blocks */
#define HFS_DRVR_DESC_MAGIC	0x4552 /* "ER": driver descriptor map */
#define HFS_OLD_PMAP_MAGIC	0x5453 /* "TS": old-type partition map */
#define HFS_NEW_PMAP_MAGIC	0x504D /* "PM": new-type partition map */
#define HFS_SUPER_MAGIC		0x4244 /* "BD": HFS MDB (super block) */
#define HFS_MFS_SUPER_MAGIC	0xD2D7 /* MFS MDB (super block) */

/* magic numbers for various internal structures */
#define HFS_FILE_MAGIC		0x4801
#define HFS_DIR_MAGIC		0x4802
#define HFS_MDB_MAGIC		0x4803
#define HFS_EXT_MAGIC		0x4804 /* XXX currently unused */
#define HFS_BREC_MAGIC		0x4811 /* XXX currently unused */
#define HFS_BTREE_MAGIC		0x4812
#define HFS_BNODE_MAGIC		0x4813

/* various FIXED size parameters */
#define HFS_SECTOR_SIZE		512    /* size of an HFS sector */
#define HFS_SECTOR_SIZE_BITS	9      /* log_2(HFS_SECTOR_SIZE) */
#define HFS_NAMELEN		31     /* maximum length of an HFS filename */
#define HFS_NAMEMAX		(3*31) /* max size of ENCODED filename */
#define HFS_BM_MAXBLOCKS	(16)   /* max number of bitmap blocks */
#define HFS_BM_BPB (8*HFS_SECTOR_SIZE) /* number of bits per bitmap block */
#define HFS_MAX_VALENCE		32767U
#define HFS_FORK_MAX		(0x7FFFFFFF)

struct hfs_btree;

/* Typedefs for unaligned integer types */
typedef unsigned char hfs_byte_t;
typedef unsigned char hfs_word_t[2];
typedef unsigned char hfs_dword_t[4];
typedef unsigned char hfs_qword_t[8];

// =======================================================
inline BYTE GET_BYTE(hfs_byte_t *a)
{
  BYTE *cPtr;
  cPtr = (BYTE*)a;
  return (*cPtr);
}
inline WORD GET_WORD(hfs_word_t *a)
{
  WORD *wPtr;
  wPtr = (WORD*)a;
  return (*wPtr);
}
inline DWORD GET_DWORD(hfs_dword_t *a)
{
  DWORD *dwPtr;
  dwPtr = (DWORD*)a;
  return (*dwPtr);
}
inline QWORD GET_QWORD(hfs_qword_t *a)
{
  QWORD *qwPtr;
  qwPtr = (QWORD*)a;
  return (*qwPtr);
}


// =======================================================
struct hfsSuperBlock // MDB: Master Data Base
{
  hfs_word_t	drSigWord;	/* Signature word indicating fs type */
  hfs_dword_t	drCrDate;	/* fs creation date/time */
  hfs_dword_t	drLsMod;	/* fs modification date/time */
  hfs_word_t	drAtrb;		/* fs attributes */
  hfs_word_t	drNmFls;	/* number of files in root directory */
  hfs_word_t	drVBMSt;	/* location (in 512-byte blocks)
				   of the volume bitmap */
  hfs_word_t	drAllocPtr;	/* location (in allocation blocks)
				   to begin next allocation search */
  hfs_word_t	drNmAlBlks;	/* number of allocation blocks */
  hfs_dword_t	drAlBlkSize;	/* bytes in an allocation block */
  hfs_dword_t	drClpSiz;	/* clumpsize, the number of bytes to
				   allocate when extending a file */
  hfs_word_t	drAlBlSt;	/* location (in 512-byte blocks)
				   of the first allocation block */
  hfs_dword_t	drNxtCNID;	/* CNID to assign to the next
				   file or directory created */
  hfs_word_t	drFreeBks;	/* number of free allocation blocks */
  hfs_byte_t	drVN[28];	/* the volume label */
  hfs_dword_t	drVolBkUp;	/* fs backup date/time */
  hfs_word_t	drVSeqNum;	/* backup sequence number */
  hfs_dword_t	drWrCnt;	/* fs write count */
  hfs_dword_t	drXTClpSiz;	/* clumpsize for the extents B-tree */
  hfs_dword_t	drCTClpSiz;	/* clumpsize for the catalog B-tree */
  hfs_word_t	drNmRtDirs;	/* number of directories in
				   the root directory */
  hfs_dword_t	drFilCnt;	/* number of files in the fs */
  hfs_dword_t	drDirCnt;	/* number of directories in the fs */
  hfs_byte_t	drFndrInfo[32];	/* data used by the Finder */
  hfs_word_t	drEmbedSigWord;	/* embedded volume signature */
  hfs_dword_t   drEmbedExtent;  /* starting block number (xdrStABN) 
				     and number of allocation blocks 
				     (xdrNumABlks) occupied by embedded
				     volume */
  hfs_dword_t	drXTFlSize;	/* bytes in the extents B-tree */
  hfs_byte_t	drXTExtRec[12];	/* extents B-tree's first 3 extents */
  hfs_dword_t	drCTFlSize;	/* bytes in the catalog B-tree */
  hfs_byte_t	drCTExtRec[12];	/* catalog B-tree's first 3 extents */
} __attribute__((packed));

// ================================================
struct CInfoHfsHeader // size must be 16384 (adjust the reserved data)
{
  QWORD qwAllocCount;
  QWORD qwBitmapSectLocation;
  QWORD qwFreeAllocs;
  QWORD qwFirstAllocBlock;
  //QWORD qwTotalSectCount;
  DWORD dwAllocSize;
  DWORD dwBlocksPerAlloc;
  BYTE cReserved[16344]; // Adjust to fit with total header size
};

// ================================================
class CHfsPart : public CFSBase
{
 public:
  CHfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CHfsPart();
  
  //static  int detect(BYTE *cBootSect, char *szFileSystem); 
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

 private:
  CInfoHfsHeader m_info;
};


#endif // FS_HFS_H
