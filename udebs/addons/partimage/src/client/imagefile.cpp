/***************************************************************************
                          imagefile.cpp  -  description
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

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
  #include <sys/mount.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "partimage.h"
#include "imagefile.h"
#include "common.h"
#include "gui_text.h"
#include "errors.h"
#include "exceptions.h"
#include "interface.h"
#include "interface_newt.h"

#include <zlib.h> // gzip compression
#include <bzlib.h> // bzip2 compression

CParam g_param;

// =======================================================
CImage::CImage(COptions * options)
{
  char * passwd = (char *) malloc (2048);
  char * login = (char *) malloc (2048);

  BEGIN;
  m_dwVolumeNumber = 0;
  memcpy(&m_options, options, sizeof(COptions));

  if (!strlen(options -> szServerName)) //local
    m_cid = new CImageDisk(options);
  else
    {
#ifdef MUST_LOGIN
      if (strlen(options -> szUserName) && strlen(options -> szPassWord))
        {
          strncpy(login, options->szUserName, 2048);
          strncpy(passwd, options->szPassWord, 2048);
          *(login+2047) = '\0';
          *(passwd+2047) = '\0';
        }
      else 
        {
// we get login/pass from user
          int nRes;
          nRes = g_interface -> askLogin(login, passwd, 2048);
          if (nRes)
            {
              free(login);
              free(passwd);
              THROW( ERR_CANCELED);
            }
        }	
#else
// fake login/passwd
      strcpy(login, "nologin");
      strcpy(passwd, "nopass");
#endif
		
      m_cid = new CImageNet(options, options->bUseSSL);
      try { m_cid -> Connect(options, login, passwd, options->szServerName,
         options->dwServerPort); }
      catch ( CExceptions * excep )
        {
          free(login);
          free(passwd);
          throw excep;
        }  
      free(login);
      free(passwd);
    }
  m_cid -> set_bIsOpened(false);

  m_qwCRC = 0LL;

  m_guiSave = NULL;
  m_guiRestore = NULL;

  // mount options
  m_bIsMounted = false;
  memset(m_szMountDevice, 0, MAXPATHLEN);
  memset(m_szMountPoint, 0, MAXPATHLEN);
  memset(m_szMountFS, 0, MAXPATHLEN);

  // parse mount options
  if (m_options.szAutoMount)
    {
      char *cBegin = m_options.szAutoMount;
      int i;
      for (i=0; (*cBegin != '#') && (*cBegin); i++,cBegin++)
	m_szMountDevice[i] = *cBegin;
      cBegin++;
      for (i=0; (*cBegin != '#') && (*cBegin); i++,cBegin++)
	m_szMountPoint[i] = *cBegin;
      cBegin++;
      for (i=0; (*cBegin != '#') && (*cBegin); i++,cBegin++)
	m_szMountFS[i] = *cBegin;
    }
  
  m_fImageFile = NULL;	
  m_gzImageFile = NULL;
  m_bzImageFile = NULL;

  RETURN;
}

// =======================================================
CImage::~CImage()
{
  BEGIN;

  // to close the image file if there was an error
  if (get_bIsOpened())
    m_cid->close();

  delete m_cid;

  RETURN;
}

// =======================================================
QWORD CImage::get_qwCurrentVolumeSize() 
{
  QWORD qwVolumeSize;
  m_cid->get_qwCurrentVolumeSize(&qwVolumeSize);

  return qwVolumeSize;
}

// =======================================================
DWORD CImage::get_dwVolumeNumber()
{
  return m_dwVolumeNumber;
}

bool  CImage::get_bIsOpened()
{
  return m_cid -> get_bIsOpened();
}
char * CImage::get_szImageFilename()
{
  return m_cid -> get_szImageFilename();
}
char * CImage::get_szOriginalFilename()
{
  return m_cid -> get_szOriginalFilename();
}
char * CImage::get_szPath()
{
  return m_cid -> get_szPath();
}
void CImage::set_dwVolumeNumber(DWORD n)
{
  m_dwVolumeNumber = n;
}
void CImage::set_bIsOpened(bool n)
{
  m_cid -> set_bIsOpened(n);
}
void CImage::set_szPath(char * str)
{
  m_cid -> set_szPath(str);
}
void CImage::set_szImageFilename(char * str)
{
  m_cid -> set_szImageFilename(str);
}
void CImage::set_szOriginalFilename(char * str)
{
  m_cid -> set_szOriginalFilename(str);
}
void CImage::set_szImageFilename(char * str, int n)
{
  m_cid -> set_szImageFilename(str, n);
}
void CImage::set_szOriginalFilename(char * str, int n)
{
  m_cid -> set_szOriginalFilename(str, n);
}
void CImage::set_qwIdentificator(QWORD n)
{
  showDebug(0, "Identificator in CMainHeader = %llu = %llX\n", n, n);
  m_qwIdentificator = n;
}
void CImage::set_options(COptions * options)
{
  showDebug(1, "SETOPTIONS\n");
  memcpy(&m_options, options, sizeof(COptions));
  m_cid->set_options(options);
}

// =======================================================
void CImage::writeSplit()
{
  BEGIN;

  char aux[MAXPATHLEN];
  QWORD qwFreeSpace;
  int nRes;

  if (m_options.bBatchMode) // error
    {
      showDebug(1, "---> CI::write(): No space left to write the image "
		"file\n");
      THROW( ERR_FULL);
    }

  qwFreeSpace = 0;
  do
    {
      nRes = g_interface -> askNewPath(get_szOriginalFilename(), get_dwVolumeNumber()+1, get_szPath(), aux, MAXPATHLEN);
      if (nRes == -1)
	THROW( ERR_CANCELED);
	  
      while (aux[strlen(aux)-1] == '/')
	aux[strlen(aux)-1] = '\0';
      set_szPath(aux);
	  
      // get disk space on new path
      SNPRINTF(aux, "%s/%s.test", get_szPath(), get_szOriginalFilename());
	      
      //debugWin("NEWPATH=[%s]", aux);

      try { m_cid->getDiskFreeSpaceForFile(&qwFreeSpace, aux); }   
      catch ( CExceptions * excep )  // exception = errno
	{
	  showDebug(3, "exception caught: %d\n", excep->GetExcept());
	  qwFreeSpace = 0LL;
	}
    } while (qwFreeSpace < /*ENDVOL_MARGIN*/ SOME_SPACE); // while no space left
      
  try { splitChangeImageFileWrite(); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "exception caught : %d\n", excep->GetExcept());
      throw excep;
    }
      
  /*try { m_cid->getFreeSpace(&qwFreeSpace); }   
  catch ( CExceptions * excep )  // exception = errno
    { qwFreeSpace = 0LL; }*/
      
  m_guiSave -> showImageFileInfo(get_szImageFilename(), qwFreeSpace, 0, m_options.szFullyBatchMode);
  showDebug(2, "imagefile split done\n");

  RETURN;
}

// =======================================================
void CImage::write(void *vBuf, DWORD dwLength, bool bUpdateCRC)
{
  static QWORD qwCount = 0LL;
  static QWORD qwTotalBytes = 0LL;
  QWORD qwFreeSpace; //, qwCurrentSize;
  char *cBuf;
  DWORD dwRes;

  BEGIN;

  showDebug(3, "begin of ci::write for %ld (bUpdateCRC = %d)\n", dwLength, bUpdateCRC);

  cBuf = (char*) vBuf;

  if (bUpdateCRC)
    {
      DWORD i;
      BYTE * cBuffer = (BYTE *) vBuf;
      for (i = 0; i < dwLength; i++)
	m_qwCRC += (cBuffer[i]);
    }

  // ---- SPLIT_SIZE need ?
  //if ( (m_options.dwSplitMode == SPLIT_SIZE) && (get_qwCurrentVolumeSize() + dwLength + ENDVOL_MARGIN > m_options.qwSplitSize))
  if (g_nThreadError == ERR_SPLITSIZE)
    {
      g_nThreadError = 0;
      try { splitChangeImageFileWrite(); }
      catch ( CExceptions * excep )
	// may catch canceled, opened, exist, errno, access, incomplete
	{ 
	  showDebug(1, "write: error 1\n");
	  throw excep;
	}
      
      try { m_cid->getFreeSpace(&qwFreeSpace); }   
      catch ( CExceptions * excep )  // exception = errno
	{ qwFreeSpace = 0LL; }
      
      m_guiSave -> showImageFileInfo(get_szImageFilename(), qwFreeSpace, 0, m_options.szFullyBatchMode);
      showDebug(2, "imagefile split done\n");
    }
  
  // ---- fatal error
  if (g_nThreadError)
    {
      showDebug(1, "Fatal error: %d\n", g_nThreadError);
      THROW(g_nThreadError); //THROW(ERR_ERRNO);
    }
 
  // ---- no space left ?
  if (g_bSpaceOnDisk == false)
    {
      writeSplit();
      g_bSpaceOnDisk = true;
    }

  // ---- compress and copy data
  if (m_options.dwCompression == COMPRESS_NONE)
    dwRes = (DWORD) fwrite(cBuf, 1, dwLength, m_fImageFile);
  else if (m_options.dwCompression == COMPRESS_GZIP)
    dwRes = (DWORD) gzwrite(m_gzImageFile, cBuf, dwLength);
  else if (m_options.dwCompression == COMPRESS_BZIP2)
    dwRes = (DWORD) BZ2_bzwrite(m_bzImageFile, cBuf, dwLength); 
  else
    THROW(ERR_COMP);
  
  if (dwRes != dwLength)
    showDebug(1, "ERROR: dwLen=%lu and dwRes=%lu\n", dwLength, dwRes);

  qwTotalBytes += (QWORD) dwLength;

  // ---- update the window ----
  if (qwCount == 0)
    {
      try { m_cid->getFreeSpace(&qwFreeSpace); }
      catch ( CExceptions * excep )
	{ qwFreeSpace = 0LL; }

      if (m_guiSave)
	m_guiSave -> showImageFileInfo(get_szImageFilename(), qwFreeSpace, get_qwCurrentVolumeSize(), m_options.szFullyBatchMode);  
    }

  qwCount += (QWORD) dwRes;
  if (qwCount > (1024 * 1024)) // update the window every 1 MB
    qwCount = 0LL;

  showDebug(3, "write end -> %ld\n", dwLength);
  RETURN;
}


// =======================================================
// may throw opened, eof (from cid)
// may throw opened, errno, comp, canceled, volumeheader,
//           partitionvolume, volumenumber (from splitchangeimagefileread)
// may throw eof
void CImage::read(char *cBuf, DWORD dwLength, bool bUpdateCRC)
{
  DWORD i;
  static QWORD qwTotalBytes = 0LL;
  static QWORD qwCount = 0LL;
  DWORD dwRes;
  int nRes;
 
  if (dwLength != 1) // avoid having BUFFER_BLOCK_SIZE messages with imginfo and restmbr
    showDebug(4, "begin of ci::read: size=%lu, pos=%lld\n", dwLength, qwTotalBytes);

  if (m_options.dwCompression == COMPRESS_NONE)
    nRes = fread(cBuf, 1, dwLength, m_fImageFile);
  else if (m_options.dwCompression == COMPRESS_GZIP)
    nRes = gzread(m_gzImageFile, cBuf, dwLength);
  else if (m_options.dwCompression == COMPRESS_BZIP2)
    nRes = BZ2_bzread(m_bzImageFile, cBuf, dwLength);
  else
    THROW(ERR_COMP);

  dwRes = (DWORD) nRes;

  // update the CRC control if the read operation is whole successful
  if ((bUpdateCRC) && (nRes > 0))
    {
      BYTE *cBuffer = (BYTE *) cBuf;
      for (i=0; i < dwRes; i++)
        m_qwCRC += (cBuffer[i]);
    }

  if (dwRes != dwLength)
    {
      if (dwLength != 1)
	showDebug(1, "READ ERROR: dwLength=%lu and dwRes=%lu and g_nThreadState=%d\n", dwLength, dwRes, g_nThreadState);

      if (g_nThreadState == THREAD_EOF)
	{
	  DWORD dwFirstRead = dwRes;
	  try { splitChangeImageFileRead(); }
	  catch ( CExceptions * excep ) // opened, errno, canceled, volumeheader
	    // partition volume, volumenumber
	    {
	      showDebug(1, "ci::read: exception raised from splitChangeImageFileRead()\n");
	      throw excep;
	    }
          qwTotalBytes = 0LL;
	  read(cBuf+dwFirstRead, dwLength-dwFirstRead, bUpdateCRC);
	  m_guiRestore -> showImageFileInfo(get_szImageFilename(), m_options.dwCompression, m_options.szFullyBatchMode);
	}
      else
	{
	  showDebug(1, "READ ERROR 2: dwLength=%lu and dwRes=%lu and g_nThreadState=%d\n", dwLength, dwRes, g_nThreadState);
	  THROW(ERR_READING);
	}
    }

  qwTotalBytes += (QWORD) dwLength;

  // ---- update the window ----
  qwCount += (QWORD) dwLength;
  if (qwCount > (1024 * 1024)) // update the window every 1 MB
    {
      qwCount = 0LL;
      if (m_guiRestore)
	m_guiRestore -> showImageFileInfo(get_szImageFilename(), m_options.dwCompression, m_options.szFullyBatchMode);
    }

  if (dwLength != 1)
    showDebug(4, "end of ci::read: %lu/%lu\n", dwRes, dwLength);
  RETURN;
}

// =======================================================
// may throw errno (from close)
// may throw canceled, opened, exist, errno, access (from openwritting)
// may throw incomplete (from cid:write)
// may throw canceled, incomplete 

void CImage::splitChangeImageFileWrite()
{
  BEGIN;

  char aux [MAXPATHLEN];
  int nRes;

  // close old imagefile
  try { closeWriting(); }
  catch ( CExceptions * excep ) // catch errno
    { throw excep; } 

  //execute /tmp/partimage-shell

  if (m_options.bRunShell)
    {
      if (!fork()) // we're the child // no error check maid
        execl("/tmp/partimage-shell", 
           get_szImageFilename(), get_szPath(),
           get_szOriginalFilename(), get_dwVolumeNumber());
    }

  // wait for a keyboard keypress if need
  if (m_options.bSplitWait)
    {
      nRes = g_interface -> WaitKeyPressed(get_szImageFilename(), get_szPath(), 
					   get_szOriginalFilename(), get_dwVolumeNumber()+1);
      if (nRes == MSGBOX_CANCEL)
	throw ERR_CANCELED;
    }

  set_dwVolumeNumber(get_dwVolumeNumber()+1); // increase the volume number
  SNPRINTF(aux, "%s/%s.%.3ld", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber());
  //*(aux+MAXPATHLEN-1) = '\0';
  
  set_szImageFilename(aux);
  try { openWriting(); }
  catch ( CExceptions * excep ) // catch opened, exist, errno, comp, access, canceled
    {
      showDebug(2, "splitW: exception raised from openW\n");
      throw excep;
    }
  RETURN;
}

// =======================================================
// may throw errno (from close)
// may throw opened, comp, eof (from cid::read)
// may throw canceled, opened, errno, comp (from openreading)
// may throw volumeheader, partitionvolume, volumenumber
void CImage::splitChangeImageFileRead()
{
  BEGIN;

  char aux[MAXPATHLEN];

  // increase volume number
  set_dwVolumeNumber(get_dwVolumeNumber()+1);
  SNPRINTF(aux, "%s/%s.%.3ld", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber());
  //*(aux+MAXPATHLEN-1) = '\0';
  set_szImageFilename(aux);

  try { closeReading(); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "exception raised from close\n");
      throw excep; 
    }

  try { openReading(); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "exception %d caught from openReading\n",
         excep->GetExcept());
      throw excep;
    }
}

// =======================================================
// may throw err (from cid)
void CImage::closeReading(bool bForceExit /*  = false */)
{
  BEGIN;

  int nRes = 0;
  char c;

  showDebug(3, "TRACE_000\n");
  
  // if not the end of the image file, avoid SIGPIPE
  if (bForceExit)
    {
      showDebug(1, "bForceExit: g_nThreadState = THREAD_ASKEXIT\n");
      g_nThreadState = THREAD_ASKEXIT;

      // unlock the buffer thread, if it does not have seen
      // it must exit
      while (g_nThreadState != THREAD_FINISHED)
	read(&c, 1, false);

      //pthread_join(g_threadBuffer, NULL);
    }
  
  showDebug(3, "TRACE_001\n");

  //pthread_join(g_threadBuffer, NULL);
  // MULTI-THREAD TERMISAISON
  if (m_options.dwCompression == COMPRESS_NONE) // No compression
    nRes = fclose(m_fImageFile);	
  else if (m_options.dwCompression == COMPRESS_GZIP) // Gzip compression
    { 
      char szTemp[2048];
      nRes = gzclose(m_gzImageFile);
      showDebug(3, "GZCLOSE: result=%s\n", formatGzipError(nRes, szTemp, sizeof(szTemp)));
    }
  else if (m_options.dwCompression == COMPRESS_BZIP2) // Bzip2 compression
    BZ2_bzclose(m_bzImageFile);
  if (nRes)
    THROW(ERR_ERRNO, errno);

  showDebug(3, "TRACE_002\n");

  try { m_cid -> close(); }
  catch ( CExceptions * excep)
    {
      showDebug(1, "exception raised\n");
      throw excep;
    }

  // umount image file location
  umountImageLocation();

  RETURN;
}

// =======================================================
// may throw err (from cid)
void CImage::closeWriting()
{
  BEGIN;
  int nRes = 0;
 
  if (m_options.dwCompression == COMPRESS_NONE) // No compression
    nRes = fclose(m_fImageFile);	
  else if (m_options.dwCompression == COMPRESS_GZIP) // Gzip compression
    nRes = gzclose(m_gzImageFile);
  else if (m_options.dwCompression == COMPRESS_BZIP2) // Bzip2 compression
    BZ2_bzclose(m_bzImageFile);
  if (nRes)
    THROW(ERR_ERRNO, errno);
  
  showDebug(1, "PTHREAD_JOIN: before\n");
  pthread_join(g_threadBuffer, NULL);
  showDebug(1, "PTHREAD_JOIN: after\n");
  
  try { m_cid -> close(); }
  catch ( CExceptions * excep)
    {
      showDebug(1, "ci::close: exception raised\n");
      throw excep;
    }

  // umount image file location
  umountImageLocation();
  RETURN;
}

// =======================================================
int CImage::mountImageLocation()
{
  int nRes;
  BEGIN;

  if ((m_options.szAutoMount) && (strlen(m_options.szAutoMount)) &&
       (!m_bIsMounted)) // if not already mounted
    {
      // warning
      g_interface->msgBoxOk(i18n("Automatic mount"),
         i18n("Please, press \"ok\" to mount [%s] on [%s]"),
         m_szMountDevice, m_szMountPoint);
      
      nRes = m_cid->mountImageLocation(m_szMountDevice, m_szMountPoint,
         m_szMountFS);

      if (nRes == -1)
	{
	  //g_interface->msgBoxError(i18n("Cannot mount %s"), m_szMountDevice);
          THROW(ERR_AUTOMOUNT, m_szMountDevice);
	  //RETURN_int(-1);	  
	}

      m_bIsMounted = true;
    }

  return 0;
}

// =======================================================
int CImage::umountImageLocation()
{
  int nRes;

  if ((m_options.szAutoMount) && (strlen(m_options.szAutoMount)) && (m_bIsMounted)) // if mounted
    {
//      debugWin("umount[%s]",m_szMountPoint);
      showDebug(1, "umount[%s]",m_szMountPoint);
      nRes = m_cid->umountImageLocation(m_szMountPoint);
      if (nRes)
        THROW(ERR_AUTOUMOUNT, m_szMountDevice);
      m_bIsMounted = false;
    }
  return 0;
}

// =======================================================
int CImage::openWritingFdDisk()
{
  BEGIN;

  int nRes;
  int nCont;
  char szAux[MAXPATHLEN], aux2[MAXPATHLEN];

  showDebug(0,"TRY: path=[%s] and file=[%s] and vol=[%d] and fully=[%s]\n", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber(), get_szImageFilename());
  try { m_cid -> openWriting(); }
  catch ( CExceptions * excep )
    { // we can recover exist, locked or path exceptions
      if (excep->GetExcept() == ERR_EXIST || excep->GetExcept() == ERR_LOCKED) 
	{  
	  if (excep->GetExcept() == ERR_EXIST)
	    nCont = excep->windowAlreadyExist(get_szImageFilename(),get_szPath());
	  else
	    nCont = excep->windowLocked(get_szImageFilename(),get_szPath());
	  switch (nCont)
	    {
	    case ERR_QUIT:
	      THROW(ERR_CANCELED);

	    case ERR_RETRY: 
	      strcpy(szAux, excep->getNewString()); // endding '/' were removed 
	      extractFilepathFromFullPath(szAux, aux2); // filepath without filename
	      set_szPath(aux2);
	      extractFilenameFromFullPath(szAux, aux2); // filename without path
	      set_szOriginalFilename(aux2);
	      SNPRINTF(szAux, "%s/%s.%.3ld", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber());
	      set_szImageFilename(szAux);
	      nRes = openWritingFdDisk();
	      RETURN_int(nRes);
	      break;

	    case ERR_CONT:
	      if (excep->GetExcept() == ERR_EXIST)
		{
		  m_options.bOverwrite = true;
		  m_cid -> set_options(&m_options);
		}
	      //openWriting(false);
	      nRes = openWritingFdDisk();
	      RETURN_int(nRes);
	      break;
	    }
	}
      else if (excep->GetExcept() == ERR_PATH)
	{
	  nCont = excep->windowWrongPath(get_szImageFilename());
	  switch (nCont)
	    {
            case ERR_QUIT:
              THROW(ERR_CANCELED);
            case ERR_RETRY:
              strcpy(szAux, excep->getNewString()); // endding '/' were removed 
              set_szPath(szAux); 
	      SNPRINTF(aux2, "%s/%s.%.3ld", szAux, get_szOriginalFilename(),
		       get_dwVolumeNumber());
              set_szImageFilename(aux2);
	      
              openWriting(/*false*/);
	      RETURN_int(0);
            }
	}
      else // other exception -> raise
	throw excep;
    }

  RETURN_int(0);
}

// =======================================================
// may throw canceled
// may throw opened, exist, errno, access (from cid)
void CImage::openWriting()
{
  BEGIN;

  int nRes;
  int nFd[2];

  errno = 0;
  nRes = pipe(nFd);
  if (nRes == -1)
    {
      showDebug(1, "Cannot call pipe. Error=%d=%s\n",errno,strerror(errno));
      THROW(ERR_ERRNO, errno);
    }

  m_nFdImage = nFd[1];

  if (m_options.dwCompression == COMPRESS_NONE) // No compression
    {
      showDebug(1, "open std\n");
      m_fImageFile = fdopen(m_nFdImage, "wb");
      if (m_fImageFile == NULL)
	{
	  showDebug(1, "error:%d %s\n", errno, strerror(errno));
	  THROW(ERR_ERRNO, errno);
	}
    }
  else if (m_options.dwCompression == COMPRESS_GZIP) // Gzip compression
    {
      showDebug(1, "open gzip\n");
      m_gzImageFile = (gzFile *) gzdopen(m_nFdImage, "wb"); //"wb1h");
      if (m_gzImageFile == NULL)
	{
	  showDebug(1, "error:%d %s\n", errno, strerror(errno));
	  THROW(ERR_ERRNO, errno);
	}
    }
  else if (m_options.dwCompression == COMPRESS_BZIP2) // Bzip2 compression
    {
      showDebug(1, "open bzip2\n");
      m_bzImageFile = BZ2_bzdopen(m_nFdImage, "wb");
      if (m_bzImageFile == NULL)
	{
	  showDebug(1, "error:%d %s\n", errno, strerror(errno));
	  THROW(ERR_ERRNO, errno);
	}
    }
  else
    THROW(ERR_COMP);

  showDebug(0,"INFO: path=[%s] and file=[%s] and vol=[%d] and fully=[%s]\n", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber(), get_szImageFilename());
  
  // mount image file location
  //if (bInitThread)
    mountImageLocation();

    openWritingFdDisk();

  // MULTI-THREAD INITIALIZATION
  // Run the thread to save the buffer to the image file
  //if (bInitThread)
    {
      g_param.image = this;
      g_param.options = m_options;
      g_param.nFd = nFd[0];
      g_bSpaceOnDisk = true;
      g_nThreadError = 0;

      showDebug(1, "PTHREAD_CREATE: before\n");
      pthread_create(&g_threadBuffer, NULL, (void*(*)(void*))&procWriteBufferToImage, &g_param);
      showDebug(1, "PTHREAD_CREATE: after\n");
    }

  /*showDebug(0,"TRY: path=[%s] and file=[%s] and vol=[%d] and fully=[%s]\n", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber(), get_szImageFilename());
  try { m_cid -> openWriting(); }
  catch ( CExceptions * excep )
    { // we can recover exist, locked or path exceptions
      if (excep->GetExcept() == ERR_EXIST || excep->GetExcept() == ERR_LOCKED) 
	{  
	  if (excep->GetExcept() == ERR_EXIST)
	    nCont = excep->windowAlreadyExist(get_szImageFilename(),get_szPath());
	  else
	    nCont = excep->windowLocked(get_szImageFilename(),get_szPath());
	  switch (nCont)
	    {
	    case ERR_QUIT:
	      THROW(ERR_CANCELED);

	    case ERR_RETRY: 
	      strcpy(szAux, excep->getNewString()); // endding '/' were removed 
	      extractFilepathFromFullPath(szAux, aux2); // filepath without filename
	      set_szPath(aux2);
	      extractFilenameFromFullPath(szAux, aux2); // filename without path
	      set_szOriginalFilename(aux2);
	      SNPRINTF(szAux, "%s/%s.%.3ld", get_szPath(), 
		       get_szOriginalFilename(), get_dwVolumeNumber());
	      set_szImageFilename(szAux);
	      openWriting(false);
	      RETURN;
	      break;

	    case ERR_CONT:
	      if (excep->GetExcept() == ERR_EXIST)
		{
		  m_options.bOverwrite = true;
		  m_cid -> set_options(&m_options);
		}
	      openWriting(false);
	      RETURN;
	      break;
	    }
	}
      else if (excep->GetExcept() == ERR_PATH)
	{
	  nCont = excep->windowWrongPath(get_szImageFilename());
	  switch (nCont)
	    {
            case ERR_QUIT:
              THROW(ERR_CANCELED);
            case ERR_RETRY:
              strcpy(szAux, excep->getNewString()); // endding '/' were removed 
              set_szPath(szAux); 
	      SNPRINTF(aux2, "%s/%s.%.3ld", szAux, get_szOriginalFilename(),
		       get_dwVolumeNumber());
              set_szImageFilename(aux2);
	      
              openWriting(false);
	      RETURN;
            }
	}
      else // other exception -> raise
	throw excep;
    }*/

  // Write volume header
  CVolumeHeader headVolume;
  memset(&headVolume, 0, sizeof(CVolumeHeader));

  // create the identificator if this is the first volume
  if (get_dwVolumeNumber() == 0)
    set_qwIdentificator(generateIdentificator());

  strcpy(headVolume.szMagicString, MAGIC_BEGIN_VOLUME);
  headVolume.dwVolumeNumber = get_dwVolumeNumber();
  headVolume.qwIdentificator = get_qwIdentificator();
  strcpy(headVolume.szVersion, CURRENT_IMAGE_FORMAT);

  write((void*)&headVolume, (DWORD)sizeof(CVolumeHeader), false);

  RETURN;
}

// =======================================================
void CImage::writeCRC(char *cData, DWORD dwDataLen)
{
  DWORD dwCRC;
  DWORD i;

  // calculates the CRC32
  dwCRC = 0;
  for (i=0; i < dwDataLen; i++)
    dwCRC += cData[i];

  try { write(&dwCRC, sizeof(DWORD), true); }
  catch (CExceptions * execp)
    { throw execp; }
}

// =======================================================
void CImage::readAndCheckCRC(char *cData, DWORD dwDataLen)
{
  DWORD dwCurCRC, dwFileCRC;
  DWORD i;

  // calculates the CRC32
  dwCurCRC = 0;
  for (i=0; i < dwDataLen; i++)
    dwCurCRC += cData[i];
  
  try { read((char*)&dwFileCRC, sizeof(DWORD), true); }
  catch (CExceptions * excep)
    {
      showDebug(1, "error in CImage::readAndCheckCRC()\n");
      throw excep; 
    }
  
  if (dwCurCRC != dwFileCRC)
    {
      showDebug(1, "Bad CRC in CImage::readAndCheckCRC(): %lu was read, instead of %lu\n", dwFileCRC, dwCurCRC);
      THROW(ERR_CHECK_CRC, dwFileCRC, dwCurCRC);
    }
}

// =======================================================
// may throw incomplete, errno, opened, exist, access, full, canceled
void CImage::writeMagic(char *szMagicString)
{
  try { write(szMagicString, strlen(szMagicString), true); }
  catch (CExceptions * execp)
    { throw execp; }
}

// =======================================================
// may throw wrongmagic
// may throw opened, eof, errno, canceled, volumeheader, volumenumber,
//           partitionvolume
void CImage::readAndCheckMagic(char *szMagicString)
{
  char szAux [strlen(szMagicString)+1];

  showDebug(2, "begin of ci::readAndCheckMagic\n");

  try { read(szAux, strlen(szMagicString), true); }
  catch (CExceptions * excep)
  {
    showDebug(2, "end of ci::readAndCheckMagic: exceptions raised from ci::read\n");
    throw excep; 
  }

  if (memcmp(szMagicString, szAux, strlen(szMagicString)))
    {
      showDebug(2, "szMagicString=%d, %s\n", strlen(szMagicString), szMagicString);
      szAux[strlen(szMagicString)] = 0;
      showDebug(2, "szAux=[%s]\n", szAux);
      THROW(ERR_WRONGMAGICINVALIMGFILE);
    }

  showDebug(2, "end of ci::readAndCheckMagic: ok\n");
}

// =======================================================
// may throw canceled
// may throw opened, errno, comp (from cid)
// may throw version_older, version_newer
void CImage::openReading(CVolumeHeader *vh /* = NULL */)
{
  int nRes;
  char szMess[2048];
  char szAux[MAXPATHLEN], aux2[MAXPATHLEN];
  
  int nFd[2];
  int nCont;

  showDebug(1, "begin (%s)\n", m_cid->get_szImageFilename());
  showDebug(3, "Initializing new buffer\n");

  // mount image file location
  mountImageLocation();

  // ---- ask for another path while the file is not found
  while (m_cid->doesFileExists(m_cid->get_szImageFilename()) == false)
    {
      if ((m_options.szAutoMount) && (strlen(m_options.szAutoMount)))
        { // Automount enabled -> wrong volume inserted, ask for the right
          // one but don't change location
	  nRes = g_interface -> msgBoxContinueCancel(
             i18n("Wrong volume inserted"),
	     i18n("You will be prompted to remove this media and insert the"
             " right one"));
          umountImageLocation();
          mountImageLocation();
	}
      else
	{
          SNPRINTF(szMess,i18n("Can't read the following volume file:\n%s\n%s"),
	          get_szImageFilename(),
		  i18n("Enter another full path (directory & name)"));
          nRes = g_interface -> askText(szMess, i18n("Volume not found"),
                  szAux, MAXPATHLEN);
          if (nRes == -1)
    	    THROW(ERR_CANCELED);
      
          set_szImageFilename(szAux);
          extractFilepathFromFullPath(get_szImageFilename(), szAux);
          set_szPath(szAux);
          extractFilenameFromFullPath(get_szImageFilename(), szAux);
          set_szOriginalFilename(szAux);
	}
    }

  nRes = pipe(nFd);
  if (nRes == -1)
    {
      showDebug(1, "no more pipes\n");
      exit(0);
    }
  m_nFdImage = nFd[0];

  // get compress level
  m_options.dwCompression = getCompressionLevelForImage(m_cid->get_szImageFilename());
  showDebug(1, "LEVEL [%s]=%d\n", m_cid->get_szImageFilename(), m_options.dwCompression);
  if (m_guiRestore)
    m_guiRestore -> showImageFileInfo(get_szImageFilename(), m_options.dwCompression, m_options.szFullyBatchMode);

  // ---- MULTI-THREAD INITIALIZATION
  // Run the thread which read the buffer from the image file
  g_param.image = this;
  g_param.options = m_options;
  g_param.nFd = nFd[1];
  g_bSpaceOnDisk = true;
  g_nThreadError = 0;
  //g_bFinished = false;

  try{ m_cid -> openReading(); }
  catch (CExceptions * excep)
    {
      if (excep->GetExcept() != ERR_LOCKED)
        throw(excep); // note: don't use THROW witch create a new CExceptions
      nCont = excep -> windowLocked(get_szImageFilename(), get_szPath());    
      switch(nCont)
        {
          case ERR_QUIT:
            THROW(ERR_CANCELED);
          case ERR_CONT:
            umountImageLocation();
            return openReading(vh);
          case ERR_RETRY:
            strcpy(szAux, excep->getNewString());
            extractFilepathFromFullPath(szAux, aux2);
            set_szPath(aux2);
            extractFilenameFromFullPath(szAux, aux2);
            set_szOriginalFilename(aux2);
            SNPRINTF(szAux, "%s/%s.%.3ld", get_szPath(), 
               get_szOriginalFilename(), get_dwVolumeNumber());
            set_szImageFilename(szAux);
            umountImageLocation();
            return openReading(vh);
        }
    }

  showDebug(1, "PTHREAD_CREATE before\n");
  pthread_create(&g_threadBuffer, NULL, (void*(*)(void*))&procReadBufferFromImage, &g_param);
  showDebug(1, "PTHREAD_CREATE after\n");

  // open stream
  if (m_options.dwCompression == COMPRESS_NONE) // No compression
    {
      m_fImageFile = fdopen(m_nFdImage, "rb");
      if (m_fImageFile == NULL)
        THROW(ERR_ERRNO, errno);
      else
        showDebug(1, "no comp open\n");
    }
  else if (m_options.dwCompression == COMPRESS_GZIP) // Gzip compression
    {
      m_gzImageFile = (gzFile *) gzdopen(m_nFdImage, "rb");
      if (m_gzImageFile == NULL)
        THROW(ERR_ERRNO, errno);
      else
        showDebug(1, "gzip open\n");
    }
  else if (m_options.dwCompression == COMPRESS_BZIP2) // Bzip2 compression
    {
      m_bzImageFile = BZ2_bzdopen(m_nFdImage, "rb");
      if (m_bzImageFile == NULL)
        THROW( errno);
      else
        showDebug(1, "bzip2 open\n");
    }
  else
    THROW(ERR_COMP);

  // ---- Check volume header
  CVolumeHeader headVolume;
  memset(&headVolume, 0, sizeof(CVolumeHeader));

  try { read((char*)&headVolume, sizeof(CVolumeHeader), false); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "exception caught from cid::read\n");
      
      if (excep -> GetExcept() == ERR_EOF)
        THROW(ERR_VOLUMEHEADER);
      else
        {
          showDebug(2, "exception raised from cid::read\n");
          throw excep;
        }
    }

  // if the CVolumeInfo struct is required
  if (vh)
    memcpy(vh, &headVolume, sizeof(CVolumeHeader));
	
  if (strcmp(headVolume.szMagicString, MAGIC_BEGIN_VOLUME) != 0)
    {	
      SNPRINTF(szAux, "%s/%s.%.3ld", get_szPath(), get_szOriginalFilename(), get_dwVolumeNumber());
      THROW(ERR_NOTAPARTIMAGEFILE, szAux);
    }

  // ----------- check image version
  showDebug(4, "ImageFile version: [%s] and CURRENT_IMAGE_FORMAT=[%s]\n", headVolume.szVersion, CURRENT_IMAGE_FORMAT);
  if (strcmp(headVolume.szVersion, CURRENT_IMAGE_FORMAT) > 0)
    THROW(ERR_VERSION_NEWER, headVolume.szVersion);
  else if ((strcmp(headVolume.szVersion, CURRENT_IMAGE_FORMAT) < 0) && 
	   (strcmp(headVolume.szVersion, "0.6.0") != 0))
    THROW(ERR_VERSION_OLDER, headVolume.szVersion);

  if (vh) // not the first volume -> image info
    RETURN;

  // create the identificator if this is the first volume
  if (get_dwVolumeNumber() == 0)
    set_qwIdentificator(headVolume.qwIdentificator);
  
  // no error if zero, because this field was null in older versions formats
  // FIXME: error if only one is zero
  if ((get_dwVolumeNumber() > 0) && (headVolume.qwIdentificator != get_qwIdentificator()))
    {
      showDebug(4, "The current volume doesn't fit: currentId=%llu=%llX and"
		" mainId=%llu=%llX\n",	headVolume.qwIdentificator,
		headVolume.qwIdentificator, get_qwIdentificator(),
		get_qwIdentificator());

      THROW(ERR_VOLUMEID);
    }

  if (headVolume.dwVolumeNumber != get_dwVolumeNumber())
    {
      THROW(ERR_VOLUMENUMBER, headVolume.dwVolumeNumber, get_dwVolumeNumber());
    }

  RETURN;
}

// =======================================================
int CImage::getCompressionLevelForImage(char *szFilename)
{
  return m_cid->getCompressionLevelForImage(szFilename);
}

// =======================================================
/*void CImage::setGuiSave(CSavingWindow *gui)
{
  m_guiSave = gui;
}

// =======================================================
void CImage::setGuiRestore(CRestoringWindow *gui)
{
  m_guiRestore = gui;
}*/

