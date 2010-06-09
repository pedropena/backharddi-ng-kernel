/***************************************************************************
                          interface_newt.h  -  description
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

#ifndef _INTERFACE_NEWT_H_
#define _INTERFACE_NEWT_H_

#include "partimage.h"
#include "misc.h"
#include "interface.h"

// =======================================================
/** Shows an error dialog box, in a newt window. The title is ¨Error¨ and the 
 *  button is ¨Cancel¨. The parameters are the same as the printf function.
 *  @param fmt format of the text to show (as printf)
 *  @param ... parameters of the text to show (as printf)
 */ 
void debugWin(char *fmt, ...);

// =======================================================
class CInterfaceNewt : public CInterface
{
public:
  CInterfaceNewt(bool bBatchMode);
  ~CInterfaceNewt();

  void showErrorInternal(char *szText);

  virtual void msgBoxOk(char *szTitle, char *szText, ...);
  virtual int msgBoxYesNo(char *szTitle, char *szText, ...);
  virtual void msgBoxCancel(char *szTitle, char *szText, ...);
  virtual void msgBoxError(char *szText, ...);
  virtual int msgBoxContinueCancel(char *szTitle, char *szText, ...);
  
  void StatusLine(char * str);
  int guiInitMainWindow(char * szDevice, char * szImageFile, char *szNetworkIP, DWORD * dwServerPort, bool * bSsl);
  void askDescription(char * szDescription, DWORD dwMaxLen);
  int askText(char *szMessage, char *szTitle, char *szDestText, WORD size);
  int askLogin(char *szLogin, char *szPasswd, WORD size);
  WORD askNewPath(char * szOriginalFilename, DWORD dwVolume, char * szPath, char *szNewPath, WORD size);
  //int askText(char *szMessage, char *szTitle, char *szDestText);
  
  //WORD ErrorLogAsRoot();
  //WORD WaitKeyPressed(char * szOld, char * szNewPath, char * szOriginalFilename, DWORD volume);
  //WORD askRestore(char * szDevice, char * szImageFilename);
  //WORD askIgnoreFSError(char * fsck);
  //void WarnFS(char *szFileSys);
  //void WarnSimulate();
  //WORD WarnRestoreMBR(char * szCurrentDevice, QWORD qwCurrentSize,char * szOriginalDevice, QWORD qwOriginalSize);
  //WORD WarnRestoreOtherMBR(char *szCurrentDevice, char *szOriginalDevice);
  //void SuccessRestoringMBR(char * szDevice);
  //WORD Error(CExceptions *excep, char *szFilename, char *szDevice=NULL);
  
  WORD ErrorExist(char *szImagefile);
  void ErrorAccess(char *szImagefile);
  void ErrorCompressionLevel(char *szImagefile);
  void ErrorAlreadyOpened(char *szImagefile);

  void ErrorPartitionVolume(char *szImagefile); 
  void ErrorVolumeID();
  void ErrorPasswd();
  void ErrorTooMany();
  void ErrorBlockNumber();
  void ErrorVolumeNumber(DWORD dwVol1, DWORD dwVol2);
  void ErrorCheckFS();
  void ErrorRefused();

private:
  bool m_bPushDone;
};

#endif
