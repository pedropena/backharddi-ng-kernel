/***************************************************************************
                          exceptions.cpp  -  class to handle exceptions
                             -------------------
    begin                : THU JAN 4 2001
    copyright            : (C) 2001 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/
// $Revision: 1.16 $
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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "partimage.h"
#include "exceptions.h"
#include "common.h"

/** Checks if number of parameters matches required number for this exception
 *  @param excep is the exception number (refer to errors.h for the list)
 *  @param nb is the number of parameters got for @param excep exception
 *  required number of parameters is hardcoded in CheckArguments.
 *  Warning: CheckArguments doesn't check argument type
*/
void CExceptions::CheckArguments(int excep, int nb)
{
  switch (excep)
    {
      // list all one argument exceptions here
    case ERR_ERRNO:
    case ERR_READ_BITMAP:
    case ERR_ZEROING:
    case ERR_OPENING_DEVICE:
    case ERR_VERSION_OLDER:
    case ERR_VERSION_NEWER:
    case ERR_NOTAPARTIMAGEFILE:
    case ERR_NOTAREGULARFILE:
    case ERR_LOCKED:
    case ERR_CREATESPACEFILE:
    case ERR_CREATESPACEFILENOSPC:
    case ERR_CREATESPACEFILEDENIED:

      if (nb != 1) 
	showDebug(1, "WARNING: %d with wrong argument number (%d)\n",excep,nb);
      break;
	
      // list all two arguments exceptions here
    case ERR_READING:
    case ERR_WRITING:
    case ERR_CHECK_CRC:
    case ERR_CHECK_NUM:
    case ERR_VOLUMENUMBER:
    case ERR_PART_TOOSMALL:
    case ERR_BLOCKSIZE:

      if (nb != 2)
	showDebug(1, "WARNING: %d with wrong argument number (%d)\n",excep,nb);
      break;
    }

}

/** Basic constructor for class CExceptions.
 * @param msg is a short text writen in debugfile for debugging
 * @param error is the exception number (see errors.h for the list)
 * Use this constructor only if the exception doesn't require any parameter
*/
CExceptions::CExceptions(const char *szMsg, signed int nError)
{
  m_nError = nError;
  m_caught = false;
  CheckArguments(nError, 0);
  if (nError > 0) 
    showDebug(1, "%s -> throws: (%d) %s\n", szMsg, nError, strerror(nError));
  else
    showDebug(1, "%s -> throws: %d\n", szMsg, nError);

  m_szArg1 = NULL;
  m_dwArg1 = m_dwArg2 = 0L;
  m_qwArg1 = m_qwArg2 = 0L;
}

/** Constructor with one DWORD parameter for class CExceptions.
 * @param szMsg is a short text writen in debugfile for debugging
 * @param nError is the exception number (see errors.h for the list)
 * @param dwArg1 is a DWORD parameter for the exception
 * Use this constructor only if the exception requires only a DWORD parameter
*/
CExceptions::CExceptions(const char *szMsg, signed int nError, DWORD dwArg1)
{
  m_nError = nError;
  m_caught = false;
  m_dwArg1 = dwArg1;
  CheckArguments(nError, 1);
  showDebug(1, "%s -> throws (DWORD): %d (%ld)\n", szMsg, nError, dwArg1);
  m_szArg1 = NULL;
  m_dwArg2 = 0L;
}

/** Constructor with two DWORD parameter for class CExceptions.
 * @param msg is a short text writen in debugfile for debugging
 * @param error is the exception number (see errors.h for the list)
 * @param dwArg1 is first DWORD parameter for the exception
 * @param dwArg2 is last DWORD parameter for the exception
 * Use this constructor only if the exception requires only 2 DWORD parameters
*/
CExceptions::CExceptions(const char *szMsg, signed int nError, DWORD dwArg1, DWORD dwArg2)
{
  m_nError = nError;
  m_caught = false;
  m_dwArg1 = dwArg1;
  m_dwArg2 = dwArg2;
  CheckArguments(nError, 2);
  showDebug(1, "%s -> throws (DWORD,DWORD): %d (%lu)(%lu)\n", szMsg, nError, dwArg1, dwArg2);
  m_szArg1 = NULL;
}

/** Constructor with two QWORD parameter for class CExceptions.
 * @param msg is a short text writen in debugfile for debugging
 * @param nError is the exception number (see errors.h for the list)
 * @param qwArg1 is first QWORD parameter for the exception
 * @param qwArg2 is last QWORD parameter for the exception
 * Use this constructor only if the exception requires only 2 DWORD parameters
*/
CExceptions::CExceptions(const char *szMsg, signed int nError, QWORD qwArg1, QWORD qwArg2)
{
  m_nError = nError;
  m_caught = false;
  m_qwArg1 = qwArg1;
  m_qwArg2 = qwArg2;
  CheckArguments(nError, 2);
  showDebug(1, "%s -> throws (QWORD,QWORD): %d (%llu)(%llu)\n", szMsg, nError, qwArg1, qwArg2);
  m_szArg1 = NULL;
}

/** Constructor with a string parameter for class CExceptions.
 * @param msg is a short text writen in debugfile for debugging
 * @param nError is the exception number (see errors.h for the list)
 * @param szArg1 is the string parameter for the exception
 * Use this constructor only if the exception requires only a string parameter
*/
CExceptions::CExceptions(const char *szMsg, signed int nError, char * szArg1)
{
  m_nError = nError;
  m_caught = false;
  m_szArg1 = strdup(szArg1);
  CheckArguments(nError, 1);
  showDebug(1, "%s -> throws (char *): %d (%s)\n", szMsg, nError, szArg1);
  m_dwArg1 = m_dwArg2 = 0L;
}

/** Constructor with a string and a DWORD parameter for class CExceptions.
 * @param szMsg is a short text writen in debugfile for debugging
 * @param nError is the exception number (see errors.h for the list)
 * @param szArg1 is the string parameter for the exception
 * @param dwArg1 is the DWORD parameter for the exception
 * Use this constructor only if the exception requires only a string and a
 * DWORD parameter
*/
CExceptions::CExceptions(const char *szMsg, signed int nError, char * szArg1,
 DWORD dwArg1 )
{
  m_nError = nError;
  m_caught = false;
  m_szArg1 = strdup(szArg1);
  m_dwArg1 = dwArg1;
  CheckArguments(nError, 2);
  showDebug(1, "%s -> throws (char *, DWORD): %d (%s)(%ld)\n", szMsg, nError, szArg1, dwArg1);
  m_dwArg2 = 0L;
}

/** Constructor for exception get from server
 * @param szMsg is a short text writen in debugfile for debugging
 * @param nError is the exception number (see errors.h for the list)
 * @param szArg1 is the string parameter for the exception
 * @param dwArg1 is the DWORD parameter for the exception
 * @param dwArg2 is the DWORD parameter for the exception
*/
CExceptions::CExceptions(const char *szMsg, signed int nError, char * szArg1,
 DWORD dwArg1, DWORD dwArg2 )
{
  m_nError = nError;
  m_caught = false;
  m_szArg1 = strdup(szArg1);
  m_dwArg1 = dwArg1;
  m_dwArg2 = dwArg2;
  showDebug(1, "{from net} %s -> throws: %d (%s)(%ld)\n", szMsg, nError, szArg1,
     dwArg1);
}



CExceptions::~CExceptions()
{
  if (m_szArg1)
    free(m_szArg1);
}

const signed int CExceptions::GetExcept() { return m_nError; }
