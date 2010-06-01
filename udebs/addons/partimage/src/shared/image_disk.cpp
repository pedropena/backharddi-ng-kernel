/***************************************************************************
                          image_disk.cpp  -  description
                             -------------------
    begin                : Mon Aug 14 2000
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

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif // _GNU_SOURCE

#ifndef _FILE_OFFSET_BITS
  #define _FILE_OFFSET_BITS 64
#endif //_FILE_OFFSET_BITS

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif
#ifdef HAVE_SYS_STATFS_H
  #include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
  #include <sys/mount.h>
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include "image_disk.h"
#include "errors.h"
#include "common.h"
#include "buffer.h"
#include "pathnames.h"

#define SPACEFILE_SIZE (10*1024*1024) // 10 MB

#ifndef HAVE_FSTAT64
  #warning "fstat32 used"
#endif
#ifndef HAVE_FSTATFS64
  #warning "fstatfs32 used"
#endif
#ifndef HAVE_OPEN64
  #warning "open32 used"
#endif

// =======================================================
CImageDisk::CImageDisk(COptions * options):CImageBase() // [Main-Thread]
{
  m_bIsOpened = false;
  m_qwTotal = 0LL;
  m_qwVolSize = 0LL;
  memcpy(&m_options, options, sizeof(COptions));
  memset(m_szSpaceFilename, 0, sizeof(m_szSpaceFilename));
  showDebug(3, "compress=%ld\n", options->dwCompression);
}

// =======================================================
CImageDisk::~CImageDisk() // [Main-Thread]
{
  showDebug(1, "total written: %lld\n", m_qwTotal);
  unlockImageFile();
}

// =======================================================
QWORD CImageDisk::getPartitionSize(const char *szDevice) // [Main-Thread]
{
  QWORD qwSize;
  long lSize;
  int fdDevice;

  showDebug(1, "begin\n");
  qwSize = 0;
	
#ifdef HAVE_OPEN64
  fdDevice = open64(szDevice, O_RDONLY);
#else 
  fdDevice = open(szDevice, O_RDONLY);
#endif
  if (fdDevice < 0)
    THROW( ERR_ERRNO, errno);

#ifdef OS_LINUX
  if (!ioctl(fdDevice, BLKGETSIZE, &lSize))
    {
      qwSize = ((QWORD) 512) * ((QWORD) lSize);
    }
  else
    THROW( ERR_ERRNO, errno);
#endif

  if (::close(fdDevice))
    THROW( ERR_ERRNO, errno);

  return qwSize;
}

// =======================================================
void CImageDisk::getFileSize(QWORD *qwFileSize, char *szFilepath) // [Main-Thread]
{
#ifdef HAVE_FSTAT64
  struct stat64 fStat;
#else
  struct stat fStat;
#endif
  //struct stat st;
  int nRes;

#ifdef HAVE_FSTAT64
  nRes = stat64(szFilepath, &fStat);
#else
  nRes = stat(szFilepath, &fStat);
#endif
  //nRes = stat(szFilepath, &st);	
  showDebug(1, "getFileSize() -> nRes=%d\n", nRes);

  if (nRes != -1)
    *qwFileSize = (QWORD)fStat.st_size; // ::getFileSize(szFilepath);
  else
    *qwFileSize = (QWORD)0LL; // ::getFileSize(szFilepath);
}

// =======================================================
void CImageDisk::get_qwCurrentVolumeSize(QWORD *qwSize) // [Main-Thread]
{
  //struct stat fStat;

  if (m_bIsOpened == false)
    {
      showDebug(3, "Failed: m_bIsOpened == false\n");
      THROW( ERR_OPENED);
    }

  errno = 0;
  *qwSize = m_qwVolSize;
}

// =======================================================
// may throw incomplete on error
void CImageDisk::write(void *vBuf, DWORD dwLength, int *nResult) // [Buffer-Thread]
{
  DWORD dwRes, dwFirstWritten;
  QWORD qwVolumeSize;
  int nRes;
  char *cBuf;

  showDebug(3, "called for %ld\n", dwLength);
  *nResult = 0;

  if (get_bIsOpened() == false)
    {
      showDebug(3, "file not opened\n");
      *nResult = ERR_OPENED; return; //THROW(ERR_OPENED);
    }
  
  //*dwResult = 0;
  m_qwTotal += (QWORD) dwLength;
  m_qwVolSize += (QWORD) dwLength;
  cBuf = (char*) vBuf;

  errno = 0;
  dwRes = fwrite(cBuf, 1, dwLength, m_fImageFile);
  //*dwResult += dwRes;

  if (dwRes != dwLength) // "out of space" or "file too large"
    {
      showDebug(1, "ERROR: dwRes != dwLength (dwRes=%lu and dwLen=%lu)\n",dwRes,dwLength);
      dwFirstWritten = dwRes;

      if (errno == EFBIG) // if "file too large" (larger than 2 GB)
	{
	  showDebug(1, "ERROR: file too large\n");
	  *nResult = ERR_FILETOOLARGE; return; //THROW(ERR_FILETOOLARGE);
	}
      else // "no space left"
	{
	  // 1. free a little space
	  nRes = destroySpaceFile(); 
	  if (nRes == -1)
	    {*nResult = ERR_DESTROYSPACEFILE; return; }
	    //THROW(ERR_DESTROYSPACEFILE);
	  
	  // 2. write the end of data
	  dwRes = fwrite(cBuf+dwFirstWritten, 1, dwLength-dwFirstWritten, m_fImageFile);
	  //*dwResult += dwRes;
	  if (dwRes != dwLength-dwFirstWritten) // out of space
	    {
	      *nResult = ERR_INCOMPLETEFINISH;
	      showDebug(3, "ERROR: ERR_INCOMPLETEFINISH (dwRes != dwLength-dwFirstWritten) error %d = %s "
			"[dwRes=%lu and dwLength-dwFirstWritten=%lu]\n", 
			errno, strerror(errno), dwRes, dwLength-dwFirstWritten);
	      newtWinMessage("error", "ok", "ERROR: ERR_INCOMPLETEFINISH (dwRes != dwLength-dwFirstWritten) "
			     "error %d = %s [dwRes=%lu and dwLength-dwFirstWritten=%lu]\n", 
			     errno, strerror(errno), dwRes, dwLength-dwFirstWritten);
	    }
	  else // no error
	    {
	      *nResult = ERR_INCOMPLETE;
	      showDebug(3, "ERROR: ERR_INCOMPLETE. error = %d = %s\n", errno, strerror(errno));
	    }
	}
    }

  // check for SPLIT_SIZE
  get_qwCurrentVolumeSize(&qwVolumeSize);
  if ( (m_options.qwSplitSize) && (qwVolumeSize + ENDVOL_MARGIN > m_options.qwSplitSize))
    {
      *nResult = ERR_SPLITSIZE;
    }

  showDebug(1, "m_qwTotal = %lld\n", m_qwTotal);
}

// =======================================================
// may raise ERR_OPENED, ERR_EOF
DWORD CImageDisk::read(void *buf, DWORD dwLength) // [Buffer-Thread]
{
  if (m_bIsOpened == false)
    THROW(ERR_OPENED);

  DWORD dwRes;
  errno = 0;
  
  // ****************************************

  //clearerr(m_fImageFile);
  dwRes = fread(buf, 1, dwLength, m_fImageFile);

  if (dwRes < dwLength)
    {
      showDebug(1, "EOF: dwRes=%lu and dwLength=%lu: MISSING: %lu\n", dwRes, dwLength, dwLength-dwRes);
    }

  m_qwTotal += (QWORD) dwRes; //dwLength;
  showDebug(1, "cid: %ld ; m_qwTotal = %lld\n", dwRes, m_qwTotal);

  return dwRes; // important: must return dwRes even if EOF reached
}

// =======================================================
void CImageDisk::unlockImageFile() // [Main-Thread]
{
#ifndef DEVEL_SUPPORT
  flock(m_nImageFd, LOCK_UN);
  showDebug(4, "UNLOCK file %s\n", m_szImageFilename);
#endif // DEVEL_SUPPORT
}

// =======================================================
// may raise an errno exception
void CImageDisk::close() // [Main-Thread]
{
  BEGIN;
 
  showDebug(1, "close -> %lld\n", m_qwTotal); 
  if (m_bIsOpened == false)
    RETURN;
  
  unlockImageFile();

  int nRes = 0;
  errno = 0;

  destroySpaceFile();

  nRes = fclose(m_fImageFile);	
  if (nRes)
    THROW(ERR_ERRNO, errno);

  m_bIsOpened = false;
  
  RETURN;
}

// =======================================================
int CImageDisk::getCompressionLevelForImage(char *szFilename) // [Main-Thread]
{
  //return(COMPRESS_LZO);

  BEGIN;

  if (!strcmp(szFilename, "stdin"))
    return COMPRESS_NONE;

  gzFile gzImageFile;
  FILE *fImageFile;
  BZFILE *bzImageFile;
  DWORD dwRes;
  CVolumeHeader headVolume;
  char *szLabel = MAGIC_BEGIN_VOLUME;
  char cBuf[64];
  
  showDebug(3, "TRACE_001\n");

  // ------ 0. Check for LZO compression
  fImageFile = fopen(szFilename, "rb");
  if (fImageFile == NULL)
    goto checkBzip2;
  dwRes = fread(cBuf, 1, 16, fImageFile);
  fclose(fImageFile);	
  if (dwRes != 16)
    goto checkBzip2;
  if (strncmp(cBuf+1, "LZO", 3) == 0)
    RETURN_int(COMPRESS_LZO);

  showDebug(3, "TRACE_002\n");

  // ------ 1. Check for a bzip2 compression
checkBzip2:
  bzImageFile = BZ2_bzopen(szFilename, "rb");
  if (bzImageFile == NULL)
    goto checkNone;
  dwRes = BZ2_bzread(bzImageFile, &headVolume, sizeof(CVolumeHeader));
  BZ2_bzclose(bzImageFile);
  if (dwRes != sizeof(CVolumeHeader))
    goto checkNone;
  if (strncmp(headVolume.szMagicString, szLabel, strlen(szLabel)) == 0)
    RETURN_int(COMPRESS_BZIP2);

  showDebug(3, "TRACE_003\n");
  
  // ------ 2. Check for no compression
 checkNone:
  fImageFile = fopen(szFilename, "rb");
  if (fImageFile == NULL)
    goto checkGzip;
  dwRes = fread(&headVolume, 1, sizeof(CVolumeHeader), fImageFile);
  fclose(fImageFile);	
  if (dwRes != sizeof(CVolumeHeader))
    goto checkGzip;
  if (strncmp(headVolume.szMagicString, szLabel, strlen(szLabel)) == 0)
    RETURN_int(COMPRESS_NONE);

  showDebug(3, "TRACE_004\n");
  
  // ------ 3. Check for a gzip compression
 checkGzip:
  gzImageFile = gzopen(szFilename, "rb");
  if (gzImageFile == NULL)
    RETURN_int(-1); // error
  dwRes = gzread(gzImageFile, &headVolume, sizeof(CVolumeHeader));
  gzclose(gzImageFile);
  if (dwRes != sizeof(CVolumeHeader))
    RETURN_int(-1); // error
  if (strncmp(headVolume.szMagicString, szLabel, strlen(szLabel)) == 0)
    //if (strcmp(headMain.szAppName, "PartImage") == 0)
    RETURN_int(COMPRESS_GZIP);
  
  showDebug(3, "TRACE_005\n");

  RETURN_int(-1); // error
}

// =======================================================
// may throw ERR_OPENED, ERR_NOTFOUND, errno, comp
void CImageDisk::openReading() // [Main-Thread]
{
  //int nRes;
#ifdef HAVE_FSTAT64
  struct stat64 fStat;
#else
  struct stat fStat;
#endif

  BEGIN;
  showDebug(1, "ALREADY=%llu\n", m_qwTotal);
  m_qwTotal=0LL;
  if (m_bIsOpened == true)
    THROW(ERR_OPENED);

  if (strcmp(m_szImageFilename, "stdin"))
    {
      // check the file exists
      errno = 0;
      if (access(m_szImageFilename, F_OK) == -1) // if the file doesn't exists
        THROW(ERR_NOTFOUND);
  
      showDebug(1, "openReading[%s]\n", m_szImageFilename);

    #ifdef OS_LINUX
      #ifdef HAVE_OPEN64
        m_nImageFd = open64(m_szImageFilename, O_RDONLY | O_LARGEFILE | O_NOFOLLOW);
      #else
        m_nImageFd = open(m_szImageFilename, O_RDONLY | O_LARGEFILE | O_NOFOLLOW);
      #endif
    #endif
    #ifdef OS_FBSD
      m_nImageFd = open(m_szImageFilename, O_RDONLY | O_NOFOLLOW);
    #endif
      if (m_nImageFd == -1)
        THROW(ERR_ERRNO, errno);
      else 
        showDebug(1, "preopen successfull for %s\n", m_szImageFilename);

      // ---- avoid RACE CONDITIONS security problems:
      errno = 0;
    #ifdef HAVE_FSTAT64
      if (fstat64(m_nImageFd, &fStat) == -1)
    #else
      if (fstat(m_nImageFd, &fStat) == -1)
    #endif
        {
          showDebug(1, "fstat() returned -1 and errno=%d=[%s]\n", errno, strerror(errno));
          THROW(ERR_ERRNO, errno);
        }

      if (!S_ISREG(fStat.st_mode)) // not a regular file
        THROW(ERR_NOTAREGULARFILE, m_szImageFilename); //BIGH
  
      // ---- lock the image file
      if (lockImage(LOCK_SH, m_nImageFd, m_szImageFilename) == -1)
        THROW(ERR_LOCKED, m_szImageFilename);
  
      m_fImageFile = fdopen(m_nImageFd, "rb");
      if (m_fImageFile == NULL)
        THROW( errno);
      else
        showDebug(1, "open\n");
      showDebug(1, "file %s opened ok\n", m_szImageFilename);
    } // stdin?
  else
    { 
      m_fImageFile = stdin;
      showDebug(1, "stdin opened for reading\n");
    }
	
  m_bIsOpened = true;
}	

// =======================================================
int CImageDisk::destroySpaceFile() // [Main-Thread]
{
  int nRes;

  showDebug(1, "DESTROY SPACE FILE [%s]\n", m_szSpaceFilename);

  //newtWinMessage("before","b","erase %s", m_szSpaceFilename);
  nRes = unlink(m_szSpaceFilename);
  //newtWinMessage("after","b","erase %s", m_szSpaceFilename);
  return nRes;
}

// =======================================================
int CImageDisk::createSpaceFile() // [Main-Thread]
{
  char cBuffer[1024];
  //int nSpaceFd;
  FILE *fStream;
  DWORD dwFileSize;
  int nRes;
  WORD i;

  showDebug(1, "CREATE SPACE FILE [%s]\n", m_szSpaceFilename);

  // write random numbers
  for (i=0; i < sizeof(cBuffer); i++)
    cBuffer[i] = (i ^ 94) % 256;
  
  // open file descriptor ICI
  fStream = openFileDescriptorSecure(m_szSpaceFilename, "a", O_WRONLY | 
     O_CREAT | O_NOFOLLOW | O_TRUNC, S_IREAD | S_IWRITE);

  if (!fStream)
    return -1;

  dwFileSize = 0;
  while (dwFileSize < SPACEFILE_SIZE)
    {
      //nRes = ::write(nSpaceFd, cBuffer, sizeof(cBuffer));
      nRes = fwrite(cBuffer, sizeof(cBuffer), 1, fStream);
      if (nRes != 1)
	{
	  fclose(fStream);
	  destroySpaceFile();
	  return -1;
	}
      dwFileSize += sizeof(cBuffer);
    }

  nRes = fclose(fStream);
  if (nRes == -1)
    return -1;

  return 0; // success
}

// =======================================================
// may raise ERR_OPENED, ERR_EXIST, errno
void CImageDisk::openWriting() // [Main-Thread]
{
#ifdef HAVE_FSTAT64
  struct stat64 fStat;
#else
  struct stat fStat;
#endif
  int flags;
  int nRes;
  char szShortName[32]; // DOS short filenames
  char szPathDir[MAXPATHLEN];

  BEGIN;

  showDebug(1, "ALREADY=%lld\n", m_qwTotal);

  if (m_bIsOpened == true)
    THROW(ERR_OPENED);
  
  SNPRINTF(szShortName, "pi%.8lx", (DWORD)generateIdentificator());
  extractFilepathFromFullPath(m_szImageFilename, szPathDir); // filepath without filename
  snprintf(m_szSpaceFilename, MAXPATHLEN, "%s/%8s.tmp", szPathDir, szShortName);
  showDebug(1, "TEMP=[%s]\n", m_szSpaceFilename);

  if (strcmp(m_szImageFilename, "stdout"))
    { // we create spacefile only if imagefile is not stdout
      errno = 0;
      nRes = createSpaceFile();
      if (nRes == -1)
        {
          if (errno == ENOSPC)
    	    {
	      showDebug(1, "No space left -> cannot create file [%s]\n",
                 m_szSpaceFilename);
	      THROW(ERR_CREATESPACEFILENOSPC, szPathDir);
    	    }
          else if (errno == EACCES)
            {
              showDebug(1, "Permission denied -> cannot create file [%s]\n",
                 m_szSpaceFilename);
	      THROW(ERR_CREATESPACEFILEDENIED, szPathDir);
    	    }
          else
	    {
	      showDebug(1, "Cannot create SpaceFile[%s]\n", m_szSpaceFilename); 
	      THROW(ERR_CREATESPACEFILE, m_szSpaceFilename);
	    }
        }
    }

  if (strcmp(m_szImageFilename, "stdout"))
    {
      errno = 0;
      flags = O_CREAT | O_WRONLY | O_NOFOLLOW;
    #ifdef OS_LINUX
      flags |= O_LARGEFILE;
    #endif
      if (m_options.bOverwrite != true)
        flags |= O_EXCL;

    #ifdef HAVE_OPEN64
      m_nImageFd = open64(m_szImageFilename, flags, S_IREAD | S_IWRITE);
    #else
      m_nImageFd = open(m_szImageFilename, flags, S_IREAD | S_IWRITE);
    #endif
      if ((m_nImageFd == -1))
        if (errno == EEXIST)
          THROW(ERR_EXIST);
        else
          THROW(ERR_ERRNO, errno);

      // ---- avoid RACE CONDITIONS security problems:
    #ifdef HAVE_FSTAT64
      if (fstat64(m_nImageFd, &fStat) == -1)
    #else
      if (fstat(m_nImageFd, &fStat) == -1)
    #endif
        THROW(ERR_ERRNO, errno);
  
      if (!S_ISREG(fStat.st_mode)) // not a regular file
        THROW(ERR_NOTAREGULARFILE, m_szImageFilename);

      // lock the image file
      if (lockImage(LOCK_EX, m_nImageFd, m_szImageFilename) == -1)
        THROW(ERR_LOCKED, m_szImageFilename);

/* FIXME: this fails when using LUFS. Is it required?
      // truncate the file (now, we can because it's not a symlink)
      if (ftruncate(m_nImageFd, 0L) == -1)
        THROW(ERR_ERRNO, errno);
*/

      showDebug(1, "open std\n");
      m_fImageFile = fdopen(m_nImageFd, "wb");
      if (m_fImageFile == NULL)
        {
          showDebug(1, "error:%d %s\n", errno, strerror(errno));
          THROW(ERR_ERRNO, errno);
        }
    }
  else // it's stdout
    {
      m_fImageFile = stdout;
      showDebug(1, "image will be on stdout\n");
    }

  m_bIsOpened = true;
  m_qwVolSize = 0LL;

  showDebug(1, "end of CImageDisk::openWriting(), return = ok\n");
}

// =======================================================
// flag = LOCK_SH | LOCK_EX if called when restoring or saving 
int CImageDisk::lockImage(int flag, int nFd, char *szFilename) // [Main-Thread]
{
#ifndef DEVEL_SUPPORT
  int nRes;

  errno = 0;

  if (flag != LOCK_SH && flag != LOCK_EX)
    {
      showDebug(1, "LOCK flag is %d\n", flag);
      RETURN_int(-1);
    }

  nRes = flock(m_nImageFd, flag | LOCK_NB);
  if (nRes == -1)
    RETURN_int(-1);
  showDebug(4, "LOCK file %s\n", m_szImageFilename);
#endif // DEVEL_SUPPORT

  return 0;
}

// =======================================================
// may throw errno
#ifdef HAVE_SYS_STATFS_H
void CImageDisk::getFreeSpace(QWORD *qwAvailDiskSpace) // [Main-Thread]
{
  int nRes;
#ifdef HAVE_FSTATFS64
  struct statfs64 fsInfo;
#else
  struct statfs fsInfo;
#endif

  *qwAvailDiskSpace = 0LL;

#ifdef HAVE_FSTATFS64
  nRes = fstatfs64(m_nImageFd, &fsInfo);
#else
  nRes = fstatfs(m_nImageFd, &fsInfo);
#endif
  if (nRes == -1) // error
    THROW(ERR_ERRNO, errno);
	
  *qwAvailDiskSpace = ((QWORD) fsInfo.f_bavail) * ((QWORD)fsInfo.f_bsize);
}
#else //ifdef HAVE_SYS_STATFS_H
void CImageDisk::getFreeSpace(QWORD *qwAvailDiskSpace)
{ 
  #warning "getFreeSpace always return 0"
  *qwAvailDiskSpace =  0LL;
}
#endif

// =======================================================
bool CImageDisk::doesFileExists(char *szPath) // [Main-Thread]
{
  //return ::doesFileExists2(szPath);
  BEGIN;

  //FILE *fStream;
#ifdef HAVE_FSTAT64
  struct stat64 fStat;
#else
  struct stat fStat;
#endif
  int nFd;
  int nRes;

  errno = 0;
  if ( strcmp(szPath, "stdin") ) {
#ifdef HAVE_OPEN64
  nFd = open64(szPath, O_RDONLY | O_NOFOLLOW | O_LARGEFILE);
#else
  nFd = open(szPath, O_RDONLY | O_NOFOLLOW | O_LARGEFILE);
#endif
  } else { // it's stdin
  nFd=0;
  return true;
  }
  showDebug(3, "doesFileExists(%s) =>%d\n",szPath,nFd);
  //fStream = fopen(szPath, "rb");
  //if (!fStream) // error
  if (nFd == -1)
    {
      RETURN_int (errno != ENOENT);
    }
  else // success
    {
#ifdef HAVE_FSTAT64
      nRes = fstat64(nFd, &fStat);
#else
      nRes = fstat(nFd, &fStat);
#endif
      showDebug(3, "fstat() returned %d\n", nRes);
      if (nRes == -1)
	return true; // for large files
      //fclose(fStream);
      ::close(nFd);
      showDebug(3, "fStat.st_mode=%ld\n",(long)fStat.st_mode);
      RETURN_int(S_ISREG(fStat.st_mode));
    }
}

// =======================================================
#ifdef HAVE_SYS_STATFS_H
void CImageDisk::getDiskFreeSpaceForFile(QWORD *qwAvailDiskSpace, char *szFilepath) // [Main-Thread]
{
  //::getDiskFreeSpaceForFile(qwFreeSpace, szFilepath);
  int nRes;
  struct statfs fsInfo;
  
  *qwAvailDiskSpace = 0LL;
  
  errno = 0;
  nRes = statfs(szFilepath, &fsInfo);
  if (nRes == -1) // error
    {
      if (errno == ENOENT) // file doesn't exists
	{	
	  // create an empty file
	  FILE *fFile;
	  fFile = fopen(szFilepath, "w");
	  if (fFile == NULL)
	    RETURN;
	  fclose(fFile);
	  getDiskFreeSpaceForFile(qwAvailDiskSpace, szFilepath);
	  unlink(szFilepath);
	  RETURN;;
	}
      else // another error
	{
	  RETURN;
	}
      
    }	
	
  *qwAvailDiskSpace = ((QWORD) fsInfo.f_bavail) * ((QWORD)fsInfo.f_bsize);
  
  RETURN;
}
#else // HAVE_SYS_STATFS_H
void CImageDisk::getDiskFreeSpaceForFile(QWORD *qwAvailDiskSpace, char *szFilepath) // [Main-Thread]
{
  #warning "getDiskFreeSpaceForFile always return 5 MB"
  *qwAvailDiskSpace = (QWORD)(5LL*1024LL*1024LL);
  return 0;
}
#endif // HAVE_SYS_STATFS_H

// =======================================================
int CImageDisk::mountImageLocation(char * szMountDevice, char * szMountPoint,
   char * szMountFS) // [Main-Thread]
{
  int nRes;
  
  errno = 0;
  showDebug(1, "automount-> %s %s %s\n", szMountDevice, szMountPoint,szMountFS);
#ifdef OS_LINUX
  if (!strcasecmp(szMountFS, "iso9660"))
    nRes = mount(szMountDevice, szMountPoint, szMountFS, MS_RDONLY, NULL);
  else if (!strcasecmp(szMountFS, "udf"))
    nRes = mount(szMountDevice, szMountPoint, szMountFS, MS_NOATIME, NULL);
  else
    nRes = mount(szMountDevice, szMountPoint, szMountFS, 0, NULL);
#endif
#ifdef OS_FBSD
  nRes = mount(szMountFS, szMountPoint, 0, NULL);
#endif

  if ((nRes) && (errno != EBUSY))  // EBUSY==already mounted
    {
      showDebug(1, "mount failed: %d (%s)\n", errno, strerror(errno));
      return -1;
    }
  else
    return 0;
}  

// =======================================================
int CImageDisk::umountImageLocation(char *szMountPoint) // [Main-Thread]
{
  sync();
  
#ifdef OS_LINUX
  return umount(szMountPoint);
#endif
#ifdef OS_FBSD
  return unmount(szMountPoint, 0);
#endif
}

