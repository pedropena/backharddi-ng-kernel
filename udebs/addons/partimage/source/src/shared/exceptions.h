/***************************************************************************
                          exceptions.h  -  description
                             -------------------
    begin                : THU JAN 4 2001
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


#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#include "gui_text.h"
#include "errors.h"

class CExceptions
{
public:

  CExceptions(const char *szMsg, signed int nError);
  CExceptions(const char *szMsg, signed int nError, DWORD dwArg1);
  CExceptions(const char *szMsg, signed int nError, DWORD dwArg1, DWORD dwArg2);
  CExceptions(const char *szMsg, signed int nError, QWORD qwArg1, QWORD qwArg2);
  CExceptions(const char *szMsg, signed int nError, char * szArg1);
  CExceptions(const char *szMsg, signed int nError, char * szArg1, DWORD dwArg1);

// this constructor should only be called when receiving exception from server
  CExceptions(const char *szMsg, signed int nError, char * szArg1, DWORD dwArg1,
     DWORD dwArg2);

  ~CExceptions();
  const signed int GetExcept();

  char * getNewString()
    { return gui.getNewString(); }
  unsigned int windowAlreadyExist(char * img, char * path)
    { return gui.windowAlreadyExist(img, path); }
  unsigned int windowLocked(char * img, char * path)
    { return gui.windowLocked(img, path); }
  unsigned int windowWrongPath(char * szFilename)
    { return gui.windowWrongPath(szFilename); }

  bool getCaught() { return m_caught; }
  void setCaught() { m_caught = true; }
  DWORD get_dwArg1() { return m_dwArg1; }
  DWORD get_dwArg2() { return m_dwArg2; }
  QWORD get_qwArg1() { showDebug(1, "QWORD1=%llu\n", m_qwArg1); return m_qwArg1; }
  QWORD get_qwArg2() { showDebug(1, "QWORD2=%llu\n", m_qwArg2); return m_qwArg2; }

  char * get_szArg1() { showDebug(1, "STR=%s\n", m_szArg1);return m_szArg1; }

private:
  void CheckArguments(int excep, int nb);
//  unsigned int action(int, char *);
  signed int m_nError;
  CExceptionsGUI gui;
  bool m_caught;
  DWORD m_dwArg1;
  DWORD m_dwArg2;
  QWORD m_qwArg1;
  QWORD m_qwArg2;
  char * m_szArg1;
};

#endif
