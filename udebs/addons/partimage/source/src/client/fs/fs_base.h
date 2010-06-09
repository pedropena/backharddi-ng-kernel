/***************************************************************************
                          fs_base.h  -  description
                             -------------------
    begin                : Sun Mar 04 2001
    copyright            : (C) 2001 by Franck Ladurelle
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

#ifndef _FS_BASE_H_
#define _FS_BASE_H_

#include "partimage.h"
#include "cbitmap.h"
#include "interface_newt.h"

struct CImage;
struct CVolumeHeader;
struct CMainTail;
struct CMainHeader;
struct COptions;
class CSavingWindow;
class CRestoringWindow;

#define ZERO_BLOCK_SIZE 512
#define INFOS_STRUCT_SIZE 16384

// ------------------ checks ---------------------
#define CHECK_FREQUENCY 65536 // check every 65536 bytes

// ================================================
struct CCheck
{
  char cMagic[3]; // must be 'C','H','K'
  DWORD dwCRC; // CRC of the CHECK_FREQUENCY blocks
  QWORD qwPos; // number of the last block written
};

// ================================================
struct CLocalHeader // size must be 16384 (adjust the reserved data)
{
  QWORD qwBlockSize;
  QWORD qwUsedBlocks;
  QWORD qwBlocksCount;
  QWORD qwBitmapSize; // bytes in the bitmap
  QWORD qwBadBlocksCount;
  char szLabel[64];
  
  BYTE cReserved[16280]; // Adjust to fit with total header size
};

// ================================================
class CFSBase
{
public: // defined in CFSBase
  CFSBase(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize);
  virtual ~CFSBase();

  void saveUsedBlocksToImageFile(COptions *options);
  void restoreUsedBlocksToDisk(COptions *options);

  void saveToImage(CImage *image, COptions *options, CSavingWindow *gui);
  void restoreFromImage(CImage *image, COptions *options, CRestoringWindow *gui);

  int getStdInfos(char *szDest, int nMaxLen, bool bShowBlocksInfo);
  int readData(void *cBuffer, QWORD qwOffset, DWORD dwLength);
  int calculateSpaceFromBitmap();
  int checkBitmapInfos();
  void setSuperBlockInfos(BOOL bCheckSuperBlockUsedSize, BOOL bCheckSuperBlockFreeSize, QWORD qwSuperBlockUsedSize, QWORD qwSuperBlockFreeSize, BOOL bCanBeMore=false);

 private:
  QWORD m_qwSuperBlockUsedSize;  
  QWORD m_qwSuperBlockFreeSize; 
  BOOL m_bCheckSuperBlockUsedSize;
  BOOL m_bCheckSuperBlockFreeSize;
  BOOL m_bCanBeMore;

public: // defined in CFSXXXXX
  //static  int detect(BYTE *cBootSect, char *szFileSystem) {return 0;} // 1 if this is a XXX partition, 0 if not, -1 if error
  virtual void printfInformations() = 0;
  virtual void readBitmap(COptions *options)=0;
  virtual void readSuperBlock()=0;
  virtual void fsck()=0;
  virtual void*getInfos()=0;
 
public: //private:
  CImage *m_image;
  CBlocksUseBitmap m_bitmap;
  QWORD m_qwPartSize;

  CSavingWindow *m_guiSave;
  CRestoringWindow *m_guiRestore;

  // ex: "/dev/hdc5"
  char m_szDevice[MAX_DEVICENAMELEN+1];

  CLocalHeader m_header; // general infos (block count, block size, used space, ...)
  
  // File descriptors
  FILE *m_fDeviceFile;
};

#endif // _FS_BASE_H_
