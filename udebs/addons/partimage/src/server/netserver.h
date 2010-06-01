/***************************************************************************
                          netserver.h  -  description
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

#ifndef _NETSERVER_H_
#define _NETSERVER_H_

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "net.h"
#include "partimaged-client.h"
#include "exceptions.h"

#define BACKLOG_SIZE 5 


// ================================================
class CNetServer : public CNet
{
private:
  int sock_listen;
  struct sockaddr_in local_sa;
  struct sockaddr_in client_sa;
  CPartimagedClients * Clients;

#ifdef HAVE_SSL
  SSL_CTX * ctx;
  X509 * client_cert;
  SSL_METHOD * meth; 
  int err;
#endif

public:
  CNetServer(unsigned short int port);
  ~CNetServer();
  unsigned int AcceptClient();
  char * GetPeer(unsigned int client);
  void Release(unsigned int client);
  unsigned int ValidatePass(unsigned int client);
  size_t RecvMsg(unsigned int client, CMessages * msg)
    { return nRecvMsg(Clients->Get(client), msg); }
  size_t SendMsg(unsigned int client, CMessages * msg)
    { return nSendMsg(Clients->Get(client), msg); }
  size_t Send(unsigned int client, const void * buf, size_t len)
//    { return nSend(Clients->Get(client), buf, len); }
    {
      CIdefRemote aux;
      showDebug(1, "before GET: %d\n", client);
      aux = Clients->Get(client);
      showDebug(1, "after GET: %d\n", client);
      return nSend(aux, buf, len);
    }
       
  size_t Recv(unsigned int client, void * buf, size_t len)
    { return nRecv(Clients->Get(client), buf, len); }

  void SendExcep(unsigned int client, CExceptions * excep);

};

#endif // _NETSERVER_H_
