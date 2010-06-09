/***************************************************************************
                          partimaged-client.cpp  -  manage clients from server
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

#include "partimaged-client.h"
#include "common.h"


// =======================================================
CPartimagedClients::CPartimagedClients()
{
  showDebug(10, "mutex init for clients\n");
  pthread_mutex_init(&mClients, &mClientsAttr);
}

// =======================================================
CPartimagedClients::~CPartimagedClients()
{
  showDebug(10, "mutex destroy for clients\n");
  pthread_mutex_destroy(&mClients);
}

// =======================================================
unsigned int CPartimagedClients::New()
{
  int next = 0;
  bool found = false;
  while (!found && next < MAX_CLIENTS)
    {
      if (!Clients[next].Present)
        found = true;
      else
        next++;
    }

  Clients[next].Present = true;
  pthread_mutex_unlock(&mClients);
  return found ? next : MAX_CLIENTS;
}

// =======================================================
void CPartimagedClients::Set(unsigned int client, int sock)
{
  pthread_mutex_lock(&mClients);
  Clients[client].Sock = sock;
  Clients[client].Ssl = NULL;
  
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::Set(unsigned int client, int sock,
   struct sockaddr_in sockaddr, void * _ssl)
{
  pthread_mutex_lock(&mClients);

  Clients[client].Sock = sock;
  Clients[client].SockAddr = sockaddr;
#ifdef HAVE_SSL
  Clients[client].Ssl = (SSL *) _ssl;
#else
  Clients[client].Ssl = (char *) _ssl;
#endif
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::Release(unsigned int client)
{
  pthread_mutex_lock(&mClients);
  showDebug(1, "%d released\n", client);
  Clients[client].Present = false;
  pthread_mutex_unlock(&mClients);
}

// =======================================================
#ifdef HAVE_SSL
SSL * CPartimagedClients::Ssl(unsigned int client)
{
  SSL * aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].Ssl;
  pthread_mutex_unlock(&mClients);
  return aux;
}
#else
char * CPartimagedClients::Ssl(unsigned int client)
{
  char * aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].Ssl;
  pthread_mutex_unlock(&mClients);
  return aux;
}
#endif

// =======================================================
int CPartimagedClients::Sock(unsigned int client)
{
  int aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].Sock;
  pthread_mutex_unlock(&mClients);
  return aux;
}

// =======================================================
struct sockaddr_in CPartimagedClients::GetSA(unsigned int client)
{
  struct sockaddr_in aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].SockAddr;
  pthread_mutex_unlock(&mClients);
  return aux;
}

// =======================================================
void CPartimagedClients::SetSock(unsigned int client, int sock)
{
  pthread_mutex_lock(&mClients);
  Clients[client].Sock = sock;
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::SetSsl(unsigned int client, void * ssl)
{
  pthread_mutex_lock(&mClients);
#ifdef HAVE_SSL
  Clients[client].Ssl = (SSL *) ssl;
#else
  Clients[client].Ssl = (char *) ssl;
#endif
  pthread_mutex_unlock(&mClients);
}
