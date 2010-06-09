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

#ifndef FS_JFS_H
#define FS_JFS_H

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

#define JFS_MAGIC_STRING      "JFS1"  // Magic word: Version 1 
#define  JFS_SUPER_MAGIC      0x3153464A

#define int16 signed short int
#define uint16 unsigned short int
#define int32 signed long int
#define uint32 unsigned long int
#define int64  signed long long int
#define uint64  unsigned long long int
#define int8  signed char
#define uint8  unsigned char

#define s16 signed short int
#define u16 unsigned short int
#define s32 signed long int
#define u32 unsigned long int
#define s64  signed long long int
#define u64  unsigned long long int
#define s8  signed char
#define u8  unsigned char

void copyAndTransformDword(DWORD *dwDest, DWORD *dwSource, DWORD dwElemCount);

#define __le64_to_cpu(x) ((u64)(x))
#define __le32_to_cpu(x) ((u32)(x))
#define __le16_to_cpu(x) ((u16)(x))

#define XT_CMP(CMP, K, X) \
{ \
	s64 offset64 = offsetXAD(X); \
	(CMP) = ((K) >= offset64 + lengthXAD(X)) ? 1 : \
		((K) < offset64) ? -1 : 0 ; \
}

// ================================================
/*
 *	buffer cache configuration
 */
/* page size */
#ifdef PSIZE
#undef PSIZE
#endif
#define	PSIZE		4096		/* page size (in byte) */
#define	L2PSIZE		12		/* log2(PSIZE) */
#define	POFFSET		4095		/* offset within page */

/* buffer page size */
#define BPSIZE	PSIZE		


/*
 *	fs fundamental size
 *
 * PSIZE >= file system block size >= PBSIZE >= DISIZE
 */
#define	PBSIZE		512		/* physical block size (in byte) */
#define	L2PBSIZE	9		/* log2(PBSIZE) */

#define DISIZE		512		/* on-disk inode size (in byte) */
#define L2DISIZE	9		/* log2(DISIZE) */

#define IDATASIZE	256		/* inode inline data size */
#define	IXATTRSIZE	128		/* inode inline extended attribute size */

#define XTPAGE_SIZE     4096
#define log2_PAGESIZE     12

#define IAG_SIZE        4096
#define IAG_EXTENT_SIZE 4096
#define	INOSPERIAG	4096		/* number of disk inodes per iag */
#define	L2INOSPERIAG	12		/* l2 number of disk inodes per iag */
#define INOSPEREXT	32		/* number of disk inode per extent */
#define L2INOSPEREXT	5		/* l2 number of disk inode per extent */
#define	IXSIZE		(DISIZE * INOSPEREXT)	/* inode extent size */
#define	INOSPERPAGE	8		/* number of disk inodes per 4K page */
#define	L2INOSPERPAGE	3		/* log2(INOSPERPAGE) */

#define	IAGFREELIST_LWM	64

#define INODE_EXTENT_SIZE	IXSIZE	/* inode extent size */
#define NUM_INODE_PER_EXTENT	INOSPEREXT
#define NUM_INODE_PER_IAG	INOSPERIAG

#define MINBLOCKSIZE		512
#define MAXBLOCKSIZE		4096
#define	MAXFILESIZE		((int64)1 << 52)

#define JFS_LINK_MAX		65535	/* nlink_t is unsigned short */







/*
 * fixed physical block address (physical block size = 512 byte)
 *
 * NOTE: since we can't guarantee a physical block size of 512 bytes the use of
 *	 these macros should be removed and the byte offset macros used instead.
 */
#define SUPER1_B	64		/* primary superblock */
#define	AIMAP_B		(SUPER1_B + 8)	/* 1st extent of aggregate inode map */
#define	AITBL_B		(AIMAP_B + 16)	/*
					 * 1st extent of aggregate inode table
					 */
#define	SUPER2_B	(AITBL_B + 32)	/* 2ndary superblock pbn */
#define	BMAP_B		(SUPER2_B + 8)	/* block allocation map */

/*
 * SIZE_OF_SUPER defines the total amount of space reserved on disk for the
 * superblock.  This is not the same as the superblock structure, since all of
 * this space is not currently being used.
 */
#define SIZE_OF_SUPER	PSIZE

/*
 * SIZE_OF_AG_TABLE defines the amount of space reserved to hold the AG table
 */
#define SIZE_OF_AG_TABLE	PSIZE

/*
 * SIZE_OF_MAP_PAGE defines the amount of disk space reserved for each page of
 * the inode allocation map (to hold iag)
 */
#define SIZE_OF_MAP_PAGE	PSIZE

/*
 * fixed byte offset address
 */
#define SUPER1_OFF	0x8000		/* primary superblock */
#define AIMAP_OFF	(SUPER1_OFF + SIZE_OF_SUPER)
					/*
					 * Control page of aggregate inode map
					 * followed by 1st extent of map
					 */
#define AITBL_OFF	(AIMAP_OFF + (SIZE_OF_MAP_PAGE << 1))
					/* 
					 * 1st extent of aggregate inode table
					 */
#define SUPER2_OFF	(AITBL_OFF + INODE_EXTENT_SIZE)
					/*
					 * secondary superblock
					 */
#define BMAP_OFF	(SUPER2_OFF + SIZE_OF_SUPER)
					/*
					 * block allocation map
					 */

/*
 * The following macro is used to indicate the number of reserved disk blocks at
 * the front of an aggregate, in terms of physical blocks.  This value is
 * currently defined to be 32K.  This turns out to be the same as the primary
 * superblock's address, since it directly follows the reserved blocks.
 */
#define AGGR_RSVD_BLOCKS	SUPER1_B

/*
 * The following macro is used to indicate the number of reserved bytes at the
 * front of an aggregate.  This value is currently defined to be 32K.  This
 * turns out to be the same as the primary superblock's byte offset, since it
 * directly follows the reserved blocks.
 */
#define AGGR_RSVD_BYTES	SUPER1_OFF

/*
 * The following macro defines the byte offset for the first inode extent in
 * the aggregate inode table.  This allows us to find the self inode to find the
 * rest of the table.  Currently this value is 44K.
 */
#define AGGR_INODE_TABLE_START	AITBL_OFF






/* Minimum n
 *	 file system option (superblock flag)
 */
/* platform option (conditional compilation) */
#define JFS_AIX		0x80000000	/* AIX support */
/*	POSIX name/directory  support */

#define JFS_OS2		0x40000000	/* OS/2 support */
/*	case-insensitive name/directory support */

#define JFS_DFS		0x20000000	/* DCE DFS LFS support */

#define JFS_LINUX      	0x10000000	/* Linux support */
/*	case-sensitive name/directory support */

/* directory option */
#define JFS_UNICODE	0x00000001	/* unicode name */

/* commit option */
#define	JFS_COMMIT	0x00000f00	/* commit option mask */
#define	JFS_GROUPCOMMIT	0x00000100	/* group (of 1) commit */
#define	JFS_LAZYCOMMIT	0x00000200	/* lazy commit */
#define	JFS_TMPFS	0x00000400	/* temporary file system - 
					 * do not log/commit:
					 */

/* log logical volume option */
#define	JFS_INLINELOG	0x00000800	/* inline log within file system */
#define JFS_INLINEMOVE	0x00001000	/* inline log being moved */

/* Secondary aggregate inode table */
#define JFS_BAD_SAIT	0x00010000	/* current secondary ait is bad */

/* sparse regular file support */
#define JFS_SPARSE	0x00020000	/* sparse regular file */

/* DASD Limits		F226941 */
#define JFS_DASD_ENABLED	0x00040000	/* DASD limits enabled */
#define	JFS_DASD_PRIME		0x00080000	/* Prime DASD usage on boot */
// number of bytes supported for a JFS partition */
#define MINJFS			(0x1000000)
#define MINJFSTEXT		"16"



/* possible values for maxentry */
#define XTROOTINITSLOT  10
#define XTROOTMAXSLOT   18
#define XTPAGEMAXSLOT   256
#define XTENTRYSTART    2


/*
 *	fixed reserved inode number
 */
/* aggregate inode */
#define AGGR_RESERVED_I	0		// aggregate inode (reserved) 
#define	AGGREGATE_I	1		// aggregate inode map inode 
#define	BMAP_I		2		// aggregate block allocation map inode 
#define	LOG_I		3		// aggregate inline log inode 
#define BADBLOCK_I	4		// aggregate bad block inode 
#define	FILESYSTEM_I	16		// 1st/only fileset inode in ait:

/*
 *      extent allocation descriptor (xad)
 */
typedef struct xad // 16 bytes
{
        unsigned        flag:8;         /* 1: flag */
        unsigned        rsvrd:16;       /* 2: reserved */
        unsigned        off1:8;         /* 1: offset in unit of fsblksize */
        uint32          off2;           /* 4: offset in unit of fsblksize */
        unsigned        len:24;         /* 3: length in unit of fsblksize */
        unsigned        addr1:8;        /* 1: address in unit of fsblksize */
        uint32          addr2;          /* 4: address in unit of fsblksize */
} xad_t;                                /* (16) */


/*
 *      xtree page:
 */
typedef union {
	struct xtheader {
		s64 next;	/* 8: */
		s64 prev;	/* 8: */

		u8 flag;	/* 1: */
		u8 rsrvd1;	/* 1: */
		s16 nextindex;	/* 2: next index = number of entries */
		s16 maxentry;	/* 2: max number of entries */
		s16 rsrvd2;	/* 2: */

		QWORD qwSelf; //pxd_t self;	/* 8: self */
	} header;		/* (32) */

	xad_t xad[XTPAGEMAXSLOT];	/* 16 * maxentry: xad array */
} xtpage_t;




/* btpaget_t flag */
#define BT_TYPE		0x07	/* B+-tree index */
#define	BT_ROOT		0x01	/* root page */
#define	BT_LEAF		0x02	/* leaf page */
#define	BT_INTERNAL	0x04	/* internal page */
#define	BT_RIGHTMOST	0x10	/* rightmost page */
#define	BT_LEFTMOST	0x20	/* leftmost page */

/* i_jfs_btorder (in inode) */
#define	BT_RANDOM		0x0000
#define	BT_SEQUENTIAL		0x0001
#define	BT_LOOKUP		0x0010
#define	BT_INSERT		0x0020
#define	BT_DELETE		0x0040



/* xad_t field construction */
#define XADoffset(xad, offset64)\
{\
        (xad)->off1 = ((uint64)offset64) >> 32;\
        (xad)->off2 = (offset64) & 0xffffffff;\
}
#define XADaddress(xad, address64)\
{\
        (xad)->addr1 = ((uint64)address64) >> 32;\
        (xad)->addr2 = (address64) & 0xffffffff;\
}
#define XADlength(xad, length32)        (xad)->len = length32

/* xad_t field extraction */
#define offsetXAD(xad)\
        ( ((int64)((xad)->off1)) << 32 | (xad)->off2 )
#define addressXAD(xad)\
        ( ((int64)((xad)->addr1)) << 32 | (xad)->addr2 )
#define lengthXAD(xad)  ( (xad)->len )

#define offsetXAD2(xad)  ( ((int64)((xad).off1)) << 32 | (xad).off2 )
#define addressXAD2(xad) ( ((int64)((xad).addr1)) << 32 | (xad).addr2 )
#define lengthXAD2(xad)  ( (xad).len )

/* xd_t field extraction */
#define	lengthPXD(pxd)	__le24_to_cpu((pxd)->len)
#define	addressPXD(pxd)\
	( ((s64)((pxd)->addr1)) << 32 | __le32_to_cpu((pxd)->addr2))


/* xad list */
typedef struct {
        int16   maxnxad;
        int16   nxad;
        xad_t   *xad;
} xadlist_t;

/* xad_t flags */
#define XAD_NEW         0x01    /* new */
#define XAD_EXTENDED    0x02    /* extended */
#define XAD_COMPRESSED  0x04    /* compressed with recorded length */
#define XAD_NOTRECORDED 0x08    /* allocated but not recorded */
#define XAD_COW         0x10    /* copy-on-write */

#define	INOTOIAG(ino)	((ino) >> L2INOSPERIAG)


/*
 *	physical xd (pxd)
 */
typedef struct 
{
	unsigned	len:24;
	unsigned	addr1:8;
	uint32		addr2;
} pxd_t;


/*
 *	simple extent descriptor (xd)
 */
typedef struct // 8 bytes
{
	unsigned	flag:8;		/* 1: flags */
	unsigned	rsrvd:8;	/* 1: */
	unsigned	len:8;		/* 1: length in unit of fsblksize */
	unsigned	addr1:8;	/* 1: address in unit of fsblksize */
	uint32		addr2;		/* 4: address in unit of fsblksize */
} xd_t;					/* - 8 - */

/* di_mode */
/*
 * This is the way OS/2 defines the mode.  That's okay for the utilities
 * which are dealing directly with the disk i-node.  The filesystem itself
 * should use the standard S_IFMT, etc. defines in stat.h
 */
#define IFMT	0xF000		/* S_IFMT - mask of file type */
#define IFDIR	0x4000		/* S_IFDIR - directory */
#define IFREG	0x8000		/* S_IFREG - regular file */
#define IFLNK	0xA000		/* S_IFLNK - symbolic link */
#define IFBLK	0x6000		/* S_IFBLK - block special file */
#define IFCHR	0x2000		/* S_IFCHR - character special file */
#define IFFIFO	0x1000		/* S_IFFIFO - FIFO */
#define IFSOCK	0xC000		/* S_IFSOCK - socket */

#define ISUID	0x0800		/* S_ISUID - set user id when exec'ing */
#define ISGID	0x0400		/* S_ISGID - set group id when exec'ing */

#define IREAD	0x0100		/* S_IRUSR - read permission */
#define IWRITE	0x0080		/* S_IWUSR - write permission */
#define IEXEC	0x0040		/* S_IXUSR - execute permission */

/* extended mode bits (on-disk inode di_mode) */
#define IFJOURNAL       0x00010000	/* journalled file */
#define ISPARSE         0x00020000	/* sparse file enabled */
#define INLINEEA        0x00040000	/* inline EA area free */
#define ISWAPFILE	0x00800000	/* file open for pager swap space */


#define BMAPVERSION	1	/* version number */
#define	TREESIZE	(256+64+16+4+1)	/* size of a dmap tree */
#define	LEAFIND		(64+16+4+1)	/* index of 1st leaf of a dmap tree */
#define LPERDMAP	256	/* num leaves per dmap tree */
#define L2LPERDMAP	8	/* l2 number of leaves per dmap tree */
#define	DBWORD		32	/* # of blks covered by a map word */
#define	L2DBWORD	5	/* l2 # of blks covered by a mword */
#define BUDMIN  	L2DBWORD	/* max free string in a map word */
#define BPERDMAP	(LPERDMAP * DBWORD)	/* num of blks per dmap */
#define L2BPERDMAP	13	/* l2 num of blks per dmap */
#define CTLTREESIZE	(1024+256+64+16+4+1)	/* size of a dmapctl tree */
#define CTLLEAFIND	(256+64+16+4+1)	/* idx of 1st leaf of a dmapctl tree */
#define LPERCTL		1024	/* num of leaves per dmapctl tree */
#define L2LPERCTL	10	/* l2 num of leaves per dmapctl tree */
#define	ROOT		0	/* index of the root of a tree */
#define	NOFREE		((s8) -1)	/* no blocks free */
#define	MAXAG		128	/* max number of allocation groups */
#define L2MAXAG		7	/* l2 max num of AG */
#define L2MINAGSZ	25	/* l2 of minimum AG size in bytes */
#define	BMAPBLKNO	0	/* lblkno of bmap within the map */

/*
 *	disk map control page per level.
 *
 * dmapctl_t must be consistent with dmaptree_t.
 */
typedef struct {
	s32 nleafs;		/* 4: number of tree leafs      */
	s32 l2nleafs;		/* 4: l2 number of tree leafs   */
	s32 leafidx;		/* 4: index of the first tree leaf      */
	s32 height;		/* 4: height of tree            */
	s8 budmin;		/* 1: minimum l2 tree leaf value        */
	s8 stree[CTLTREESIZE];	/* CTLTREESIZE: dmapctl tree    */
	u8 pad[2714];		/* 2714: pad to 4096            */
} dmapctl_t;			/* - 4096 -                     */

/*
 *	dmap summary tree
 *
 * dmaptree_t must be consistent with dmapctl_t.
 */
typedef struct {
	s32 nleafs;		/* 4: number of tree leafs      */
	s32 l2nleafs;		/* 4: l2 number of tree leafs   */
	s32 leafidx;		/* 4: index of first tree leaf  */
	s32 height;		/* 4: height of the tree        */
	s8 budmin;		/* 1: min l2 tree leaf value to combine */
	s8 stree[TREESIZE];	/* TREESIZE: tree               */
	u8 pad[2];		/* 2: pad to word boundary      */
} dmaptree_t;			/* - 360 -                      */

/*
 *	dmap page per 8K blocks bitmap
 */
typedef struct {
	s32 nblocks;		/* 4: num blks covered by this dmap     */
	s32 nfree;		/* 4: num of free blks in this dmap     */
	s64 start;		/* 8: starting blkno for this dmap      */
	dmaptree_t tree;	/* 360: dmap tree                       */
	u8 pad[1672];		/* 1672: pad to 2048 bytes              */
	u32 wmap[LPERDMAP];	/* 1024: bits of the working map        */
	u32 pmap[LPERDMAP];	/* 1024: bits of the persistent map     */
} dmap_t;			/* - 4096 -                             */


/* 
 *	on-disk aggregate disk allocation map descriptor.
 */
typedef struct {
	s64 dn_mapsize;		/* 8: number of blocks in aggregate     */
	s64 dn_nfree;		/* 8: num free blks in aggregate map    */
	s32 dn_l2nbperpage;	/* 4: number of blks per page           */
	s32 dn_numag;		/* 4: total number of ags               */
	s32 dn_maxlevel;	/* 4: number of active ags              */
	s32 dn_maxag;		/* 4: max active alloc group number     */
	s32 dn_agpref;		/* 4: preferred alloc group (hint)      */
	s32 dn_aglevel;		/* 4: dmapctl level holding the AG      */
	s32 dn_agheigth;	/* 4: height in dmapctl of the AG       */
	s32 dn_agwidth;		/* 4: width in dmapctl of the AG        */
	s32 dn_agstart;		/* 4: start tree index at AG height     */
	s32 dn_agl2size;	/* 4: l2 num of blks per alloc group    */
	s64 dn_agfree[MAXAG];	/* 8*MAXAG: per AG free count           */
	s64 dn_agsize;		/* 8: num of blks per alloc group       */
	s8 dn_maxfreebud;	/* 1: max free buddy system             */
	u8 pad[3007];		/* 3007: pad to 4096                    */
} dbmap_t;


#define	MUTEXLOCK_T struct semaphore

/* 
 *	in-memory aggregate disk allocation map descriptor.
 */
/*typedef struct bmap {
	dbmap_t db_bmap;	// on-disk aggregate map descriptor 
	struct inode *db_ipbmap;	// ptr to aggregate map incore inode 
	MUTEXLOCK_T db_bmaplock;	// aggregate map lock 
	u32 *db_DBmap;
} bmap_t;*/


/*
 *	jfs_imap.h: disk inode manager
 */

#define	EXTSPERIAG	128	/* number of disk inode extent per iag  */
#define IMAPBLKNO	0	/* lblkno of dinomap within inode map   */
#define SMAPSZ		4	/* number of words per summary map      */
#define	EXTSPERSUM	32	/* number of extents per summary map entry */
#define	L2EXTSPERSUM	5	/* l2 number of extents per summary map */
#define	PGSPERIEXT	4	/* number of 4K pages per dinode extent */
#define	MAXIAGS		((1<<20)-1)	/* maximum number of iags       */
#define	MAXAG		128	/* maximum number of allocation groups  */

#define AMAPSIZE      512	/* bytes in the IAG allocation maps */
#define SMAPSIZE      16	/* bytes in the IAG summary maps */

/* convert inode number to iag number */
#define	INOTOIAG(ino)	((ino) >> L2INOSPERIAG)

/* convert iag number to logical block number of the iag page */
#define IAGTOLBLK(iagno,l2nbperpg)	(((iagno) + 1) << (l2nbperpg))

/* get the starting block number of the 4K page of an inode extent
 * that contains ino.
 */
#define INOPBLK(pxd,ino,l2nbperpg)    	(addressPXD((pxd)) +		\
	((((ino) & (INOSPEREXT-1)) >> L2INOSPERPAGE) << (l2nbperpg)))

/*
 *	inode allocation map:
 * 
 * inode allocation map consists of 
 * . the inode map control page and
 * . inode allocation group pages (per 4096 inodes)
 * which are addressed by standard JFS xtree.
 */
/*
 *	inode allocation group page (per 4096 inodes of an AG)
 */
typedef struct {
	s64 agstart;		/* 8: starting block of ag              */
	s32 iagnum;		/* 4: inode allocation group number     */
	s32 inofreefwd;		/* 4: ag inode free list forward        */
	s32 inofreeback;	/* 4: ag inode free list back           */
	s32 extfreefwd;		/* 4: ag inode extent free list forward */
	s32 extfreeback;	/* 4: ag inode extent free list back    */
	s32 iagfree;		/* 4: iag free list                     */

	/* summary map: 1 bit per inode extent */
	s32 inosmap[SMAPSZ];	/* 16: sum map of mapwords w/ free inodes;
				 *      note: this indicates free and backed
				 *      inodes, if the extent is not backed the
				 *      value will be 1.  if the extent is
				 *      backed but all inodes are being used the
				 *      value will be 1.  if the extent is
				 *      backed but at least one of the inodes is
				 *      free the value will be 0.
				 */
	s32 extsmap[SMAPSZ];	/* 16: sum map of mapwords w/ free extents */
	s32 nfreeinos;		/* 4: number of free inodes             */
	s32 nfreeexts;		/* 4: number of free extents            */
	/* (72) */
	u8 pad[1976];		/* 1976: pad to 2048 bytes */
	/* allocation bit map: 1 bit per inode (0 - free, 1 - allocated) */
	u32 wmap[EXTSPERIAG];	/* 512: working allocation map  */
	u32 pmap[EXTSPERIAG];	/* 512: persistent allocation map */
	pxd_t inoext[EXTSPERIAG];	/* 1024: inode extent addresses */
} iag_t;			/* (4096) */










/* 
 *	aggregate superblock 
 *
 * The name superblock is too close to super_block, so the name has been
 * changed to jfs_superblock.  The utilities are still using the old name.
 */
struct jfs_superblock
{
	char	s_magic[4];	/* 4: magic number */
	uint32	s_version;	/* 4: version number */

	int64	s_size;		/* 8: aggregate size in hardware/LVM blocks;
				 * VFS: number of blocks
				 */
	int32	s_bsize;	/* 4: aggregate block size in bytes; 
				 * VFS: fragment size
				 */
	int16	s_l2bsize;	/* 2: log2 of s_bsize */
	int16	s_l2bfactor;	/* 2: log2(s_bsize/hardware block size) */
	int32	s_pbsize;	/* 4: hardware/LVM block size in bytes */
	int16	s_l2pbsize;	/* 2: log2 of s_pbsize */
	int16	pad;		/* 2: padding necessary for alignment */

	uint32	s_agsize;	/* 4: allocation group size in aggr. blocks */

	uint32	s_flag;		/* 4: aggregate attributes:
				 *    see jfs_filsys.h
				 */ 
	uint32	s_state;	/* 4: mount/unmount/recovery state: 
				 *    see jfs_filsys.h
				 */
	int32	s_compress;	/* 4: > 0 if data compression */

	pxd_t	s_ait2;		/* 8: first extent of secondary
				 *    aggregate inode table
				 */

	pxd_t	s_aim2;		/* 8: first extent of secondary
				 *    aggregate inode map
				 */
	uint32	s_logdev;	/* 4: device address of log */
	int32	s_logserial;	/* 4: log serial number at aggregate mount */
	pxd_t	s_logpxd;	/* 8: inline log extent */

	pxd_t	s_fsckpxd;	/* 8: inline fsck work space extent */

        QWORD qwTime; //struct timestruc_t s_time; /* 8: time last updated */

        int32   s_fsckloglen;   /* 4: Number of filesystem blocks reserved for
                                 *    the fsck service log.  
                                 *    N.B. These blocks are divided among the
                                 *         versions kept.  This is not a per
                                 *         version size.
                                 *    N.B. These blocks are included in the 
                                 *         length field of s_fsckpxd.
                                 */
        int8    s_fscklog;      /* 1: which fsck service log is most recent
                                 *    0 => no service log data yet
                                 *    1 => the first one
                                 *    2 => the 2nd one
                                 */
	char	s_fpack[11];	/* 11: file system volume name 
                                 *     N.B. This must be 11 bytes to
                                 *          conform with the OS/2 BootSector
                                 *          requirements
                                 */

	/* extendfs() parameter under s_state & FM_EXTENDFS */
	int64	s_xsize;	/* 8: extendfs s_size */
	pxd_t	s_xfsckpxd;	/* 8: extendfs fsckpxd */
	pxd_t	s_xlogpxd;	/* 8: extendfs logpxd */
				/* - 128 byte boundary - */

	/*
	 *	DFS VFS support (preliminary) 
	 */
	char	s_attach;	/* 1: VFS: flag: set when aggregate is attached
				 */
	uint8	rsrvd4[7];	/* 7: reserved - set to 0 */

	uint64	totalUsable;	/* 8: VFS: total of 1K blocks which are
				 * available to "normal" (non-root) users.
				 */
	uint64	minFree;	/* 8: VFS: # of 1K blocks held in reserve for 
				 * exclusive use of root.  This value can be 0,
				 * and if it is then totalUsable will be equal 
				 * to # of blocks in aggregate.  I believe this
				 * means that minFree + totalUsable = # blocks.
				 * In that case, we don't need to store both 
				 * totalUsable and minFree since we can compute
				 * one from the other.  I would guess minFree 
				 * would be the one we should store, and 
				 * totalUsable would be the one we should 
				 * compute.  (Just a guess...)
				 */

	uint64	realFree;	/* 8: VFS: # of free 1K blocks can be used by 
				 * "normal" users.  It may be this is something
				 * we should compute when asked for instead of 
				 * storing in the superblock.  I don't know how
				 * often this information is needed.
				 */
	/*
	 *	graffiti area
	 */
};

/*
 *      on-disk inode (dinode_t): 512 bytes
 *
 * note: align 64-bit fields on 8-byte boundary.
 */
struct CJfsDiskInode
{
        /*
         *      I. base area (128 bytes)
         *      ------------------------
         *
         * define generic/POSIX attributes
         */
        uint32  di_inostamp;    /* 4: stamp to show inode belongs to fileset */
        int32   di_fileset;     /* 4: fileset number */
        uint32  di_number;      /* 4: inode number, aka file serial number */
        uint32  di_gen;         /* 4: inode generation number */

        pxd_t   di_ixpxd;       /* 8: inode extent descriptor */

        int64   di_size;        /* 8: size */
        int64   di_nblocks;     /* 8: number of blocks allocated */

        uint32   di_nlink;      /* 4: number of links to the object */    /* D233784 */

        uint32  di_uid;         /* 4: user id of owner */
        uint32  di_gid;         /* 4: group id of owner */

        uint32  di_mode;        /* 4: attribute, format and permission */

        QWORD qwATime; //struct timestruc_t  di_atime;   /* 8: time last data accessed */
        QWORD qwCTime; //struct timestruc_t  di_ctime;   /* 8: time last status changed */
        QWORD qwMTime; //struct timestruc_t  di_mtime;   /* 8: time last data modified */
        QWORD qwOTime; //struct timestruc_t  di_otime;   /* 8: time created */

	u8 di_acl[16]; // dxd_t di_acl;		/* 16: acl descriptor */

	u8 di_ea[16]; // dxd_t di_ea;		/* 16: ea descriptor */

	s32 di_compress;	/* 4: compression */

	s32 di_acltype;		/* 4: Type of ACL */


        /*
         *      II. extension area (128 bytes)
         *      ------------------------------
         */
        /*
         *      extended attributes for file system (96);
         */
        union {
                uint8   _data[96];

                /*
                 *      DFS VFS+ support (preliminary place holder)
                 */
                struct {
                        uint8   _data[96];
                } _dfs;

                /*
                 *      block allocation map
                 */
                struct {
                        struct bmap *__bmap;    /* 4: incore bmap descriptor */
                } _bmap;

                /*
                 *      inode allocation map (fileset inode 1st half)
                 */
                struct {
                        struct inomap *__imap;  /* 4: incore imap control */
                        uint32  _gengen;        /* 4: di_gen generator */
                } _imap;
        } _data2;


        /*
         *      B+-tree root header (32)
         *
         * B+-tree root node header, or
         * data extent descriptor for inline data;
         * N.B. must be on 8-byte boundary.
         */
#define di_btroot di_DASD
        BYTE di_DASD[16]; //dasd_t  di_DASD;        // 16: DASD limit info for directories  F226941
        BYTE di_dxd[16]; //dxd_t   di_dxd;         /* 16: data extent descriptor */

        /*
         *      III. type-dependent area (128 bytes)
         *      ------------------------------------
         *
         * B+-tree root node xad array or inline data
         */
        union {
                uint8   _data[128];

                /*
                 *      regular file or directory
                 *
                 * B+-tree root node/inline data area
                 */
                struct {
		  xad_t xad[8];
                        //uint8   _xad[128];
                } _file;

                /*
                 *      device special file
                 */
                struct {
                        dev_t   _rdev;       /* device major and minor */
                } _specfile;

                /*
                 *      symbolic link.
                 *
                 * link is stored in inode if its length is less than
                 * D_PRIVATE. Otherwise stored like a regular file.
                 */
                struct {
                        uint8   _fastsymlink[128];
                } _symlink;
        } _data3;

        /*
         *      IV. type-dependent extension area (128 bytes)
         *      -----------------------------------------
         *
         *      user-defined attribute, or
         *      inline data continuation, or
         *      B+-tree root node continuation
         *
         */
        union {
                uint8   _data[128];
        } _data4;


}; // CJfsDiskInode

// ================================================
struct CInfoJfsHeader // size must be 16384 (adjust the reserved data)
{
  QWORD qwOfficialBlocksCount;
  QWORD qwMappedBlocksByBitmap;
  DWORD dwAllocTreeMaxLevel;
  //QWORD qwOfficialBitmapSize;

  DWORD dwReserved;
  BYTE cReserved[16360]; // Adjust to fit with total header size
};

// ================================================
class CJfsPart: public CFSBase
{
public:
  CJfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CJfsPart();
  
  //static  int detect(BYTE *cBootSect, char *szFileSystem); 
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}
  int ujfs_rwdaddr(s64 *offset, xtpage_t *page, s64 lbno);
  int find_inode(u32 inum, QWORD *qwAddress, DWORD dwType);
  int find_iag(u32 iagnum, s64 *address, DWORD dwType);
  int browseBitmap(QWORD *qwPage, xtpage_t *page, DWORD dwLevel, QWORD qwBlocksToProcess);
  int readDmap(QWORD *qwPage, xtpage_t *page, QWORD qwBlocksToProcess);

  // for debug only:
  void printFileBlocks(DWORD dwInode); 
  void printInodeWhoseOwnerIs(DWORD dwOwner);
  
 private:
  CInfoJfsHeader m_info;
};

#endif // FS_JFS_H
