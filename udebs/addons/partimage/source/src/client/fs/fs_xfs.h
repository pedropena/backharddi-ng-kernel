/***************************************************************************
                          cxfspart.h  -  description
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

#ifndef FS_XFS_H
#define FS_XFS_H

#include <ctype.h>

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

#define MAX_XFSBLOCKSIZE 65536

// ================================================
// do we need conversion? 
/*
#define ARCH_NOCONVERT 1
#if __BYTE_ORDER == __LITTLE_ENDIAN 
#define ARCH_CONVERT   0
#else
#define ARCH_CONVERT   ARCH_NOCONVERT
#endif

// swapping macros
#define swab16(x) \
        ((unsigned short)( \
                (((unsigned short)(x) & (unsigned short)0x00ffU) << 8) | \
                (((unsigned short)(x) & (unsigned short)0xff00U) >> 8) ))

#define swab32(x) \
        ((unsigned int)( \
                (((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) | \
                (((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) | \
                (((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) | \
                (((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))
#define swab64(x) \
        ((unsigned long long)( \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x00000000000000ffULL) << 56) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x000000000000ff00ULL) << 40) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x0000000000ff0000ULL) << 24) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x00000000ff000000ULL) <<  8) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x000000ff00000000ULL) >>  8) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x0000ff0000000000ULL) >> 24) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x00ff000000000000ULL) >> 40) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0xff00000000000000ULL) >> 56) )) 

#define INT_SWAP(type, var) \
    ((sizeof(type) == 8) ? swab64(var) : \
    ((sizeof(type) == 4) ? swab32(var) : \
    ((sizeof(type) == 2) ? swab16(var) : \
    (var))))

#define INT_GET(reference,arch) \
    (((arch) == ARCH_NOCONVERT) \
        ? \
            (reference) \
        : \
            INT_SWAP((reference),(reference)) \
    )
*/
// ================================================

#define XFS_SUPER_MAGIC 0x58465342

  typedef enum {
	XFS_BTNUM_BNOi, XFS_BTNUM_CNTi, XFS_BTNUM_BMAPi, XFS_BTNUM_INOi,
	XFS_BTNUM_MAX
} xfs_btnum_t;

#define BBSIZE 512
#define	XFS_BTNUM_BNO	((xfs_btnum_t)XFS_BTNUM_BNOi)
#define	XFS_BTNUM_CNT	((xfs_btnum_t)XFS_BTNUM_CNTi)
#define	XFS_BTNUM_BMAP	((xfs_btnum_t)XFS_BTNUM_BMAPi)
#define	XFS_BTNUM_INO	((xfs_btnum_t)XFS_BTNUM_INOi)

#define	XFS_SB_MAGIC		0x58465342	// 'XFSB'
typedef signed char	__int8_t;
typedef unsigned char	__uint8_t;
typedef signed short int	__int16_t;
typedef unsigned short int	__uint16_t;
typedef signed int	__int32_t;
typedef unsigned int	__uint32_t;
#if defined(__alpha__) || defined(__ia64__) || defined(__powerpc64__) || defined(__x86_64__)
typedef signed long int	__int64_t;
typedef unsigned long int	__uint64_t;
#else
typedef signed long long int    __int64_t;
typedef unsigned long long int  __uint64_t;
#endif

// POSIX Extensions
typedef unsigned char	uchar_t;
typedef unsigned short	ushort_t;
typedef unsigned int	uint_t;
typedef unsigned long	ulong_t;

typedef enum { B_FALSE, B_TRUE } boolean_t;

typedef __uint32_t	xfs_agblock_t;	// blockno in alloc. group 
typedef	__uint32_t	xfs_extlen_t;	// extent length in blocks 
typedef	__uint32_t	xfs_agnumber_t;	// allocation group number 
typedef __int32_t	xfs_extnum_t;	// # of extents in a file 
typedef __int16_t	xfs_aextnum_t;	// # extents in an attribute fork 
typedef	__int64_t	xfs_fsize_t;	// bytes in a file 
typedef __uint64_t	xfs_ufsize_t;	// unsigned bytes in a file 

typedef	__int32_t	xfs_suminfo_t;	// type of bitmap summary info 
typedef	__int32_t	xfs_rtword_t;	// word type for bitmap manipulations 

typedef	__int64_t	xfs_lsn_t;	// log sequence number 
typedef	__int32_t	xfs_tid_t;	// transaction identifier 

typedef	__uint32_t	xfs_dablk_t;	// dir/attr block number (in file) 
typedef	__uint32_t	xfs_dahash_t;	// dir/attr hash value 

typedef __uint16_t	xfs_prid_t;	// prid_t truncated to 16bits in XFS 

typedef struct 
{
  unsigned char   __u_bits[16];
} uuid_t;


// These types are 64 bits on disk but are either 32 or 64 bits in memory.
// Disk based types:
typedef __uint64_t	xfs_dfsbno_t;	// blockno in filesystem (agno|agbno) 
typedef __uint64_t	xfs_drfsbno_t;	// blockno in filesystem (raw) 
typedef	__uint64_t	xfs_drtbno_t;	// extent (block) in realtime area 
typedef	__uint64_t	xfs_dfiloff_t;	// block number in a file 
typedef	__uint64_t	xfs_dfilblks_t;	// number of blocks in a file 

#if defined(__alpha__) || defined(__ia64__) || defined(__powerpc64__)
typedef unsigned long  __u64; 
typedef signed long    __s64; 
#else
typedef unsigned long long  __u64; 
typedef signed long long    __s64; 
#endif

typedef __u64	xfs_off_t;
//typedef __s32	xfs32_off_t;
typedef __u64	xfs_ino_t;	// <inode> type 
typedef __s64	xfs_daddr_t;	// <disk address> type 
typedef char *	xfs_caddr_t;	// <core address> type 
typedef off_t	linux_off_t;
//typedef __kernel_ino_t	linux_ino_t;
typedef __uint32_t	xfs_dev_t;

#define XFS_AGF_DADDR           (1)
#define	XFS_AGFL_DADDR		(3)

#define	XFS_AGFL_SIZE		(BBSIZE / sizeof(xfs_agblock_t))

struct xfs_agfl
{
	xfs_agblock_t	agfl_bno[XFS_AGFL_SIZE];
};

struct xfs_superblock
{
  DWORD	sb_magicnum;	// magic number == XFS_SB_MAGIC 
  DWORD	sb_blocksize;	// logical block size, bytes 
  xfs_drfsbno_t	sb_dblocks;	// number of data blocks 
  xfs_drfsbno_t	sb_rblocks;	// number of realtime blocks 
  xfs_drtbno_t	sb_rextents;	// number of realtime extents 
  uuid_t		sb_uuid;	// file system unique id 
  xfs_dfsbno_t	sb_logstart;	// starting block of log if internal 
  xfs_ino_t	sb_rootino;	// root inode number 
  xfs_ino_t	sb_rbmino;	// bitmap inode for realtime extents 

  xfs_ino_t	sb_rsumino;	// summary inode for rt bitmap 

  xfs_agblock_t	sb_rextsize;	// realtime extent size, blocks 

  xfs_agblock_t	sb_agblocks;	// size of an allocation group 
  xfs_agnumber_t	sb_agcount;	// number of allocation groups 
  xfs_extlen_t	sb_rbmblocks;	// number of rt bitmap blocks 

  xfs_extlen_t	sb_logblocks;	// number of log blocks 
  WORD	sb_versionnum;	// header version == XFS_SB_VERSION 
  WORD	sb_sectsize;	// volume sector size, bytes 
  WORD	sb_inodesize;	// inode size, bytes 
  WORD	sb_inopblock;	// inodes per block 
  char		sb_fname[12];	// file system name 
  BYTE	sb_blocklog;	// log2 of sb_blocksize 
  BYTE	sb_sectlog;	// log2 of sb_sectsize 
  BYTE	sb_inodelog;	// log2 of sb_inodesize 
  BYTE	sb_inopblog;	// log2 of sb_inopblock 
  BYTE	sb_agblklog;	// log2 of sb_agblocks (rounded up) 
  BYTE	sb_rextslog;	// log2 of sb_rextents 
  BYTE	sb_inprogress;	// mkfs is in progress, don't mount 
  BYTE	sb_imax_pct;	// max % of fs for inode space 
  // statistics 
  // These fields must remain contiguous.  If you really
  // want to change their layout, make sure you fix the
  // code in xfs_trans_apply_sb_deltas().
  QWORD	sb_icount;	// allocated inodes 
  QWORD	sb_ifree;	// free inodes 
  QWORD	sb_fdblocks;	// free data blocks 
  QWORD	sb_frextents;	// free realtime extents 

  // End contiguous fields.
  xfs_ino_t	sb_uquotino;	// user quota inode 
  xfs_ino_t	sb_gquotino;	// group quota inode 
  WORD	sb_qflags;	// quota flags 
  BYTE	sb_flags;	// misc. flags 
  BYTE	sb_shared_vn;	// shared version number 
  xfs_extlen_t	sb_inoalignmt;	// inode chunk alignment, fsblocks 
  DWORD	sb_unit;	// stripe or raid unit 
  DWORD	sb_width;	// stripe or raid width 	
  BYTE	sb_dirblklog;	// log2 of dir block size (fsbs) 
  BYTE       sb_dummy[7];    // padding 
};


 // Short form header: space allocation btrees.

// =============== xfs_alloc_btree.h ==========================

/*
 * There are two on-disk btrees, one sorted by blockno and one sorted
 * by blockcount and blockno.  All blocks look the same to make the code
 * simpler; if we have time later, we'll make the optimizations.
 */
#define	XFS_ABTB_MAGIC	0x41425442	// 'ABTB' for bno tree 
#define	XFS_ABTC_MAGIC	0x41425443	// 'ABTC' for cnt tree 

// =============== xfs_btree.h ==========================
// Short form header: space allocation btrees.

#define	NULLAGBLOCK	((xfs_agblock_t)-1)

struct xfs_btree_sblock
{
  __uint32_t	bb_magic;	// magic number for block type 
  __uint16_t	bb_level;	// 0 is a leaf 
  __uint16_t	bb_numrecs;	// current # of data records 
  xfs_agblock_t	bb_leftsib;	// left sibling block or NULLAGBLOCK 
  xfs_agblock_t	bb_rightsib;	// right sibling block or NULLAGBLOCK 
};
typedef xfs_btree_sblock xfs_alloc_block;

// Long form header: bmap btrees.
// Combined header and structure, used by common code.
typedef struct xfs_btree_hdr
{
  __uint32_t	bb_magic;	// magic number for block type 
  __uint16_t	bb_level;	// 0 is a leaf 
  __uint16_t	bb_numrecs;	// current # of data records 
} xfs_btree_hdr_t;

typedef struct xfs_btree_block
{
  xfs_btree_hdr_t	bb_h;		// header 
  union		
  {
    struct	
    {
      xfs_agblock_t	bb_leftsib;
      xfs_agblock_t	bb_rightsib;
    }s;		// short form pointers
    struct	
    {
      xfs_dfsbno_t	bb_leftsib;
      xfs_dfsbno_t	bb_rightsib;
    } l;		// long form pointers 
  }bb_u;		// rest 
} xfs_btree_block_t;

typedef xfs_agblock_t xfs_alloc_ptr;	// btree pointer type 

struct xfs_alloc_rec
{
	xfs_agblock_t	ar_startblock;	// starting block number 
	xfs_extlen_t	ar_blockcount;	// count of free blocks 
};// xfs_alloc_rec_t, xfs_alloc_key_t;
typedef xfs_alloc_rec xfs_alloc_key;

// There are two on-disk btrees, one sorted by blockno and one sorted
// by blockcount and blockno.  All blocks look the same to make the code
// simpler; if we have time later, we'll make the optimizations.
#define	XFS_ABTB_MAGIC	0x41425442	// 'ABTB' for bno tree 
#define	XFS_ABTC_MAGIC	0x41425443	// 'ABTC' for cnt tree 

// =============== xfs_ag.h =================================

#define	XFS_AGF_MAGIC	0x58414746	/* 'XAGF' */
#define	XFS_AGI_MAGIC	0x58414749	// 'XAGI'
#define	XFS_AGF_VERSION	1
#define	XFS_AGI_VERSION	1

// Btree number 0 is bno, 1 is cnt.  This value gives the size of the
// arrays below.
#define	XFS_BTNUM_AGF	((int)XFS_BTNUM_CNTi + 1)

// The second word of agf_levels in the first a.g. overlaps the EFS
// superblock's magic number.  Since the magic numbers valid for EFS
// are > 64k, our value cannot be confused for an EFS superblock's.
struct xfs_agf
{
  // Common allocation group header information
  __uint32_t	agf_magicnum;	// magic number == XFS_AGF_MAGIC 
  __uint32_t	agf_versionnum;	// header version == XFS_AGF_VERSION 
  xfs_agnumber_t	agf_seqno;	// sequence # starting from 0 
  xfs_agblock_t	agf_length;	// size in blocks of a.g. 

  // Freespace information
  xfs_agblock_t	agf_roots[XFS_BTNUM_AGF];	// root blocks 
  __uint32_t	agf_spare0;	// spare field 
  __uint32_t	agf_levels[XFS_BTNUM_AGF];	// btree levels 
  __uint32_t	agf_spare1;	// spare field 
  __uint32_t	agf_flfirst;	// first freelist block's index 
  __uint32_t	agf_fllast;	// last freelist block's index 
  __uint32_t	agf_flcount;	// count of blocks in freelist 
  xfs_extlen_t	agf_freeblks;	// total free blocks 
  xfs_extlen_t	agf_longest;	// longest free space 
};

#define	XFS_AGF_MAGICNUM	0x00000001
#define	XFS_AGF_VERSIONNUM	0x00000002
#define	XFS_AGF_SEQNO		0x00000004
#define	XFS_AGF_LENGTH		0x00000008
#define	XFS_AGF_ROOTS		0x00000010
#define	XFS_AGF_LEVELS		0x00000020
#define	XFS_AGF_FLFIRST		0x00000040
#define	XFS_AGF_FLLAST		0x00000080
#define	XFS_AGF_FLCOUNT		0x00000100
#define	XFS_AGF_FREEBLKS	0x00000200
#define	XFS_AGF_LONGEST		0x00000400
#define	XFS_AGF_NUM_BITS	11
#define	XFS_AGF_ALL_BITS	((1 << XFS_AGF_NUM_BITS) - 1)

// disk block (xfs_daddr_t) in the AG 
//#define	XFS_AGF_DADDR		((xfs_daddr_t)1)
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_AGF_BLOCK)
xfs_agblock_t xfs_agf_block(struct xfs_mount *mp);
#define	XFS_AGF_BLOCK(mp)	xfs_agf_block(mp)
#else
#define	XFS_AGF_BLOCK(mp)	XFS_HDR_BLOCK(mp, XFS_AGF_DADDR)
#endif

// ================================================
#define	XFS_BMAP_MAGIC	0x424d4150	// 'BMAP'

// ================================================
struct CInfoXfsHeader // size must be 16384 (adjust the reserved data)
{
  DWORD dwAgCount;
  DWORD dwAgBlocksCount;
  DWORD dwReserved;
  BYTE cReserved[16372]; // Adjust to fit with total header size
};

// ================================================
class CXfsPart: public CFSBase
{
public:
  CXfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CXfsPart();
  
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}
  void scanFreelist(xfs_agf *agf);
  void scanSbtree(xfs_agf *agf, DWORD dwRoot, DWORD dwLevels); 
  void scanfuncBno(char *cBlockData, DWORD dwLevel, xfs_agf *agf);
  void addToHist(DWORD dwAgNo, DWORD dwAgBlockNo, QWORD qwLen);

  QWORD convertAgbToDaddr(DWORD dwAgNo, DWORD dwAgBlockNo);
  QWORD convertAgToDaddr(DWORD dwAgNo, DWORD dwOffset);


 private:
  CInfoXfsHeader m_info;
};

#endif // FS_XFS_H
