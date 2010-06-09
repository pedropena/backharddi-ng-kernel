/***************************************************************************
                    fs_afs.h  -  AtheOS File System Support
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

#ifndef FS_AFS_H
#define FS_AFS_H

#include "partimage.h"
#include "fs_base.h"
#include <asm/types.h>

// ================================================
#define AFS_SUPERBLOCK_SIZE	1024
#define AFS_SUPERBLOCK_OFFSET	1024
#define	SUPER_BLOCK_MAGIC1	0x41465331	/* AFS1 */
#define	SUPER_BLOCK_MAGIC2	0xdd121031
#define	SUPER_BLOCK_MAGIC3	0x15b6830e

// ================================================
#define int32 long
#define uint32 DWORD
#define BlockRun_s DWORD

// ================================================
struct CAfsSuper
{
  char	as_zName[32];
  int32	as_nMagic1; // @32
  int32	as_nByteOrder;
  uint32 as_nBlockSize;
  uint32 as_nBlockShift;
  off_t	as_nNumBlocks;
  off_t	as_nUsedBlocks;

  int32	as_nInodeSize;

  int32	as_nMagic2; // @68
  int32	as_nBlockPerGroup;	// Number of blocks per allocation group (Max 65536)
  int32	as_nAllocGrpShift;	// Number of bits to shift a group number to get a byte address.
  int32	as_nAllocGroupCount;
  int32	as_nFlags;

  BlockRun_s as_sLogBlock;
  off_t	as_nLogStart;
  int as_nValidLogBlocks;
  int as_nLogSize;
  int32	as_nMagic3;

  BlockRun_s as_sRootDir;		// Root dir inode.
  BlockRun_s as_sDeletedFiles;	// Directory containing files scheduled for deletion.
  BlockRun_s as_sIndexDir;		// Directory of index files.
  int as_nBootLoaderSize;
  int32	as_anPad[7];
};

// ================================================
struct CInfoAfsHeader // size must be 16384 (adjust the reserved data)
{
  DWORD	dwByteOrder;
  DWORD dwBlockShift;
  DWORD	dwBlockPerGroup;	// Number of blocks per allocation group (Max 65536)
  DWORD	dwAllocGrpShift;	// Number of bits to shift a group number to get a byte address.
  DWORD	dwAllocGroupCount;
  DWORD	dwFlags;
  DWORD dwBootLoaderSize;
  QWORD qwBitmapStart;
  BYTE cReserved[16384]; // Adjust to fit with total header size
};

// ================================================
class CAfsPart : public CFSBase
{
 public:
  CAfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CAfsPart();
  
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

 private:
  int testRoot(int a, int b);
  bool doesGroupHasSuper(int nGroupDescNb);

 private:
  CInfoAfsHeader m_info;
};


#endif // FS_AFS_H
