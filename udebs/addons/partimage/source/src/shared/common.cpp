/***************************************************************************
                          common.cpp  -  procedures shared between client&server
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
  #include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
  #include <sys/statfs.h>
#endif

#include <zlib.h> // gzip compression
#include <bzlib.h> // bzip2 compression
#include <string.h>

#include "common.h"
#include "partimage.h"
#include "pathnames.h"

DWORD g_dwDebugLevel;
int g_nDebugThreadMain;
int g_nDebugThreadBufferR;
int g_nDebugThreadBufferW;
pthread_mutex_t g_mutexDebug; // lock the buffer while working inside it
pthread_mutexattr_t g_mutexDebugAttr;

char *g_szSupportedFileSystems = "ext2fs,ext3fs,reiserfs-3.5,reiserfs-3.6,reiserfs,fat,fat16,fat32,ntfs,hpfs,xfs,jfs,hfs,ufs";

// =======================================================
#define addText(format, args...) \
{ \
  nLenRest = nMaxLen - strlen(szDest); \
  snprintf(szTemp, sizeof(szTemp), format, ##args); \
  strncat(szDest, szTemp, nLenRest); \
}

void formatCompilOptions(char *szDest, int nMaxLen)
{
  int nLenRest;
  char szTemp[8192];

  memset(szDest, 0, nMaxLen);

  addText(i18n("==== Partition Image: compilation options used ====\n"));
  addText(i18n("\n* Version is %s [%s].\n"), PACKAGE_VERSION, __VERSION__);

  addText(i18n("\n* Supported file systems:\n  - "));
  addText(g_szSupportedFileSystems);
  addText(i18n("\n"));

#ifdef LIBRARY_EXT2FS
  addText(i18n("  - Ext2fs support: based on libext2fs library\n"));
#else
  addText(i18n("  - Ext2fs support: internal code\n"));
#endif

  addText(i18n("\n* Debug options:\n"));

#ifdef PARTIMAGE_LOG
  addText(i18n("  - PARTIMAGE_LOG=%s\n"), PARTIMAGE_LOG);
#endif
#ifdef PARTIMAGED_LOG
  addText(i18n("  - PARTIMAGED_LOG=%s\n"), PARTIMAGED_LOG);
#endif

#ifdef DEVEL_SUPPORT
  addText(i18n("  - DEVEL ENABLED\n"));
#else
  addText(i18n("  - DEVEL DISABLED\n"));
#endif

  addText(i18n("  - DEFAULT_DEBUG_LEVEL=%d\n"), DEFAULT_DEBUG_LEVEL);
#ifdef APPEND_PID
  addText(i18n("  - APPEND_PID=%s\n"), (APPEND_PID ? i18n("true") : 
     i18n("false")));
#endif

  addText(i18n("\n* Other options:\n"));
  addText(i18n("  - CURRENT_IMAGE_FORMAT=%s\n"), CURRENT_IMAGE_FORMAT);

#ifdef MUST_LOGIN
  addText(i18n("  - USERS MUST LOGIN\n"));
  addText(i18n("  - PARTIMAGED_USERS=%s\n"), PARTIMAGED_USERS);
#else
  addText(i18n("  - USERS DON'T LOGIN\n"));
  addText(i18n("  - PARTIMAGED_USERS not used\n"));
#endif
#ifdef HAVE_SSL
  addText(i18n("  - SSL ENABLED\n"));
#else
  addText(i18n("  - SSL DISABLED\n"));
#endif
#ifdef MUST_CHEUID
  addText(i18n("  - CHEUID ENABLED\n"));
#else
  addText(i18n("  - CHEUID DISABLED\n"));
#endif
}

// =======================================================
char *formatGzipError(int nError, char *szTemp, int nMaxLen)
{
  switch (nError)
    {
    case Z_OK:
      snprintf(szTemp, nMaxLen, "GZIP nError = %d: Z_OK\n", nError);
      break;
	  
    case Z_STREAM_END:
      snprintf(szTemp, nMaxLen, "GZIP = %d: Z_STREAM_END\n", nError);
      break;

    case Z_ERRNO:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d: Z_ERRNO and errno=%d=[%s]\n", nError, errno, strerror(errno));
      break;	  
	  
    case Z_NEED_DICT:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d: Z_NEED_DICT\n", nError);
      break;

    case Z_STREAM_ERROR:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d: Z_STREAM_ERROR\n", nError);
      break;

    case Z_DATA_ERROR:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d: Z_DATA_ERROR\n", nError);
      break;

    case Z_MEM_ERROR:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d: Z_MEM_ERROR\n", nError);
      break;

    case Z_BUF_ERROR:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d: Z_BUF_ERROR\n", nError);
      break;
	  
    default:
      snprintf(szTemp, nMaxLen, "GZIP: nError = %d\n", nError);
      break;
    };

  return szTemp;
}

// =======================================================
void showDebugMsg(FILE *fDebug, DWORD dwDebugLevel, const char *szFile,
		  const char *szFunction, int nLine, const char *fmt, ...)
{
  if (dwDebugLevel == 0)
    return;

  pthread_mutex_lock(&g_mutexDebug);

  va_list args;
  DWORD i;

  if (dwDebugLevel <= g_dwDebugLevel)
  {
    // add tabs to distinguish log levels
    for (i = 1; i < dwDebugLevel; i++)
      fprintf(fDebug, "\t");
    
    va_start(args, fmt);
    
    if (getpid() == g_nDebugThreadMain) 
      fprintf(fDebug, "[Main] %s->%s#%d: ", szFile, szFunction, nLine);
    else if (getpid() == g_nDebugThreadBufferR) 
      fprintf(fDebug, "[BufferR] %s->%s#%d: ", szFile, szFunction, nLine);
    else if (getpid() == g_nDebugThreadBufferW) 
      fprintf(fDebug, "[BufferW] %s->%s#%d: ", szFile, szFunction, nLine);
    else
      fprintf(fDebug, "[TH=%d] %s->%s#%d: ", getpid(), szFile, szFunction, nLine);
    
    vfprintf(fDebug, fmt, args);

    // do not slow down the progran if standard debug level
    // must be enabled: if no flush, the log won't be up-to-date if there
    // is a segfault
    //if (g_dwDebugLevel != 1)
      fflush(fDebug);
    
    va_end(args);
  }
  pthread_mutex_unlock(&g_mutexDebug);
}

// =======================================================
char *formatTime2(DWORD dwSec, char *szBuf, int nMaxLen)
{
  int nHour, nMin, nSec;

  nSec = dwSec % 60;
  dwSec -= nSec; dwSec /= 60; // now, dwSec = dwMinutes
  nMin = dwSec % 60;
  dwSec -= nMin; dwSec /= 60; // now, dwSec = dwHours
  nHour = dwSec;
  //nHour = (dwSec - (dwSec % 3600)) / 60;

  *szBuf = 0;
  if (nHour)
    snprintf(szBuf, nMaxLen, "%dh:", nHour);
  if (nMin || nHour)
    snprintf(szBuf+strlen(szBuf), nMaxLen-strlen(szBuf), "%dm:", nMin);
  snprintf(szBuf+strlen(szBuf), nMaxLen-strlen(szBuf), "%dsec", nSec);
  
  return szBuf;
}

// =======================================================
char *formatTimeNoGui(DWORD dwSec, char *szBuf, int nMaxLen)
{
  int nHour, nMin, nSec;

  nSec = dwSec % 60;
  dwSec -= nSec; dwSec /= 60; // now, dwSec = dwMinutes
  nMin = dwSec % 60;
  dwSec -= nMin; dwSec /= 60; // now, dwSec = dwHours
  nHour = dwSec;
  //nHour = (dwSec - (dwSec % 3600)) / 60;

  *szBuf = 0;
  snprintf(szBuf, nMaxLen, "%02d:%02d:%02d", nHour, nMin, nSec);
  return szBuf;
}

// =======================================================
char *formatSize2(QWORD qwSize, char *szText, int nMaxLen)
{	
  //showDebug(1,"SALUT\n");

  double dSize;
  
  QWORD llKiloB = ((QWORD) 1024);
  QWORD llMegaB = ((QWORD) 1024)*((QWORD) 1024);
  QWORD llGigaB = ((QWORD) 1024)*((QWORD) 1024)*((QWORD) 1024);
  QWORD llTeraB = ((QWORD) 1024)*((QWORD) 1024)*((QWORD) 1024)*((QWORD) 1024);
  
  if (qwSize < llKiloB) // In Bytes
    {
      snprintf(szText, nMaxLen, i18n("%lld bytes"), qwSize);
    }
  else if (qwSize < llMegaB) // In KiloBytes
    {
      dSize = ((double) qwSize) / ((double) 1024.0);
      snprintf(szText, nMaxLen, i18n("%.2f KiB"), (float) dSize);
    }
  else if (qwSize < llGigaB) // In MegaBytes
    {
      dSize = ((double) qwSize) / ((double) (1024.0 * 1024.0));
      snprintf(szText, nMaxLen, i18n("%.2f MiB"), (float) dSize);
    }
  else if (qwSize < llTeraB)// In GigaBytes
    {
      dSize = ((double) qwSize) / ((double) (1024.0 * 1024.0 * 1024.0));
      snprintf(szText, nMaxLen, i18n("%.2f GiB"), (float) dSize);
    }
  else // In TeraBytes
    {
      dSize = ((double) qwSize)/((double) (1024.0 * 1024.0 * 1024.0 * 1024.0));
      snprintf(szText, nMaxLen, i18n("%.2f TiB"), (float) dSize);
    }
  
  return szText;
}

// =======================================================
char *formatSizeNoGui(QWORD qwSize, char *szText, int nMaxLen)
{
  //showDebug(1,"SALUT\n");

  double dSize;

  QWORD llKiloB = ((QWORD) 1024);
  QWORD llMegaB = ((QWORD) 1024)*((QWORD) 1024);
  QWORD llGigaB = ((QWORD) 1024)*((QWORD) 1024)*((QWORD) 1024);
  QWORD llTeraB = ((QWORD) 1024)*((QWORD) 1024)*((QWORD) 1024)*((QWORD) 1024);

  if (qwSize < llKiloB) // In Bytes
    {
      snprintf(szText, nMaxLen, i18n("%lldb"), qwSize);
    }
  else if (qwSize < llMegaB) // In KiloBytes
    {
      dSize = ((double) qwSize) / ((double) 1024.0);
      snprintf(szText, nMaxLen, i18n("%4.0fK"), (float) dSize);
    }
  else if (qwSize < llGigaB) // In MegaBytes
    {
      dSize = ((double) qwSize) / ((double) (1024.0 * 1024.0));
      snprintf(szText, nMaxLen, i18n("%4.0fM"), (float) dSize);
    }
  else if (qwSize < llTeraB)// In GigaBytes
    {
      dSize = ((double) qwSize) / ((double) (1024.0 * 1024.0 * 1024.0));
      snprintf(szText, nMaxLen, i18n("%1.2fG"), (float) dSize);
    }
  else // In TeraBytes
    {
      dSize = ((double) qwSize)/((double) (1024.0 * 1024.0 * 1024.0 * 1024.0));
      snprintf(szText, nMaxLen, i18n("%1.2fT"), (float) dSize);
    }

  return szText;
}

// =======================================================
// avoid RACE CONDITIONS security problems
FILE *openFileDescriptorSecure(char *szFilename, char *szMode, int nFlags, mode_t mode)
{
  int nFd;
  FILE *fStream;
#ifdef HAVE_FSTAT64
  struct stat64 fStat;
#else
  struct stat fStat;
#endif

  // init
  errno = 0;
#ifdef OS_LINUX
  nFlags |= O_LARGEFILE;
#endif

  // open file descriptor
  nFd = open(szFilename, nFlags, mode);
  if (nFd == -1)
    return NULL;

#ifdef HAVE_FSTAT64
  if (fstat64(nFd, &fStat) == -1)
    return NULL;
#else
  if (fstat(nFd, &fStat) == -1)
    return NULL;
#endif

  if (!S_ISREG(fStat.st_mode)) // not a regular file
    return NULL;

  fStream = fdopen(nFd, szMode);
  if (fStream == NULL)
    return NULL;

  return fStream;
}

// =======================================================
QWORD generateIdentificator()
{
  QWORD qwResult;
  int i;
  unsigned char *ptr;

  ptr = (unsigned char *) &qwResult;
  qwResult = time(NULL);
  
  for (i=0; i < (int)sizeof(qwResult); i++)
    ptr[i] ^= getpid();

  showDebug(3, "generateIdentificator() = %llu = %Xh\n", qwResult, qwResult);
  return qwResult;
}

// =======================================================
int extractFilenameFromFullPath(char *szFullpath, char *szFilename)
{
  int nLen;

  // if no '/' present
  if (strchr(szFullpath, '/') == 0)
    strcpy(szFilename, szFullpath);
  else // if '/' is present
    strcpy(szFilename, strrchr(szFullpath, '/')+1);

  // remove the volume number at the end of the filename
  nLen = strlen(szFilename);
  if (isdigit(szFilename[nLen-3]) && isdigit(szFilename[nLen-2]) && 
      isdigit(szFilename[nLen-1]) && szFilename[nLen-4] == '.') // if there is a volume number
    szFilename[nLen-4] = 0; // truncate volume number

  return 0;
}

// =======================================================
int extractFilepathFromFullPath(char *szFullpath, char *szFilepath)
{
  int i;
  int nLen;
  
  // if no '/' present
  if (strchr(szFullpath, '/') == 0)
    {
      strcpy(szFilepath, ".");
      return 0;	
    }
  
  // if '/' is present
  nLen = strrchr(szFullpath, '/') - szFullpath;
  for (i=0; i < nLen; i++)
    szFilepath[i] = szFullpath[i];
  szFilepath[i] = 0;
  
  return 0;
}

