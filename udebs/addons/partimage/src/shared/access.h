/***************************************************************************
                          access.h  -  description
                             -------------------
    begin                : Tue Feb 20 2001
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

#ifndef _ACCESS_H_
#define _ACCESS_H_

unsigned int CheckAccess(bool bMustLogin, char * szLogin, char * szPasswd);
char * GetSalt(char * szLogin);
unsigned int CheckAccessFile(char * szFile);

#endif
