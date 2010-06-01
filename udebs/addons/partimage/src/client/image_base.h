/***************************************************************************
                          image_base.h  -  description
                             -------------------
    begin                : Sun Nov 5 2000
    copyright            : (C) 2000 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef IMAGE_BASE_H
#define IMAGE_BASE_H

#include "partimage.h"
#include "misc.h"

// ================================================
class CImageBase
{
 public:
  CImageBase() {}
  virtual ~CImageBase() {}
  
  virtual bool  get_bIsOpened()=0;
  virtual char * get_szImageFilename()=0;
  virtual char * get_szOriginalFilename()=0;
  virtual char * get_szPath()=0;
  virtual void set_bIsOpened(bool n)=0;
  virtual void set_szPath(char * str)=0;
  virtual void set_szImageFilename(char * str)=0;
  virtual void set_szOriginalFilename(char * str)=0;
  virtual void set_szImageFilename(char * str, int n)=0;
  virtual void set_szOriginalFilename(char * str, int n)=0;
  virtual void get_qwCurrentVolumeSize(QWORD *qwSize)=0;
  virtual void set_options(COptions * options)=0;
  
  virtual void write(void *buf, DWORD dwLength, int *ndwResult)=0;
  //virtual void write(void *buf, DWORD dwLength)=0; 
  virtual DWORD read(void *buf, DWORD dwLength)=0; 
  
  virtual void openWriting()=0;
  virtual void openReading()=0;
  virtual void close()=0;
  virtual int mountImageLocation(char *, char *, char *)=0;
  virtual int umountImageLocation(char *)=0;
  
  // misc	
  virtual bool doesFileExists(char *szPath)=0;
  virtual void getFreeSpace(QWORD *)=0;
  virtual void getDiskFreeSpaceForFile(QWORD *qwFreeSpace, char *szFilepath)=0;
  virtual void getFileSize(QWORD *qwFileSpace, char *szFilepath)=0;
  virtual void Connect(COptions *, char *, char *, char *, int)=0;
  //virtual void get_qwCurVolSize(QWORD *qwFileSize)=0;
  virtual int getCompressionLevelForImage(char *szFilename)=0;
};

#endif // IMAGE_BASE_H
