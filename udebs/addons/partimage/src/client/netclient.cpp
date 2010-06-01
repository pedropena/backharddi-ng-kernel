/***************************************************************************
                          netclient.cpp  -  description
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

#ifdef MUST_LOGIN
  #include <crypt.h>
#endif

#include "netclient.h"
#include "exceptions.h"
#include "common.h"

// ================================================

CNetClient::CNetClient(bool bUseSSL):CNet()
{
  showDebug(9, "begin\n");
  int sock;

  if (!(sock = socket(AF_INET, SOCK_STREAM, 0)))
    {
      showDebug(1, "socket error\n");
      THROW(ERR_ERRNO, errno);
    }
  memset(&IdfServer.SockAddr, '\0', sizeof(struct sockaddr));
  IdfServer.Sock = sock;

#ifdef HAVE_SSL
  if (bUseSSL)
    {
      showDebug(3, "initializing client ssl\n");
      SSLeay_add_ssl_algorithms();
      meth = SSLv2_client_method();
      SSL_load_error_strings();
      ctx = SSL_CTX_new(meth);
      if (!ctx)
        THROW(ERR_SSL_CTX);
      m_bUseSSL = (ctx != NULL);
    }
  else
    m_bUseSSL = false;
#else
  m_bUseSSL = false;
#endif

  showDebug(9, "end constructor ok\n");
}      

void CNetClient::Connect(char * adr, unsigned short int port)
{
  BEGIN;
  char * ok = (char *) malloc(5);
  char * szServerBanner = (char *) malloc (BANNER_SIZE+1);
  char * szBanner;
  char * explaination = (char *) malloc(8);
  struct hostent * host;

  if ((host = gethostbyname(adr)) == NULL)
  {
      showDebug(2, "connect error, gethostbyname() -> h_errno: %d\n", h_errno);
      /* FIXME: Perhaps we should provide the user with 
	 the details of h_errno */
      THROW(ERR_HOST);
  }

  IdfServer.SockAddr.sin_family = AF_INET;
  IdfServer.SockAddr.sin_addr.s_addr = *(unsigned long *)host->h_addr;
  IdfServer.SockAddr.sin_port = htons(port);

  if (connect(IdfServer.Sock, (struct sockaddr *) &IdfServer.SockAddr,
     sizeof(IdfServer.SockAddr)))
    {
      showDebug(2, "connect error: %d\n", errno);
      THROW(ERR_ERRNO, errno);
    }

  showDebug(2, "connected, waitting for banner\n");

  IdfServer.Ssl = NULL; // desactive SSL until server allows it
  Recv(szServerBanner, BANNER_SIZE+1); // +1 -> preserve endding \0
  showDebug(2, "Banner received: %s\n", szServerBanner);

#ifdef MUST_LOGIN
  szBanner = Banner(m_bUseSSL, true);
#else
  szBanner = Banner(m_bUseSSL, false);
#endif

  Send(szBanner, BANNER_SIZE+1);
  showDebug(2, "Banner sent: %s\n", szBanner);

  Recv(ok, 5);
  if (strcmp(ok, " ack") && strcmp(ok, "nack"))
    showDebug(2, "Wrong answer from server: %s\n", ok);

  showDebug(3, "ok=%s\n", ok);
  if (strcmp(ok, " ack"))
    {
      Recv(explaination, 8);
      if (!strcmp(explaination, "banners"))
        THROW(ERR_REFUSED);
      else if (!strcmp(explaination, "toomany"))
        THROW(ERR_TOOMANY);
      else
        THROW(ERR_VERSIONSMISMATCH);
    }

#ifdef HAVE_SSL

  if (m_bUseSSL && strstr(szServerBanner, "SSL"))
    {
      int err;
      showDebug(1, "trying to use SSL as required\n");

      IdfServer.Ssl = SSL_new(ctx);
      SSL_set_fd(IdfServer.Ssl, IdfServer.Sock);
      err = SSL_connect(IdfServer.Ssl);
      if (err == -1)
        {
          showDebug(1, "error ssl connect\n");
          THROW(ERR_SSL_CONNECT);
        }
      showDebug(3, "ssl cipher: %s\n", SSL_get_cipher(IdfServer.Ssl)); 
    }
  else
    {
      IdfServer.Ssl = NULL;
      showDebug(1, "no SSL because partimaged doesn't support it\n");
    }
#else
  showDebug(1, "not compiled with SSL\n"); 
  IdfServer.Ssl = NULL;
#endif

  showDebug(3, "connected to server\n");
}

// return 0 if pass good else !0
// login/pass may be "nologin"/"nopass" if compiled with no login
unsigned int CNetClient::SendPass(char * login, char * pass)
{
  int nRes;
  unsigned int nSize;
  char * szSalt;
  char * szPasswd;
  char * ok = (char *) malloc(5);

// send login
  nSize = strlen(login)+1;
  nRes = Send(&nSize, sizeof(unsigned int));
  nRes = Send((void *) login, nSize);

  nRes = Recv(&nSize, sizeof(unsigned int));
  szSalt = (char *) malloc (nSize);
  nRes = Recv(szSalt, nSize); 

#ifdef MUST_LOGIN
  #ifdef HAVE_PAM
    szPasswd = strdup(pass);
  #else     
    szPasswd = crypt(pass, szSalt);
  #endif
#else
  szPasswd = strdup(pass);
#endif

// send password
  nSize = strlen(szPasswd)+1;
  nRes = Send(&nSize, sizeof(unsigned int));
  nRes = Send((void *) szPasswd, nSize);

// check answer
  nRes = Recv(ok, 4+1); // preserve endding \0
  *(ok+4) = '\0';
  return strcmp(ok, " ack");
}

CExceptions * CNetClient::RecvExcep(char * msg)
{
  DWORD dwArg1, dwArg2, dwErrCode, dwAux;
  char * szArg1;

  Recv(&dwErrCode, sizeof(dwErrCode));
  Recv(&dwArg1, sizeof(dwArg1));
  Recv(&dwArg2, sizeof(dwArg2));
  Recv(&dwAux, sizeof(dwAux)); // strlen of szArg1
  szArg1 = (char *) malloc(dwAux);
  Recv(szArg1, dwAux);

  return new CExceptions(msg, (signed int) dwErrCode, szArg1, dwArg1, dwArg2);
}
