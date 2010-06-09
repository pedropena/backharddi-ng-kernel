/***************************************************************************
                          image_disk.h  -  description
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

#ifndef IMAGE_DISK_H
#define IMAGE_DISK_H

#include "image_base.h"
#include "partimage.h"
#include "errors.h"
#include "exceptions.h"

#include <zlib.h> // gzip compression
#include <bzlib.h> // bzip2 compression
#include <string.h>

// ================================================
class CImageDisk : public CImageBase
{
 public:
  CImageDisk(COptions * options);
  virtual ~CImageDisk();
  
 private:
  QWORD m_qwTotal;
  QWORD m_qwVolSize;
  bool m_bIsOpened;
  char m_szPath[MAXPATHLEN+1];
  char m_szOriginalFilename[MAXPATHLEN+1];
  char m_szImageFilename[MAXPATHLEN+1];
  char m_szSpaceFilename[MAXPATHLEN+1];
  COptions m_options;
  
  int m_nImageFd;
  FILE *m_fImageFile;
  
 public:
  virtual bool  get_bIsOpened()
    { return m_bIsOpened; }
  virtual char * get_szImageFilename()
    { return m_szImageFilename; }
  virtual char * get_szOriginalFilename()
    { return m_szOriginalFilename; }
  virtual char * get_szPath()
    { return m_szPath; }
  virtual void get_qwCurrentVolumeSize(QWORD *qwSize);
  virtual void set_bIsOpened(bool n)
    { m_bIsOpened = n; }
  virtual void set_szPath(char * str)
    { strcpy(m_szPath, str); }
  virtual void set_szImageFilename(char * str)
    { strcpy(m_szImageFilename,str); }
  virtual void set_szOriginalFilename(char * str)
    { strcpy(m_szOriginalFilename,str); }
  virtual void set_szImageFilename(char * str, int n)
    { printf("virtual szIF\n"); strncpy(m_szImageFilename,str, n); 
    printf("fin szIF\n");}
  virtual void set_szOriginalFilename(char * str, int n)
    { strncpy(m_szOriginalFilename,str, n); }
  virtual void set_options(COptions * options)
    { memcpy(&m_options, options, sizeof(COptions)); 
       showDebug(3, "->compression=%ld\n", options->dwCompression); }
  
 public:
  // read/write
  virtual void write(void *buf, DWORD dwLength, int *nRes);
  //virtual void write(void *buf, DWORD dwLength); // return 0 if error, or written bytes
  virtual DWORD read(void *buf, DWORD dwLength); 
  
  // open/close
  virtual void openWriting();
  virtual void openReading();
  virtual void close();
  virtual int lockImage(int flag, int nFd, char *szFilename);
  virtual int mountImageLocation(char *, char *, char *);
  virtual int umountImageLocation(char *);
  
  // misc
  bool doesFileExists(char *szPath);
  virtual void getFreeSpace(QWORD * qw_AvailDiskSpace);
  virtual void getDiskFreeSpaceForFile(QWORD *qwFreeSpace, char *szFilepath);
  virtual void getFileSize(QWORD *qwFileSpace, char *szFilepath);
  QWORD getPartitionSize(const char *);
  virtual void Connect(COptions *, char *, char *, char *, int) {};
  //virtual void get_qwCurVolSize(QWORD *qwFileSize);
  virtual int getCompressionLevelForImage(char *szFilename);

 private:
  int destroySpaceFile();
  int createSpaceFile();
  void unlockImageFile();

};

#endif // IMAGE_DISK_H
