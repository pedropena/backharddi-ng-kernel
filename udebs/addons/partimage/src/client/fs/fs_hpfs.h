/***************************************************************************
                          chpfspart.h  -  description
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

#ifndef FS_HPFS_H
#define FS_HPFS_H

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

// format version of the fat image
#define CURRENT_HPFS_FORMAT "0.1"

/* Notation */

typedef unsigned secno;			/* sector number, partition relative */

typedef secno dnode_secno;		/* sector number of a dnode */
typedef secno fnode_secno;		/* sector number of an fnode */
typedef secno anode_secno;		/* sector number of an anode */

/* sector 0 */

/* The boot block is very like a FAT boot block, except that the
   29h signature byte is 28h instead, and the ID string is "HPFS". */

#define BB_MAGIC 0xaa55

struct hpfs_boot_block
{
  unsigned char jmp[3];
  unsigned char oem_id[8];
  unsigned char bytes_per_sector[2];	/* 512 */
  unsigned char sectors_per_cluster;
  unsigned char n_reserved_sectors[2];
  unsigned char n_fats;
  unsigned char n_rootdir_entries[2];
  unsigned char n_sectors_s[2];
  unsigned char media_byte;
  unsigned short sectors_per_fat;
  unsigned short sectors_per_track;
  unsigned short heads_per_cyl;
  unsigned int n_hidden_sectors;
  unsigned int n_sectors_l;		/* size of partition */
  unsigned char drive_number;
  unsigned char mbz;
  unsigned char sig_28h;		/* 28h */
  unsigned char vol_serno[4];
  unsigned char vol_label[11];
  unsigned char sig_hpfs[8];		/* "HPFS    " */
  unsigned char pad[448];
  unsigned short magic;			/* aa55 */
};


/* sector 16 */

/* The super block has the pointer to the root directory. */

#define SB_MAGIC 0xf995e849

struct hpfs_super_block
{
  unsigned long int magic;			/* f995 e849 */
  unsigned long int magic1;			/* fa53 e9c5, more magic? */
  //unsigned huh202;			/* ?? 202 = N. of B. in 1.00390625 S.*/
  char version;				/* version of a filesystem  usually 2 */
  char funcversion;			/* functional version - oldest version
  					   of filesystem that can understand
					   this disk */
  unsigned short int zero;		/* 0 */
  fnode_secno root;			/* fnode of root directory */
  secno n_sectors;			/* size of filesystem */
  unsigned n_badblocks;			/* number of bad blocks */
  secno bitmaps;			/* pointers to free space bit maps */
  unsigned zero1;			/* 0 */
  secno badblocks;			/* bad block list */
  unsigned zero3;			/* 0 */
  time_t last_chkdsk;			/* date last checked, 0 if never */
  /*unsigned zero4;*/			/* 0 */
  time_t last_optimize;			/* date last optimized, 0 if never */
  secno n_dir_band;			/* number of sectors in dir band */
  secno dir_band_start;			/* first sector in dir band */
  secno dir_band_end;			/* last sector in dir band */
  secno dir_band_bitmap;		/* free space map, 1 dnode per bit */
  char volume_name[32];			/* not used */
  secno user_id_table;			/* 8 preallocated sectors - user id */
  unsigned zero6[103];			/* 0 */
};

/* sector 17 */

/* The spare block has pointers to spare sectors.  */

#define SP_MAGIC 0xf9911849

struct hpfs_spare_block
{
  unsigned magic;			/* f991 1849 */
  unsigned magic1;			/* fa52 29c5, more magic? */

  unsigned dirty: 1;			/* 0 clean, 1 "improperly stopped" */
  /*unsigned flag1234: 4;*/		/* unknown flags */
  unsigned sparedir_used: 1;		/* spare dirblks used */
  unsigned hotfixes_used: 1;		/* hotfixes used */
  unsigned bad_sector: 1;		/* bad sector, corrupted disk (???) */
  unsigned bad_bitmap: 1;		/* bad bitmap */
  unsigned fast: 1;			/* partition was fast formatted */
  unsigned old_wrote: 1;		/* old version wrote to partion */
  unsigned old_wrote_1: 1;		/* old version wrote to partion (?) */
  unsigned install_dasd_limits: 1;	/* HPFS386 flags */
  unsigned resynch_dasd_limits: 1;
  unsigned dasd_limits_operational: 1;
  unsigned multimedia_active: 1;
  unsigned dce_acls_active: 1;
  unsigned dasd_limits_dirty: 1;
  unsigned flag67: 2;
  unsigned char mm_contlgulty;
  unsigned char unused;

  secno hotfix_map;			/* info about remapped bad sectors */
  unsigned n_spares_used;		/* number of hotfixes */
  unsigned n_spares;			/* number of spares in hotfix map */
  unsigned n_dnode_spares_free;		/* spare dnodes unused */
  unsigned n_dnode_spares;		/* length of spare_dnodes[] list,
					   follows in this block*/
  secno code_page_dir;			/* code page directory block */
  unsigned n_code_pages;		/* number of code pages */
  /*unsigned large_numbers[2];*/	/* ?? */
  unsigned super_crc;			/* on HPFS386 and LAN Server this is
  					   checksum of superblock, on normal
					   OS/2 unused */
  unsigned spare_crc;			/* on HPFS386 checksum of spareblock */
  unsigned zero1[15];			/* unused */
  dnode_secno spare_dnodes[100];	/* emergency free dnode list */
  unsigned zero2[1];			/* room for more? */
};
// ================================================
struct CInfoHpfsHeader // size must be 16384 (adjust the reserved data)
{
  DWORD dwBitmapPointer;
  DWORD dwBitmapQuadBlocksCount;
  char cHpfsVersion;

  BYTE cReserved1[7];
  BYTE cReserved[16368]; // Adjust to fit with total header size
};

// ================================================
class CHpfsPart : public CFSBase
{
 public:
  CHpfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  ~CHpfsPart();
  
  static  int detect(BYTE *cBootSect, char *szFileSystem); 
  virtual void printfInformations();
  virtual void readBitmap(COptions *options);
  virtual void readSuperBlock();
  virtual void fsck();
  virtual void* getInfos() {return (void*)&m_info;}

 private:
  int getBitmapQuadBlocksCount(DWORD *dwBitmapQuadBlocksCount);
  
 private:
  CInfoHpfsHeader m_info;
};

#endif // FS_HPFS_H
