
/***************************************************************************
                          common.h  -  description
                             -------------------
    begin                : Sun Jul 16 2000
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

#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <pthread.h>

#include <sys/param.h>

#include "partimage.h"

// debug
extern DWORD g_dwDebugLevel;
extern int g_nDebugThreadMain;
extern int g_nDebugThreadBufferW;
extern int g_nDebugThreadBufferR;
extern pthread_mutex_t g_mutexDebug; // lock the buffer while working inside it
extern pthread_mutexattr_t g_mutexDebugAttr;
extern char *g_szSupportedFileSystems; 

void formatCompilOptions(char *szDest, int nMaxLen);
FILE *openFileDescriptorSecure(char *szFilename, char *szMode, int nFlags, mode_t mode);
char *formatGzipError(int nError, char *szTemp, int nMaxLen);
int extractFilenameFromFullPath(char *szFullpath, char *szFilename);
int extractFilepathFromFullPath(char *szFullpath, char *szFilepath);
QWORD generateIdentificator();

/** Show formated advanced debug informations in the log file (/var/log/partimage-debug.log in most cases)
 *  The debug level allow to tell if the message must be written in a normal use, or only in a debug session
 *  This function must not be used directly: use showDebug() instead.
 *  nDebug Level:
 *  + 0 -> no message in the log file
 *  + 1 -> important messages in the log file (default) good for users
 *  + 2 -> many messages in the log file: good for developers
 *  + 3 -> all messages in the log file: good for debugging
*/
void showDebugMsg(FILE *fDebug, DWORD dwDebugLevel, const char *szFile, const char *szFunction, int nLine, const char *fmt, ...);

//bool doesFileExists(char *szPath);

/** Returns the formatted size in the appropriated unit. Examples:
 *  193 --> "193 bytes"
 *  9185 --> "8.97 KB"
 *  58955455 --> "56.22 MB" 
 *  @param qwSize numerical size
 *  @param szText buffer where to write the result string
 *  @return pointer to the result string (allows to use this function in an expression)
 */
char *formatSize2(QWORD qwSize, char *szText, int nMaxLen);
#define formatSize(qwSize, szBuf) formatSize2(qwSize, szBuf, sizeof(szBuf)) 
char *formatSizeNoGui(QWORD qwSize, char *szText, int nMaxLen);
#define formatSizeNG(qwSize, szBuf) formatSizeNoGui(qwSize, szBuf, sizeof(szBuf))


/** Returns the formatted time in a string. This function receives a number of seconds, and
 *  format this in hour, minutes, seconds into a char buffer.
 *  Examples:
 *  70 --> "1min:10sec"
 *  3700 --> "1h:1min:30sec"
 *  @param qwSec number of seconds
 *  @param szBuf buffer where to write the result string
 *  @return pointer to the result string (allows to use this function in an expression)
 */
char *formatTime2(DWORD dwSec, char *szBuf, int nMaxLen);
#define formatTime(dwSec, szBuf) formatTime2(dwSec, szBuf, sizeof(szBuf)) 
char *formatTimeNoGui(DWORD dwSec, char *szBuf, int nMaxLen);
#define formatTimeNG(dwSec, szBuf) formatTimeNoGui(dwSec, szBuf, sizeof(szBuf))

/** returns the free disk space available to write a file. For example, if the
    file is "/home/user/file.txt", it returns the free space on the /home partition.
 *  @param qwAvailDiskSpace adress of the 64 bits number of the free space in bytes
 *  @param szFilename path of the file to look to the free space for
 *  @return @p 0 if success, @p -1 if error
*/
//int getDiskFreeSpaceForFile2(QWORD *qwAvailDiskSpace, const char *szFilename);

/** returns the compression type used for an image file. For example, if the image
    was created in the gzip compression mode, and it was uncompressed, then it will
    returns COMPRESS_NONE. It doesn't use the file name to detect the compression level.
 *  @param szFilename path of the file to look to the compression level for
 *  @return @p COMPRESS_NONE or @p COMPRESS_GZIP or @p COMPRESS_BZIP2
*/
//int getCompressionLevelForImageCommon(char *szFilename);

// ===================================================================
struct CVolumeHeader // size must be 512 (adjust the reserved data)
{
  char szMagicString[32];
  char szVersion[64]; // version of the image file
  DWORD dwVolumeNumber;
  QWORD qwIdentificator; // to avoid using volumes from differents image files on restore
  
  unsigned char cReserved[404]; // Adjust to fit with total header size
} __attribute__ ((packed));

// ===================================================================
struct CMainTail // size must be 16384 (adjust the reserved data)
{
  QWORD qwCRC;
  DWORD dwVolumeNumber;
  
  unsigned char cReserved[16372]; // Adjust to fit with total header size
};

// ===================================================================
struct COptions
{
  bool bOverwrite;
  //DWORD dwSplitMode;
  bool bSplitWait; // Wait after each volume change
  bool bRunShell;  // run a shell command on each volume change
  bool bEraseWithNull;
  bool bCheckBeforeSaving;
  bool bFailIfMounted; // Fail if file system is mounted
  bool bAskDesc;
  bool bBatchMode;
  bool bSync;
  bool bUseSSL; 
  bool bBackupMBR;
  bool bSimulateMode;
  DWORD dwFinish;
  DWORD dwServerPort; // ip port
  DWORD dwDebugLevel;
  DWORD dwCompression;
  QWORD qwSplitSize; // 0L = auto, !0 = bytes size
  char szServerName[MAX_HOSTNAMESIZE]; // partimaged server's name
  char szAutoMount[MAXPATHLEN];
  char szUserName[MAX_USERNAMELEN];
  char szPassWord[MAX_PASSWORDLEN];
  char szFullyBatchMode[2048];
};

#define MAX_UNAMEINFOLEN 65 //SYS_NMLN

// ================================================
struct CMainHeader // size must be 16384 (adjust the reserved data)
{
  char szFileSystem[512]; // ext2fs, ntfs, reiserfs, ...
  char szPartDescription[MAX_DESCRIPTION]; // user description of the partition
  char szOriginalDevice[MAX_DEVICENAMELEN]; // original partition name
  char szFirstImageFilepath[4095]; //MAXPATHLEN]; // for splitted image files
  
  // system and hardware infos
  char szUnameSysname[MAX_UNAMEINFOLEN];
  char szUnameNodename[MAX_UNAMEINFOLEN];
  char szUnameRelease[MAX_UNAMEINFOLEN];
  char szUnameVersion[MAX_UNAMEINFOLEN];
  char szUnameMachine[MAX_UNAMEINFOLEN];

  DWORD dwCompression; // COMPRESS_XXXXXX
  DWORD dwMainFlags;
  struct tm dateCreate; // date of image creation
  QWORD qwPartSize; // size of the partition in bytes
  char szHostname[MAX_HOSTNAMESIZE];
  char szVersion[64]; // version of the image file

  // MBR backup
  DWORD dwMbrCount; // how many MBR are saved in the image file
  DWORD dwMbrSize; // size of a MBR record (allow to change the size in the next versions)

  // future encryption support
  DWORD dwEncryptAlgo; // algo used to encrypt data
  char cHashTestKey[16]; // used to test the password without giving it
  
  // reserved for future use (save DiskLabel, Extended partitions, ...)
  DWORD dwReservedFuture000;
  DWORD dwReservedFuture001;
  DWORD dwReservedFuture002;
  DWORD dwReservedFuture003;
  DWORD dwReservedFuture004;
  DWORD dwReservedFuture005;
  DWORD dwReservedFuture006;
  DWORD dwReservedFuture007;
  DWORD dwReservedFuture008;
  DWORD dwReservedFuture009;

  BYTE cReserved[6524]; // Adjust to fit with total header size
} __attribute__ ((packed));
#endif //COMMON_H
