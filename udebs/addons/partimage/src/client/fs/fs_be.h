/***************************************************************************
                      fs_be.h  -  BeOS File System Support
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

#ifndef FS_BE_H
#define FS_BE_H

#include "partimage.h"
#include "fs_base.h"

// ================================================

// Max name length of BFS
#define BEFS_NAME_LEN 255

#define BEFS_SYMLINK_LEN 160
#define BEFS_NUM_DIRECT_BLOCKS 12
#define B_OS_NAME_LENGTH 32

// Flags of superblock
#define BEFS_CLEAN 0x434c454e
#define BEFS_DIRTY 0x44495254

// Magic headers of BFS's superblock, inode and index
#define BEFS_SUPER_BLOCK_MAGIC1 0x42465331 /* BFS1 */
#define BEFS_SUPER_BLOCK_MAGIC2 0xdd121031
#define BEFS_SUPER_BLOCK_MAGIC3 0x15b6830e
#define BEFS_INODE_MAGIC1 0x3bbe0ad9
#define BEFS_INDEX_MAGIC 0x69f6c2e8
#define BEFS_SUPER_MAGIC BEFS_SUPER_BLOCK_MAGIC1

// ================================================
typedef u64	befs_off_t;
typedef u64	befs_time_t;
typedef void	befs_binode_etc;
typedef struct _befs_block_run 
{
  u32	allocation_group;
  u16	start;
  u16	len;
} befs_block_run;
typedef befs_block_run	befs_inode_addr;

// ================================================
struct CBefsSuper
{
  char		name[B_OS_NAME_LENGTH];
  u32		magic1;
  u32		fs_byte_order;

  u32		block_size;
  u32		block_shift;

  befs_off_t	num_blocks;
  befs_off_t	used_blocks;

  u32		inode_size;

  u32		magic2;
  u32		blocks_per_ag;
  u32		ag_shift;
  u32		num_ags;

  u32		flags;

  befs_block_run	log_blocks;
  befs_off_t	log_start;
  befs_off_t	log_end;

  u32		magic3;
  befs_inode_addr	root_dir;
  befs_inode_addr	indices;

  u32		pad[8];
};

// ================================================
/*
struct CInfoBeHeader // size must be 16384 (adjust the reserved data)
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
*/

// ================================================
/*ass CBefsPart : public CFSBase
{
 public:
  CBefsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CBefsPart();
  
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
};*/


#endif // FS_BE_H
