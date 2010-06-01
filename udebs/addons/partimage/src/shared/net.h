/***************************************************************************
                          net.h  -  description
                             -------------------
    begin                : Thu Oct 26 2000
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


#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream.h>
#include <pthread.h>

#include "pathnames.h"
#include "messages.h"
#include "common.h"
#include "../server/partimaged.h"

#include "structures.h"

#ifndef _NET_H_
#define _NET_H_

// banner format is "version( capability)+\0" 
// for now, banner is in form "0.5.7 SSL LOG                   "
// true banner depends on configure options --disable-ssl, --disable-login
#define BANNER_SIZE 32

#ifdef HAVE_SSL
  #include <openssl/rsa.h>
  #include <openssl/crypto.h>
  #include <openssl/x509.h>
  #include <openssl/pem.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#endif

class CNet
{
public:
  CNet()  {}
  ~CNet() {}

  size_t nSend(CIdefRemote remote, const void * buf, size_t len);
  size_t nRecv(CIdefRemote remote, void * buf, size_t len);

  size_t nSendMsg(CIdefRemote remote, CMessages * msg);
  size_t nRecvMsg(CIdefRemote remote, CMessages * msg);

  size_t nSendStr(CIdefRemote remote, const char * str);
  size_t nRecvStr(CIdefRemote remote, const char * str, size_t len);

  char * Banner(bool bUseSSL, bool bMustLogin);
};  

#endif // _NET_H_
