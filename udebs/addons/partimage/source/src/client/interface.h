/***************************************************************************
                          interface.h  -  description
                             -------------------
    begin                : Wed Mar 14 2001
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

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include "partimage.h"
#include "exceptions.h"
#include "misc.h"

#define MSGBOX_YES           1
#define MSGBOX_NO            2
#define MSGBOX_CONTINUE      3
#define MSGBOX_CANCEL        4

// =======================================================
class CInterface
{
public:
  CInterface(bool bBatchMode) {m_bBatchMode = bBatchMode;} 
  virtual ~CInterface() {} 

  bool getBatchMode() { return m_bBatchMode; }

  // defined in herited classes
  virtual void msgBoxOk(char *szTitle, char *szText, ...)=0;
  virtual void msgBoxCancel(char *szTitle, char *szText, ...)=0;
  virtual void msgBoxError(char *szText, ...)=0;
  virtual int msgBoxYesNo(char *szTitle, char *szText, ...)=0;
  virtual int msgBoxContinueCancel(char *szTitle, char *szText, ...)=0;
  virtual void showErrorInternal(char *szText)=0;

  // defined in the base class
  void showError(signed int nErr, char *szFormat, ...);

  void showAboutDialog();

  void ErrorWriting(DWORD block, signed int err);
/**/  void ErrorWritingHeader(char * header, signed int err);
/**/  void ErrorWritingBitmap(DWORD block, signed int err);
/**/  void ErrorWritingDisk(DWORD block, signed int err);
/**/  void ErrorWritingSuperblock(signed int err);
/**/  void ErrorWritingMBR();
/**/  void ErrorWritingMainHeader();
/**/  void ErrorWritingMainTail();
/**/  void ErrorWritingInfos();

  void ErrorReading(DWORD block, signed int err);
/**/  void ErrorReadingHeader(char * header, signed int err);
  void ErrorReadingBitmap(DWORD block, signed int err);
/**/  void ErrorReadingDisk(DWORD block, signed int err);
  void ErrorReadingSuperblock(signed int err);
/**/  void ErrorReadingDiskBitmap(DWORD block, signed int err);
/**/  void ErrorReadingMBR();
/**/  void ErrorReadingMainTail();
/**/  void ErrorReadingMainHeader();
/**/  void ErrorReadingInfos();
/**/  void ErrorReadingMBRMagic();

/**/  void ErrorNewerRelease();
/**/  void ErrorFileSystem(char *);
/**/  void ErrorDiskFull();
  void ErrorZeroing(DWORD block, signed int err);

/**/  void ErrorDetectingFS(char * szDevice, signed int err);
  void ErrorEncryption();
/**/  void ErrorAskFirstVolume(char * szVolume);
/**/  void ErrorInvalidImagefile(char * szFilename);
  void ErrorTooSmall(QWORD qwOriginalSize, QWORD qwSize);
  
  void ErrorNoMemory();
  void ErrorAccess(char * szImageFile);
  void ErrorNoMBR();
  void ErrorCRC();
  void ErrorCRC(QWORD qwOriginalCRC, QWORD qwCRC);
  void ErrorWrongVolumeNumber(DWORD dwExpectedVolume, DWORD dwVolume);
/**/  void ErrorClosing();
  void ErrorAlreadyLocked(char * szPartition, signed int err);
  void ErrorOpeningPartition(char * szDevice, signed int err);
  void ErrorSavingMBR();
/**/  void ErrorInvalidFS(char * szFs);
/**/  void ErrorLocking(char * szFilename, signed int err);

  virtual void StatusLine(char * str)=0;
  virtual int askLogin(char * szLogin, char * szPasswd, WORD size)=0;
  virtual WORD askNewPath(char * szOriginalFilename, DWORD dwVolume,
     char * szPath, char *szNewPath, WORD size)=0;
  virtual int askText(char * szMessage, char * szTitle, char * szDestText,
     WORD size)=0;
  virtual void askDescription(char * szDescription, DWORD dwMaxLen)=0;
  virtual int guiInitMainWindow(char * szDevice, char * szImageFile, char *
     szNetworkIP, DWORD * dwServerPort, bool * bSsl)=0;
  /*virtual int askText(char * szMessage, char * szTitle, char * szDestText)=0;
  virtual void showAboutDialog()=0;
  virtual WORD windowRoot()=0;
  virtual WORD WaitKeyPressed(char * szOld, char * szNewPath,
     char * szOriginalFilename, DWORD volume)=0;
  virtual WORD askRestore(char * szDevice, char * szImageFilename)=0;
  virtual WORD askIgnoreFSError(char * fsck)=0;*/

//  WORD Error(CExceptions * excep, char * szFilename);

  WORD ErrorLogAsRoot();
  WORD ErrorBugBzip2();
  WORD WaitKeyPressed(char *szOld, char *szNewPath, char *szOriginalFilename, DWORD dwVolume);
  WORD askRestore(char *szDevice, char *szImageFilename);
  WORD askIgnoreFSError(char *szFsck);
  WORD askIgnoreNoFschk(char *szFsck);
  WORD askIgnoreDeniedFschk(char *szFsck);
  void WarnFS(char *szFileSys);
  void WarnFsBeta(char *szFileSys);
  void WarnSimulate();
  WORD WarnRestoreMBR(char * szCurrentDevice, QWORD qwCurrentSize, char * szOriginalDevice, QWORD qwOriginalSize);
  WORD WarnRestoreOtherMBR(char *szCurrentDevice,  char *szOriginalDevice);
  void SuccessRestoringMBR(char *szDevice);
  WORD Error(CExceptions *excep, char *szFilename, char *szDevice=NULL);
  //void Error();

private:
  bool m_bBatchMode;
};

#endif
