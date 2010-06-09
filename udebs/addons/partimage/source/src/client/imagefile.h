/***************************************************************************
                          imagefile.h  -  description
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

#ifndef IMAGEFILE_H
#define IMAGEFILE_H

#include "partimage.h"
#include "image_disk.h"
#include "image_net.h"
#include "buffer.h"

class CSavingWindow;
class CRestoringWindow;
class CVolumeHeader;

// ================================================
class CImage
{
 private:
  CImageBase *m_cid;
  CSavingWindow *m_guiSave;
  CRestoringWindow *m_guiRestore;
  //CMTBuffer m_bufferMT;
  QWORD m_qwCRC;
  QWORD m_qwIdentificator;
  DWORD m_dwVolumeNumber;
  COptions m_options;

  FILE *m_fImageFile;
  gzFile *m_gzImageFile;
  BZFILE *m_bzImageFile;

  int m_nFdImage;

  // auto-mount options
  bool m_bIsMounted;
  char m_szMountDevice[MAXPATHLEN];
  char m_szMountPoint[MAXPATHLEN];
  char m_szMountFS[MAXPATHLEN];

 public:
  CImage(COptions * options); 
  ~CImage();
  
  QWORD get_qwIdentificator() {return m_qwIdentificator;}
  QWORD get_qwCurrentVolumeSize();
  DWORD get_dwVolumeNumber();
  bool  get_bIsOpened();
  char * get_szImageFilename();
  char * get_szOriginalFilename();
  char * get_szPath();
  QWORD get_qwCRC() { return m_qwCRC; }
  //CMTBuffer *getMTBuffer() {return &m_bufferMT;}
  CImageBase *getImageDisk() {return m_cid;}
  
  int openWritingFdDisk();

  void set_dwVolumeNumber(DWORD n);
  void set_bIsOpened(bool n);
  void set_szPath(char * str);
  void set_szImageFilename(char * str);
  void set_szOriginalFilename(char * str);
  void set_szImageFilename(char * str, int n);
  void set_szOriginalFilename(char * str, int n);
  void set_qwCRC(QWORD n) {m_qwCRC = n;}
  void set_qwIdentificator(QWORD n);
  void set_options(COptions * options);

 public: // read/write
  void write(void *buf, DWORD dwLength, bool bUpdateCRC); // return 0 if error, or written bytes
  void read(char *buf, DWORD dwLength, bool bUpdateCRC);

  void writeSplit();
  int getCompressionLevelForImage(char *szFilename);
  
  // open/close  
  void openWriting(/*bool bInitThread = true*/);
  void openReading(CVolumeHeader *vh = NULL);
  void closeReading(bool bForceExit = false);
  void closeWriting();

  // magic strings
  void readAndCheckMagic(char *szMagicString);
  void writeMagic(char *szMagicString);
  
  // CRCs
  void writeCRC(char *cData, DWORD dwDataLen);
  void readAndCheckCRC(char *cData, DWORD dwDataLen);

  // GUI
  void setGuiSave(CSavingWindow *gui) {m_guiSave = gui;}
  void setGuiRestore(CRestoringWindow *gui) {m_guiRestore = gui;}
  
 private:
  void splitChangeImageFileWrite();
  void splitChangeImageFileRead();

  int mountImageLocation();
  int umountImageLocation();    

};

#endif // IMAGEFILE_H
