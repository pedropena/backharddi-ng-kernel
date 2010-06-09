/***************************************************************************
                          partimaged.cpp  - thread to handle a client 
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
#include <stdlib.h>
#include <iostream.h>

#include "net.h"
#include "netserver.h"
#include "messages.h"
#include "image_disk.h"
#include "common.h"
#include "pathnames.h"
#include "buffer.h"
#include "partimaged-gui_newt.h"
#include "partimaged.h"

#ifdef MUST_CHEUID
  #include "privs.h"
  extern CPrivs * g_privs;
#endif

extern CNetServer * g_Server;
extern CPartimagedInterface * g_Window;

void * partimaged(void * _s)
{
  bool bAux, bQuit;

#ifdef MUST_CHEUID
  g_privs -> ForceUser();
  showDebug(1, "UID is %d, EUID is %d\n", getuid(), geteuid());
#endif

  char * buf = (char *) malloc(BUFFER_LEN);
  char * szAux, * szMountDevice, * szMountPoint, * szMountFS;
  QWORD qwAux, qwSize = 0LL;
  DWORD dwAux, m_cmd, m_size;
  DWORD dwRes = 0L;
  int n;
  int nClient;
  CMessages * mess; 
  CImageDisk * cid;
  CNetServer * Server = g_Server;
  COptions options;

  bQuit = false; 
  memcpy(&nClient, _s, sizeof(int));

  mess = new CMessages;

  Server->Recv(nClient, &options, sizeof(COptions)); 

  cid = new CImageDisk(&options);

  while (!bQuit)
    {
      g_Window->Status(i18n("waiting ..."));

      n = Server->RecvMsg(nClient, mess);
      if (n != MESSAGE_LEN)
        {
          showDebug(2, "client %d: received %d instead of %d\n", nClient, n,
             MESSAGE_LEN);
          showDebug(2, "client %d: error while receiving message from network\n"             , nClient);
          g_Window->SetState(nClient, i18n("error !"));
          g_Window->SetLocation(nClient, "connexion lost");
          g_Server->Release(nClient);
          pthread_exit(NULL);
        }

      mess->getMessage(&m_cmd, &m_size);
      showDebug(3, "retour recvmsg: %ld %ld\n", m_cmd, m_size);

      switch (m_cmd)
      {
        case MSG_NEWOPTIONS:
          n = Server->Recv(nClient, &options, sizeof(COptions));
          cid -> set_options(&options);
          mess->setMessage(MSG_OK);
          Server->SendMsg(nClient, mess);
          break; 
         
        case MSG_GETSPACE: 
          g_Window->Status(i18n("getspace ..."));
          try { cid -> getFreeSpace(&qwAux); }
          catch ( CExceptions * excep) // errno 
            { 
              Server->SendExcep(nClient, excep);
              break; // jump to next case
            }
          showDebug(3, "return ok for getspace: %lld\n", qwAux);
          mess->setMessage(MSG_OK);
          Server->SendMsg(nClient, mess);
          Server->Send(nClient, (void *)&qwAux, sizeof(QWORD));
          break;

        case MSG_WRITE:
          g_Window->Status(i18n("writing ..."));
          if (m_size > BUFFER_LEN)
            {
              delete g_Window;
	      g_Window = NULL;
              showDebug(1, "buffer overflow : %ld\n", m_size);
              exit(5);
            }
          n = Server->Recv(nClient, buf, m_size); // receive what to write

          {
          int nRes;
          try { cid -> write(buf, n, &nRes); } 
          catch ( CExceptions * excep )
            {
              mess->setMessage(MSG_ERROR, (DWORD) excep->GetExcept()); 
              Server->SendExcep(nClient, excep);
              break;
            }
          mess->setMessage(MSG_OK, (DWORD) nRes); 
          Server->SendMsg(nClient, mess);

          g_Window->SetSize(nClient, qwSize); // update partition size
          qwSize += (QWORD) n;
          }
          break;

        case MSG_OPENW:
          g_Window->Status(i18n("opening for write ..."));

          try { cid -> openWriting(); }
          catch ( CExceptions * excep )
            {
              Server->SendExcep(nClient, excep);
              break;
            }

          mess->setMessage(MSG_OK, 0L);
          Server->SendMsg(nClient, mess);
          g_Window->SetState(nClient, i18n("saving"));
          g_Window->SetLocation(nClient, cid->get_szImageFilename());
          break;

        case MSG_READ:
          g_Window->Status(i18n("reading ...")); 

          if (m_size > BUFFER_LEN)
            {
              delete g_Window;
	      g_Window = NULL;
              showDebug(1, "buffer overflow : %ld\n", m_size);
              exit(5);
            }

          try { dwRes = cid -> read(buf, m_size); } 
          catch ( CExceptions * excep )
            {
              Server->SendExcep(nClient, excep);
              break;
            }

          g_Window->SetSize(nClient, qwSize);
          qwSize += (QWORD) m_size;

          mess->setMessage(MSG_OK, dwRes);
          Server->SendMsg(nClient, mess);
          Server->Send(nClient, (void *)buf, dwRes);
          break;

        case MSG_CLOSE: 
          g_Window->Status(i18n("closing ..."));
          try { cid -> close(); }
          catch ( CExceptions * excep )
            {
              Server->SendExcep(nClient, excep);
              break;
            }
          mess->setMessage(MSG_OK, 0L);
          Server->SendMsg(nClient, mess);
          break;

        case MSG_OPENR:
          g_Window->Status(i18n("opening for read ..."));
          g_Window->SetState(nClient, i18n("restoring"));
          g_Window->SetLocation(nClient, cid->get_szImageFilename());
          try { cid -> openReading(); }
          catch ( CExceptions * excep )
            {
              Server->SendExcep(nClient, excep);
              break;
            }
          mess->setMessage(MSG_OK, 0L);
          Server->SendMsg(nClient, mess);
          break;

        case MSG_DOESFILEEXIST:
          bool b;
          n = Server->Recv(nClient, buf, m_size);
          b = cid -> doesFileExists(buf);
          mess->setMessage(b ? MSG_OK : MSG_ERROR);
          Server->SendMsg(nClient, mess);
          break; 

        case MSG_GETDISKSPACEFILE:
          n = Server->Recv(nClient, buf, m_size);
          cid -> getDiskFreeSpaceForFile(&qwAux, buf);
          Server->Send(nClient, &qwAux, sizeof(QWORD));
          break; 

        case MSG_GET_C: 
          n = Server->Recv(nClient, buf, m_size);
          n = cid -> getCompressionLevelForImage(buf);
          mess->setMessage(MSG_OK, n);
          Server->SendMsg(nClient, mess);
          break; 

        case MSG_GETFILESIZE:
          n = Server->Recv(nClient, buf, m_size);
          cid -> getFileSize(&qwAux, buf);
          Server->Send(nClient, &qwAux, sizeof(QWORD));
          break; 

        case MSG_SET_IO:
          cid -> set_bIsOpened(m_size?true:false);
          break;
        case MSG_SET_P:
          if (m_size > BUFFER_LEN)
            {
              delete g_Window;
	      g_Window = NULL;
              showDebug(1, "buffer overflow : %ld\n", m_size);
              exit(5);
            }
          n = Server->Recv(nClient, buf, m_size);
          cid -> set_szPath(buf);
          break;
        case MSG_SET_IF:
          if (m_size > BUFFER_LEN)
            {
              delete g_Window;
	      g_Window = NULL;
              showDebug(1, "buffer overflow : %ld\n", m_size);
              exit(5);
            }
          n = Server->Recv(nClient, buf, m_size);
          cid -> set_szImageFilename(buf);
          break;
        case MSG_SET_OF:
          if (m_size > BUFFER_LEN)
            {
              delete g_Window;
	      g_Window = NULL;
              showDebug(1, "buffer overflow : %ld\n", m_size);
              exit(5);
            }
          n = Server->Recv(nClient, buf, m_size);
          cid -> set_szOriginalFilename(buf);
          break;

        case MSG_GET_CVS:
          cid -> get_qwCurrentVolumeSize(&qwAux);
          Server->Send(nClient, &qwAux, sizeof(QWORD));
          break; 

        case MSG_GET_IO:
          bAux = cid -> get_bIsOpened();
          Server->Send(nClient, (void *) &bAux, sizeof(bool));
          break;

        case MSG_GET_P:
          szAux = cid -> get_szPath();
          mess->setMessage(MSG_OK, strlen(szAux)+1);
          Server->SendMsg(nClient, mess);
          Server->Send(nClient, (void *) szAux, strlen(szAux)+1);
          break;

        case MSG_GET_IF:
          szAux = cid -> get_szImageFilename();
          mess->setMessage(MSG_OK, strlen(szAux)+1);
          Server->SendMsg(nClient, mess);
          Server->Send(nClient, (void *) szAux, strlen(szAux)+1);
          break;

        case MSG_GET_OF:
          szAux = cid -> get_szOriginalFilename();
          mess->setMessage(MSG_OK, strlen(szAux)+1);
          Server->SendMsg(nClient, mess);
          Server->Send(nClient, (void *) szAux, strlen(szAux)+1);
          break;

        case MSG_QUIT:
          bQuit = true;
          Server->SendMsg(nClient, mess); // ACK
          break;

        case MSG_MOUNTIMAGELOCATION:
// receive device to be mounted
          n = Server->Recv(nClient, buf, m_size);
          if ((unsigned int)n == m_size)
            szMountDevice=strdup(buf);
          else
            {
              szMountDevice = NULL;
              showDebug(1, "inconsistant size (%d instead of %d)\n", n,m_size);
            }
// receive mountpoint
          Server->Recv(nClient, &m_size, sizeof(m_size));
          n = Server->Recv(nClient, buf, m_size);
          if ((unsigned int)n == m_size)
            szMountPoint=strdup(buf);
          else
            {
              szMountPoint = NULL;
              showDebug(1, "inconsistant size (%d instead of %d)\n", n,m_size);
            }
// receive filesystem type
          Server->Recv(nClient, &m_size, sizeof(m_size));
          n = Server->Recv(nClient, buf, m_size);
          if ((unsigned int)n == m_size)
            szMountFS=strdup(buf);
          else
            {
              szMountFS = NULL;
              showDebug(1, "inconsistant size (%d instead of %d)\n", n,m_size);
            }
         
// call CImageDisk mount 
          dwAux = cid -> mountImageLocation(szMountDevice, szMountPoint,
             szMountFS);

// return status
          mess->setMessage(dwAux ? MSG_ERROR : MSG_OK);
          Server->SendMsg(nClient, mess);
          break;
       
        case MSG_UMOUNTIMAGELOCATION:
// receive mointpoint
          n = Server->Recv(nClient, buf, m_size);
          if ((unsigned int)n == m_size)
            szMountPoint=strdup(buf);
          else
            {
              szMountPoint = NULL;
              showDebug(1, "inconsistant size (%d instead of %d)\n", n,m_size);
            }
// call CImageDisk umount 
          dwAux = cid -> umountImageLocation(szMountPoint);

// return status
          mess->setMessage(dwAux ? MSG_ERROR : MSG_OK);
          Server->SendMsg(nClient, mess);
          break;

        default:
          delete g_Window;
	  g_Window = NULL;
          exit(5);
          break;
      }
    }          
  delete mess;
  mess = NULL;
  delete cid;
  cid = NULL;
  g_Window->SetState(nClient, i18n("finished"));
  g_Window->SetLocation(nClient, "");
  g_Window->SetSize(nClient, "");
  g_Server->Release(nClient);
  free(buf);
  pthread_exit(NULL);
}
