/***************************************************************************
                        partimage.h  -  description
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

#ifndef PARTIMAGE_H
#define PARTIMAGE_H

#include <errno.h>
#include <libintl.h> // intl translation
#include <stdio.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "pathnames.h"

// the image format created by the current version
#define CURRENT_IMAGE_FORMAT "0.6.1"

#define BEGIN {showDebug(9, "begin\n");}
#define RETURN_bool(x) {showDebug(9, "end (%d)\n",(bool)x); return(x);}
#define RETURN_int(x) {showDebug(9, "end (%d)\n",(int)x); return(x);}
#define RETURN_WORD(x) {showDebug(9, "end (%u)\n",(WORD)x); return(x);}
#define RETURN_DWORD(x) {showDebug(9, "end (%lu)\n",(DWORD)x); return(x);}
#define RETURN_QWORD(x) {showDebug(9, "end (%llu)\n",(QWORD)x); return(x);}
#define RETURN {showDebug(9, "end\n"); return;}

#define showDebug(level, format, args...) showDebugMsg(g_fDebug, level, __FILE__, __FUNCTION__, __LINE__, format, ## args)
#define THROW(nError, args...) showDebug(1, "THROW: %d\n", nError),throw new CExceptions(__FUNCTION__, nError, ## args)
#define SNPRINTF(szDest, szFormat, args...) snprintf(szDest, sizeof(szDest)-1, szFormat, ## args),szDest[sizeof(szDest)-1]=0 // allow to use this macro in 'else'

#define COPYBUF_SIZE 262144 // 256 KB
#define MAX_FREE_BLOCKS_SKIPPED 2048

// time
extern time_t g_timeBegin; // Beginning of main()
extern time_t g_timeStart; // Beginning of the copy, after confirmation
extern time_t g_timeEnd;   // End of the operation

typedef bool BOOL; // variant size
typedef unsigned char BYTE; // 8 bits
typedef unsigned short int WORD; // 16 bits
typedef unsigned long int DWORD; // 32 bits
typedef unsigned long long int QWORD; // 64 bits

#include "endianess.h"

extern QWORD g_qwCopiedBytesCount;

// debug
extern FILE *g_fDebug;
extern FILE *g_fLocalDebug;
extern char g_szDebug[];

extern DWORD g_dwCrcTable[];
class CInterface;
extern CInterface *g_interface;
extern bool g_bSigInt;
extern bool g_bSigKill;

#ifdef OS_LINUX
  #define i18n(X)	gettext(X)
#else
  #define i18n(X) (X)
#endif

#define my_min(a,b) ((a) < (b) ? (a) : (b))
#define my_max(a,b) ((a) > (b) ? (a) : (b))

/*
#define min(a,b) ((a) < (b) ? (a) : (b))
#define min3(a,b,c) (min(min((a),(b)),(c)))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define max3(a,b,c) (max(max((a),(b)),(c)))
*/

#define OPERATION_SAVE       1
#define OPERATION_RESTORE    2
#define OPERATION_RESTMBR    3
#define OPERATION_IMGINFO    4
#define OPERATION_EXIT       5

// returned by CExceptionsGUI::windowError 
// user canceled the job
#define ERR_QUIT 1
// user entered a new path or filename
#define ERR_RETRY 2
// user want to go on without any change
#define ERR_CONT 3

#define FS_UNKNOWN     0
#define FS_EXT2        1
#define FS_EXT3        2
#define FS_FAT12       3
#define FS_FAT16       4
#define FS_FAT32       5
#define FS_NTFS        6
#define FS_REISER_3_5  7 // kernel-2.2 version
#define FS_REISER_3_6  8 // kernel-2.4 versions
#define FS_HPFS        9
#define FS_EXTENDED    10
#define FS_SWAP0       11
#define FS_SWAP1       12
#define FS_UFS         13 // Old Unix File System (FFS: Berkeley BSD Fast File System,  Solaris FS, ...)
//#define FS_FFS         14 // Berkeley BSD Fast File System
#define FS_JFS         15 // IBM Journalized File System (AIX)
#define FS_XFS         16 // XFS (SGI)
#define FS_HFS         17 // HFS (MacOS)
#define FS_HFSPLUS     18 // HFS+ (MacOS 8.1 and higher)
#define FS_AFS         19 // AFS (AtheOS file system)
#define FS_BEFS        20 // BeFS (BeOS file system)

#define MAX_DESCRIPTION         4096
#define MAX_HOSTNAMESIZE	128
#define MAX_DEVICENAMELEN       512

#define MAX_USERNAMELEN		32
#define MAX_PASSWORDLEN		32

// COMPRESSION
#define COMPRESS_NONE    0
#define COMPRESS_GZIP    1
#define COMPRESS_BZIP2   2	
#define COMPRESS_LZO     3

// ENCRYPTION
#define ENCRYPT_NONE     0

// SPLIT MODE
#define SPLIT_AUTO	 1
#define SPLIT_SIZE       2

// FINISH MODE
#define FINISH_WAIT      0
#define FINISH_HALT      1
#define FINISH_REBOOT    2
#define FINISH_QUIT      3
#define FINISH_LAST      4

#define FINISH_LAST_COUNTFILE "/tmp/partimage.count"
#define FINISH_LAST_COUNTFILE_LOCK "/tmp/partimage.count.lock"

// ===================== MAGIC STRINGS  ============================
#define MAGIC_BEGIN_LOCALHEADER	              "MAGIC-BEGIN-LOCALHEADER"
#define MAGIC_BEGIN_DATABLOCKS                "MAGIC-BEGIN-DATABLOCKS"
#define MAGIC_BEGIN_BITMAP                    "MAGIC-BEGIN-BITMAP"
#define MAGIC_BEGIN_MBRBACKUP                 "MAGIC-BEGIN-MBRBACKUP"
#define MAGIC_BEGIN_TAIL                      "MAGIC-BEGIN-TAIL"
#define MAGIC_BEGIN_INFO                      "MAGIC-BEGIN-INFO"
#define MAGIC_BEGIN_EXT000                    "MAGIC-BEGIN-EXT000" // reserved for future use
#define MAGIC_BEGIN_EXT001                    "MAGIC-BEGIN-EXT001" // reserved for future use
#define MAGIC_BEGIN_EXT002                    "MAGIC-BEGIN-EXT002" // reserved for future use
#define MAGIC_BEGIN_EXT003                    "MAGIC-BEGIN-EXT003" // reserved for future use
#define MAGIC_BEGIN_EXT004                    "MAGIC-BEGIN-EXT004" // reserved for future use
#define MAGIC_BEGIN_EXT005                    "MAGIC-BEGIN-EXT005" // reserved for future use
#define MAGIC_BEGIN_EXT006                    "MAGIC-BEGIN-EXT006" // reserved for future use
#define MAGIC_BEGIN_EXT007                    "MAGIC-BEGIN-EXT007" // reserved for future use
#define MAGIC_BEGIN_EXT008                    "MAGIC-BEGIN-EXT008" // reserved for future use
#define MAGIC_BEGIN_EXT009                    "MAGIC-BEGIN-EXT009" // reserved for future use
#define MAGIC_BEGIN_VOLUME		      "PaRtImAgE-VoLuMe"

// ==================== DEFAULT OPTIONS ============================
#define OPTIONS_DEFAULT_COMPRESS              COMPRESS_GZIP
#define OPTIONS_DEFAULT_ERASE_EMPTY           false
#define OPTIONS_DEFAULT_SPLIT_SIZE            2136746229 // 1.99 GB OLD: [0 // do not split]
#define OPTIONS_DEFAULT_OVERWRITE             false
#define OPTIONS_DEFAULT_CHECK                 true
#define OPTIONS_DEFAULT_FAIL_IF_MOUNTED       true
#define OPTIONS_DEFAULT_ASK_DESC	      true
#define OPTIONS_DEFAULT_FINISH	      	      FINISH_WAIT
#define OPTIONS_DEFAULT_SERVERPORT	      4025
#define OPTIONS_DEFAULT_AUTOSTART             false
#define OPTIONS_DEFAULT_SPLIT_WAIT            false
#define OPTIONS_DEFAULT_RUN_SHELL             false
#define OPTIONS_DEFAULT_SYNC                  true
#define OPTIONS_DEFAULT_SSL                   true
#define OPTIONS_DEFAULT_BACKUP_MBR            true
#define OPTIONS_DEFAULT_SIMULATE_MODE         false
#define OPTIONS_DEFAULT_SERVERNAME	      ""
#define OPTIONS_DEFAULT_AUTOMOUNT             ""

#define OPTIONS_DEFAULT_USERNAME			  ""
#define OPTIONS_DEFAULT_PASSWORD			  ""

// ======================== FUNCTIONS ================================
void usage();
int checkStructSizes();
void closeDebugFiles();
void formatOperationSucessMsg(char *szMsg, int nMaxLen, char *szOperation);

#endif // PARTIMAGE_H
