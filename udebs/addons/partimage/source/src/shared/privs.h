/***************************************************************************
                          privs.h  -  description
                             -------------------
    begin                : Tue Jun 28 2001
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

#ifndef _PRIVS_H_
#define _PRIVS_H_

#include <sys/types.h>

class CPrivs
{
public: 
  CPrivs(char * _user);
  void Root();
  void User();
  void ForceUser();
  bool AsSwitched() { return switched; }
private:
  uid_t user;
  bool switched;
};

#endif // _PRIVS_H_
