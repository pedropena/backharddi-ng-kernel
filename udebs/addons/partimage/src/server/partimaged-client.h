/***************************************************************************
                          partimaged-client.h  -  description
                             -------------------
    begin                : Fri Feb 23 2001
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

#include "net.h"
#include "structures.h"

class CPartimagedClients
{
public:
  CPartimagedClients();
  ~CPartimagedClients();
  unsigned int New();
  void Set(unsigned int client, int sock);
  void Set(unsigned int client, int sock, struct sockaddr_in sockaddr,
     void * _ssl);
  void Release(unsigned int client); 
  CIdefRemote Get(unsigned int client) { return Clients[client]; }
#ifdef HAVE_SSL
  SSL * Ssl(unsigned int client);
#else
  char * Ssl(unsigned int client);
#endif
  int Sock(unsigned int client);
  void SetSock(unsigned int client, int sock);
  void SetSsl(unsigned int client, void * ssl);
  struct sockaddr_in GetSA(unsigned int client);

private:
  pthread_mutex_t mClients;
  pthread_mutexattr_t mClientsAttr;
  CIdefRemote Clients[MAX_CLIENTS+1]; // one more for first client to reject
};
