/***************************************************************************
                          privs.h  -  class for changing userid to 'partimag'
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif 

#include "common.h"
#include "privs.h"

#ifdef MUST_CHEUID
// get our new uid/euid from /etc/passwd
// if not run as root, we can't change them
CPrivs::CPrivs(char * _user)
{
  struct passwd * password = (struct passwd *) malloc(sizeof(struct passwd));

  switched = false;
  if (getuid()) // we're not root, stay as user
    {
// we can't use showDebug since not yet opened
      user = getuid();
      setuid(user);
      seteuid(user);
    }
  else
    {
      password = getpwnam(_user);
      endpwent();
      
      if (password)
        {
          switched = true;
          user = password -> pw_uid;
          setuid(0);
          seteuid(user);              // we're now _user
        }
      else
        {
// we can't use showDebug since not yet opened
          fprintf(stderr, "Changing to user '%s' failed:\n", _user);
          fprintf(stderr, "impossible to get user '%s' from /etc/passwd.\n",
             _user);
          fprintf(stderr, "You must add '%s' to your password file.\n", _user);
          fprintf(stderr, "See: 'man adduser' or 'man useradd' for help.\n");
          exit(1);
        }
    }
}

// be root 
void CPrivs::Root()
{
  showDebug(4, "Switched to Root\n");
  seteuid(0);
}

// switch to unpriv user
void CPrivs::User()
{
  showDebug(4, "Switched to user\n");
  seteuid(user);
}

// go to unpriv user without way back
void CPrivs::ForceUser()
{
  showDebug(4, "forced unpriv user\n");
  seteuid(0);
  setuid(user);
  seteuid(user);
}

#else // MUST_CHEUID
// how does it work for BSD ?

CPrivs::CPrivs(char * _user) {}
void CPrivs::Root()          {}
void CPrivs::User()          {}
#endif // MUST_CHEUID
