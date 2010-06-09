/***************************************************************************
                          structures.h  -  description
                             -------------------
    begin                : Tue May 1 2001
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

#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_SSL
  #include <openssl/rsa.h>
  #include <openssl/crypto.h>
  #include <openssl/x509.h>
  #include <openssl/pem.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#endif

class CIdefRemote
{
public:   
  int Sock;
  struct sockaddr_in SockAddr;
  bool Present;
#ifdef HAVE_SSL
  SSL * Ssl;
#else
  char * Ssl;
#endif
};

#endif // _STRUCTURES_H_
