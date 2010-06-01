/***************************************************************************
                          net.cpp  -  basic class for network support
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

#include "net.h"
#include "pathnames.h"

size_t CNet::nSend(CIdefRemote remote, const void * buf, size_t len)
{
  static DWORD nb = 0L;

  size_t res;
  showDebug(3, "begin # %ld for %d bytes \n", nb, len);

#ifdef HAVE_SSL
  showDebug(3, "send avec ssl\n");
  if (remote.Ssl)
    res = SSL_write(remote.Ssl, (char *) buf, len);
  else
#endif
    res = send(remote.Sock, buf, len, MSG_NOSIGNAL); 

  if (res != len)
    showDebug(1, "end: %s\n", strerror(errno));
  showDebug(3, "end ret=%d\n", res);
  nb++;
  return res;
}

  
size_t CNet::nRecv(CIdefRemote remote, void * buf, size_t len)
{
  unsigned int res;
  static DWORD nb = 0L;
  showDebug(3, "begin # %ld for %d\n", nb, len);

  
#ifdef HAVE_SSL
  if (remote.Ssl)
    res = SSL_read(remote.Ssl, (char *) buf, len);
  else
#endif
    res = recv(remote.Sock, buf, len, MSG_WAITALL);

  if ((signed int)res == -1)
    showDebug(1, "end %s\n", strerror(errno));
  else if (res < len)
    showDebug(1, "received %d bytes instead of %d\n", res, len);
  
  showDebug(3, "end nb=%d\n", res);
  nb++;
  return (size_t) res;
}

  
size_t CNet::nRecvMsg(CIdefRemote remote, CMessages * msg)
{
  size_t res;
  void * buf;
  showDebug(3, "begin (*)\n");
  buf = malloc (MESSAGE_LEN);

  res = nRecv(remote, buf, MESSAGE_LEN);

  if (res < sizeof(CMessages))
    showDebug(1, "incomplete message received\n");

  msg->fromString((char *) buf);
  free(buf);
  return res;
}


size_t CNet::nSendMsg(CIdefRemote remote, CMessages * msg)
{
  size_t len;

  len = nSend(remote, msg->toString(), MESSAGE_LEN);

  if (len != sizeof(CMessages))
    showDebug(1, "(*) incomplete message sent (%d instead of %d)\n",
       len, sizeof(CMessages));
  showDebug(3, "end (*)\n");
  return len;
}


char * CNet::Banner(bool bUseSSL, bool bMustLogin)
{
  char * szBanner = (char *) malloc (BANNER_SIZE+1);
  memset(szBanner, ' ', BANNER_SIZE);
  *(szBanner+BANNER_SIZE) = '\0';
  strcpy(szBanner, PACKAGE_VERSION);
#ifdef HAVE_SSL
  if (bUseSSL)
    { strcat(szBanner, " SSL"); }
#endif
#ifdef MUST_LOGIN
  if (bMustLogin)
    { strcat(szBanner, " LOG"); }
#endif

  showDebug(1, "generated banner: %s\n", szBanner);
  return szBanner;
}
  
