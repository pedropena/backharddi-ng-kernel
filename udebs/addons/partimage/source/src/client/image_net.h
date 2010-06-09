/***************************************************************************
                          image_net.h  -  description
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

#ifndef IMAGE_NET_H
#define IMAGE_NET_H

#include "image_base.h"
#include "partimage.h"
#include "errors.h"
#include "messages.h"
#include "netclient.h"

#include <zlib.h> // gzip compression
#include <bzlib.h> // bzip2 compression
#include <pthread.h>

// ================================================
class CImageNet : public CImageBase
{
 public:
  CImageNet(COptions * options, bool bUseSSL);
  virtual ~CImageNet();
  
 private:
  bool m_bIsOpened, m_bIsSplitted;
  char m_szPath[MAXPATHLEN], m_szOriginalFilename[MAXPATHLEN];
  char m_szImageFilename[MAXPATHLEN];
  pthread_mutex_t m_mutexNetwork; // lock the buffer while working inside it
  pthread_mutexattr_t m_mutexNetworkAttr;
  
  FILE * m_fImageFile;
  //gzFile * m_gzImageFile;
  //BZFILE * m_bzImageFile;
  
 private:
  void lock(int n);
  void unlock();

 public:
  virtual bool  get_bIsOpened();
  virtual char * get_szImageFilename();
  virtual char * get_szOriginalFilename();
  virtual char * get_szPath();
  
  virtual void set_bIsOpened(bool n);
  virtual void set_szPath(char * str);
  virtual void set_szImageFilename(char * str);
  virtual void set_szOriginalFilename(char * str);
  virtual void set_szImageFilename(char * str, int n);
  virtual void set_szOriginalFilename(char * str, int n);
  virtual void get_qwCurrentVolumeSize(QWORD *qwSize);
  virtual void set_options(COptions * options);
  
 private:
  CMessages * mess;
  CNetClient * netcl;
  
 public:
  // read/write
  virtual void write(void *vBuf, DWORD dwLength, int *nResult);
  //virtual void write(void *buf, DWORD dwLength); // return 0 if error, or written bytes
  virtual DWORD read(void *buf, DWORD dwLength); // return 0 if error, or read bytes
  
  // open/close
  virtual void openWriting();
  virtual void openReading();
  virtual void close();
  virtual int mountImageLocation(char *, char *, char *);
  virtual int umountImageLocation(char *);
  
  
  // misc
  virtual void getFreeSpace(QWORD * qw_AvailDiskSpace);
  virtual bool doesFileExists(char *szPath);
  virtual void getDiskFreeSpaceForFile(QWORD *qwFreeSpace, char *szFilepath);
  virtual void getFileSize(QWORD *qwFileSpace, char *szFilepath);
  virtual void Connect(COptions *, char *, char *, char *, int);
  //virtual void get_qwCurVolSize(QWORD *qwFileSize);
  virtual int getCompressionLevelForImage(char *szFilename);
  
};

#endif // IMAGE_NET_H
