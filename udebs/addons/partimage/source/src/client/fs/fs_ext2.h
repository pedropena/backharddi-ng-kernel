/***************************************************************************
           fs_ext2.h  -  EXT2FS (Linux standard) File System Support
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

#ifndef FS_EXT2_H
#define FS_EXT2_H

#include "partimage.h"
#include "fs_base.h"
#include <asm/types.h>

#define LOGICAL_EXT2_BLKSIZE 1024 // 512 // the size of the abstract block used in CFSBase
#define ISEXT3FS (m_info.dwFeatureCompat & EXT3_FEATURE_COMPAT_HAS_JOURNAL)

// ================================================
#define EXT2_SUPER_MAGIC                0xEF53
#define EXT2_BOOTSECT_LEN               1024
#define EXT2_MIN_BLOCK_SIZE		1024
#define	EXT2_MAX_BLOCK_SIZE		4096
#define EXT2_MIN_BLOCK_LOG_SIZE		10

// ================================================
#ifndef EXT2_FEATURE_COMPAT_DIR_PREALLOC
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#endif

#ifndef EXT2_FEATURE_COMPAT_IMAGIC_INODES
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#endif

#ifndef EXT3_FEATURE_COMPAT_HAS_JOURNAL
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#endif

#ifndef EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#endif

#ifndef EXT2_FEATURE_RO_COMPAT_LARGE_FILE
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#endif

#ifndef EXT2_FEATURE_RO_COMPAT_BTREE_DIR
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004
#endif

// ================================================
struct CInfoExt2Header // size must be 16384 (adjust the reserved data)
{
  DWORD dwGroupsCount;
  DWORD dwTotalBlocksCount;
  DWORD dwFirstBlock;
  DWORD dwBlockSize;
  DWORD dwLogicalBlocksPerExt2Block;
  DWORD dwBlocksPerGroup;

  DWORD dwFeatureCompat; 	// compatible feature set 
  DWORD dwFeatureIncompat; 	// incompatible feature set 
  DWORD dwFeatureRoCompat; 	// readonly-compatible feature set 

  DWORD dwRevLevel; // Revision level 
  char cUuid[16]; // 128-bit uuid for volume
  DWORD dwDescBlocks;
  DWORD dwDescPerBlock;

  BYTE cReserved[16320]; // Adjust to fit with total header size
};

// ================================================
class CExt2Part : public CFSBase
{
 public:
  CExt2Part(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CExt2Part();
  
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

 private:
  int testRoot(int a, int b);
  bool doesGroupHasSuper(int nGroupDescNb);

 private:
  CInfoExt2Header m_info;
};

// ================================================
// Structure of the super block
struct CExt2Super 
{
  __u32	s_inodes_count;		// Inodes count
  __u32	s_blocks_count;		// Blocks count
  __u32	s_r_blocks_count;	// Reserved blocks count
  __u32	s_free_blocks_count;	// Free blocks count
  __u32	s_free_inodes_count;	// Free inodes count
  __u32	s_first_data_block;	// First Data Block
  __u32	s_log_block_size;	// Block size
  __s32	s_log_frag_size;	// Fragment size
  __u32	s_blocks_per_group;	// # Blocks per group
  __u32	s_frags_per_group;	// # Fragments per group
  __u32	s_inodes_per_group;	// # Inodes per group
  __u32	s_mtime;		// Mount time
  __u32	s_wtime;		// Write time
  __u16	s_mnt_count;		// Mount count
  __s16	s_max_mnt_count;	// Maximal mount count 
  __u16	s_magic;		// Magic signature
  __u16	s_state;		// File system state
  __u16	s_errors;		// Behaviour when detecting errors 
  __u16	s_minor_rev_level; 	// minor revision level 
  __u32	s_lastcheck;		// time of last check 
  __u32	s_checkinterval;	// max. time between checks 
  __u32	s_creator_os;		// OS 
  __u32	s_rev_level;		// Revision level 
  __u16	s_def_resuid;		// Default uid for reserved blocks 
  __u16	s_def_resgid;		// Default gid for reserved blocks 
  /*
   * These fields are for EXT2_DYNAMIC_REV superblocks only.
   *
   * Note: the difference between the compatible feature set and
   * the incompatible feature set is that if there is a bit set
   * in the incompatible feature set that the kernel doesn't
   * know about, it should refuse to mount the filesystem.
   * 
   * e2fsck's requirements are more strict; if it doesn't know
   * about a feature in either the compatible or incompatible
   * feature set, it must abort and not try to meddle with
   * things it doesn't understand...
   */
  __u32	s_first_ino; 		/* First non-reserved inode */
  __u16 s_inode_size; 		/* size of inode structure */
  __u16	s_block_group_nr; 	/* block group # of this superblock */
  __u32	s_feature_compat; 	/* compatible feature set */
  __u32	s_feature_incompat; 	/* incompatible feature set */
  __u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
  __u8	s_uuid[16];		/* 128-bit uuid for volume */
  char	s_volume_name[16]; 	/* volume name */
  char	s_last_mounted[64]; 	/* directory where last mounted */
  __u32	s_algorithm_usage_bitmap; /* For compression */
  /*
   * Performance hints.  Directory preallocation should only
   * happen if the EXT2_COMPAT_PREALLOC flag is on.
   */
  __u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
  __u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
  __u16	s_padding1;
  __u32	s_reserved[204];	/* Padding to the end of the block */
};

// ================================================
// Structure of a blocks group descriptor
struct CExt2GroupDesc
{
  __u32	bg_block_bitmap;	// Blocks bitmap block 
  __u32	bg_inode_bitmap;	// Inodes bitmap block 
  __u32	bg_inode_table;		// Inodes table block 
  __u16	bg_free_blocks_count;	// Free blocks count 
  __u16	bg_free_inodes_count;	// Free inodes count
  __u16	bg_used_dirs_count;	// Directories count 
  __u16	bg_pad;
  __u32	bg_reserved[3];
};

#endif // FS_EXT2_H
