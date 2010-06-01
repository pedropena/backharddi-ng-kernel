/***************************************************************************
                          interface_none.h  -  description
                             -------------------
    begin                : Sun 6 Apr 2003
    copyright            : (C) 2003 by Ian Jackson
    email                : ian@davenant.greenend.org.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _INTERFACE_NONE_H_
#define _INTERFACE_NONE_H_

#include "partimage.h"
#include "exceptions.h"
#include "interface.h"

// =======================================================
class CInterfaceNone : public CInterface
{
private:
  char *optstring;
  int count;
public:
  CInterfaceNone(char *a_optstring);
  virtual ~CInterfaceNone() {}

  // we specialise everything in CInterface
  void msgBoxOk(char *szTitle, char *szText, ...);
  void msgBoxCancel(char *szTitle, char *szText, ...);
  void msgBoxError(char *szText, ...);
  int msgBoxYesNo(char *szTitle, char *szText, ...);
  int msgBoxContinueCancel(char *szTitle, char *szText, ...);
  void showErrorInternal(char *szText);

  int guiInitMainWindow(char * szDevice, char * szImageFile, char *
			szNetworkIP, DWORD * dwServerPort, bool * bSsl)
    {return 0;}
  
  void askDescription(char * szDescription, DWORD dwMaxLen);
  int askText(char * szMessage, char * szTitle, char * szDestText);
  int askText(char * szMessage, char * szTitle, char * szDestText,
     WORD size);
  int askLogin(char * szLogin, char * szPasswd, WORD size);
  WORD askRestore(char * szDevice, char * szImageFilename);

  void StatusLine(char * str);
  WORD askNewPath(char * szOriginalFilename, DWORD dwVolume,
			  char * szPath, char *szNewPath, WORD size);
private:
  void message_only(char *kind, char *title, char *format,
		    va_list al, char *reslt);
  char *lookup(char *title, char *text, char *deflt);
  void invalid_programmed_response();
  void flusherr();
};

#endif
