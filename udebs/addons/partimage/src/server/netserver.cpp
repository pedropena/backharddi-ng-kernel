/***************************************************************************
                          netserver.cpp  -  class to handle server net part
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

#include <netdb.h>

#include "pathnames.h"
#include "netserver.h"
#include "access.h"
#include "exceptions.h"

extern bool g_bMustLogin;

// ================================================
CNetServer::CNetServer(unsigned short int port):CNet()
{
  int option = 1;
  BEGIN;

  Clients = new CPartimagedClients;

#ifdef HAVE_SSL
  ctx = NULL;
  SSL_load_error_strings();
  SSLeay_add_ssl_algorithms();
  meth = SSLv23_server_method();
  ctx = SSL_CTX_new(meth);
  if (!ctx)
    {
      ERR_print_errors_fp(stderr);
      THROW(ERR_SSL_CTX);
    }
 
  if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0)
    {
      ERR_print_errors_fp(stderr);
      THROW(ERR_SSL_LOADCERT);
    }

  if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0)
    {
      ERR_print_errors_fp(stderr);
      THROW(ERR_SSL_LOADKEY);
    }

  if (!SSL_CTX_check_private_key(ctx))
    {
      fprintf(stderr,
         "private key does not match the certificate public key\n");
      THROW(ERR_SSL_PRIVKEY);
    }
#endif


  if (!(sock_listen = socket(AF_INET, SOCK_STREAM, 0)))
    {
      showDebug(1, "socket error %d %s\n", errno, strerror(errno));
      THROW(ERR_ERRNO, errno); 
    }
  memset(&local_sa, '\0', sizeof(struct sockaddr));

  /* we want to reuse the (addr,port) -> SO_REUSEADDR */
  setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
  local_sa.sin_family = AF_INET;
  local_sa.sin_addr.s_addr = INADDR_ANY;
  local_sa.sin_port = htons(port);

  if (bind(sock_listen, (struct sockaddr *) &local_sa, sizeof(struct sockaddr_in)))
    {
      showDebug(1, "bind error %s\n", strerror(errno));
      THROW(ERR_ERRNO, errno); 
    }

  if (listen(sock_listen, BACKLOG_SIZE))
    {
      showDebug(1, "listen error %s\n", strerror(errno));
      THROW(ERR_ERRNO, errno); 
    }
  showDebug(9, "end\n");
}      

// ================================================
CNetServer::~CNetServer()
{
  showDebug(1, "cns detroyed\n");
  delete Clients;
/*
#ifdef HAVE_SSL
  for (unsigned int i = 0; i < nb_clients; i++)
    if (idf_clients[i].ssl)
      SSL_free(idf_clients[i].ssl);
  if (ctx)
    SSL_CTX_free(ctx);
#endif
*/
}

// ================================================
unsigned int CNetServer::AcceptClient()
{
  socklen_t len = sizeof(struct sockaddr_in);
  int sock;
  unsigned int client;
  char * szBanner;
  char * szClientBanner = (char *) malloc(BANNER_SIZE+1);
  char * pos;
  int n;

  BEGIN;

  sock = accept(sock_listen, (struct sockaddr *) &client_sa, &len);
  if (sock == -1)
    {
      perror("CNetServer::AcceptClient: accept error");
      THROW(ERR_ERRNO, errno);
    }

  client = Clients->New();

  Clients->Set(client, sock);

#ifdef MUST_LOGIN
  if (g_bMustLogin)
    szBanner = Banner(true, true); 
  else
    szBanner = Banner(true, false); 
#else
  szBanner = Banner(true, false); 
#endif
  showDebug(1, "Banner: %s\n", szBanner);

  Send(client, szBanner, BANNER_SIZE+1); // preserve endding \0
  Recv(client, szClientBanner, BANNER_SIZE+1);
  *(szClientBanner+BANNER_SIZE) = '\0';
  showDebug(1, "received banner for client %d: %s\n", client, szClientBanner);

  if (strcmp(szBanner, szClientBanner))
    { // banners mismatch, client can't log on
      showDebug(3, "wrong banner received from %s:%d\n", 
         inet_ntoa(client_sa.sin_addr), ntohs(client_sa.sin_port));
      Send(client, "nack", 5);

// return reason of refuse
      pos = strchr(szBanner, ' ');
      if (!pos)
        Send(client, "1234567", 8); // useless but must be 8 caracters long
      n = (pos-szBanner);
      if (strncmp(szBanner, szClientBanner, n))
        Send(client, "version", 8);
      else
        Send(client, "banners", 8);
      Release(client);
      free(szBanner);
      free(szClientBanner);
      szBanner = NULL;
      szClientBanner = NULL;
      THROW(ERR_REFUSED);
    }

  if (client == MAX_CLIENTS)
    { // partimaged support only MAX_CLIENTS at same time, user has to wait
      Send(client, "nack", 5);
      Send(client, "toomany", 8); 
      free(szBanner);
      free(szClientBanner);
      szBanner = NULL;
      szClientBanner = NULL;
      THROW(ERR_TOOMANY);
    }
  
// client can log
  Send(client, " ack", 5);

#ifdef HAVE_SSL
  SSL * ssl;
  showDebug(3, "switching to SSL\n");
  
  ssl = SSL_new (ctx);

  SSL_set_fd(ssl, sock);
  err = SSL_accept(ssl);
  if (err == -1)
    {
      showDebug(2, "error for client %d: %s\n", client,
         ERR_error_string(ERR_peek_error(), NULL));
      SSL_free(ssl);
      ssl = NULL;
    }
#else
  showDebug(1, "SSL not used\n");
  char * ssl = NULL;
#endif

  showDebug(3, "connexion from %s:%d\n", 
     inet_ntoa(client_sa.sin_addr), ntohs(client_sa.sin_port));

  Clients->Set(client, sock, client_sa, ssl); 

  free(szBanner);
  free(szClientBanner);
  szBanner = NULL;
  szClientBanner = NULL;
  showDebug(9, "end\n");
  return client;
}

void CNetServer::Release(unsigned int client)
{
  Clients->Release(client);
}

void CNetServer::SendExcep(unsigned int client, CExceptions * excep)
{
  DWORD dwAux;
  char * szAux;
  CMessages * mess = new CMessages;

  showDebug(1, "send except to client %d: [%ld %ld %s]\n", client,
    excep->get_dwArg1(), excep->get_dwArg2(), excep->get_szArg1());

// send error && error code 
  mess -> setMessage(MSG_ERROR, (DWORD) excep->GetExcept());
  SendMsg(client, mess);

// send errorcode
  dwAux = excep->GetExcept();
  Send(client, &dwAux, sizeof(dwAux));

// send dwArg1
  dwAux = excep->get_dwArg1();
  Send(client, &dwAux, sizeof(dwAux));
// send dwArg2
  dwAux = excep->get_dwArg2();
  Send(client, &dwAux, sizeof(dwAux));
// send szArg1
  szAux = excep->get_szArg1();
  if (!szAux)
    szAux = "";
  dwAux = strlen(szAux)+1;
  Send(client, &dwAux, sizeof(dwAux));
  Send(client, szAux, dwAux);
  delete mess;
}

char * CNetServer::GetPeer(unsigned int client)
{
  char * szAux = (char *) malloc(15+7); //aaa.aaa.aaa.aaa:xxxxx\0
  snprintf(szAux, 22, "%s:%d",inet_ntoa(Clients->GetSA(client).sin_addr),
     ntohs(Clients->GetSA(client).sin_port));
  *(szAux+21) = '\0';

  return szAux;
}

// return 0 on success
unsigned int CNetServer::ValidatePass(unsigned int client)
{
  int nRes;
  unsigned int nSize;
  char * szLogin;
  char * szPass;
  char * szSalt;

// receive login 
  nRes = Recv(client, &nSize, sizeof(unsigned int)); 
  szLogin = (char *) malloc (nSize);
  nRes = Recv(client, szLogin, nSize); 

  showDebug(2, "client %d is %s\n", client, szLogin);

  szSalt = GetSalt(szLogin);
  if (!szSalt)
    szSalt = strdup("");
  nSize = strlen(szSalt)+1;

  nRes = Send(client, &nSize, sizeof(unsigned int));
  nRes = Send(client, szSalt, nSize);

// receive password
  nRes = Recv(client, &nSize, sizeof(unsigned int)); 
  szPass = (char *) malloc (nSize);
  nRes = Recv(client, szPass, nSize); 

  nRes = CheckAccess(g_bMustLogin, szLogin, szPass);
  showDebug(1, "return of checkaccess: %d\n", nRes);
  if (nRes)
    {
      Send(client, "nack", 5);
      Release(client);
    }
  else
    Send(client, " ack", 5);

  free(szPass);
  szPass = NULL;
  free(szLogin);
  szLogin = NULL;
  free(szSalt);
  szSalt = NULL;
  return nRes;
}
  
