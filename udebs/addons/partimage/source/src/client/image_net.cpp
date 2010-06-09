/***************************************************************************
                          image_net.cpp  -  description
                             -------------------
    begin                : Mon Aug 14 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
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

#include "image_net.h"
#include "common.h"
#include "gui_text.h"
#include "exceptions.h"

// =======================================================
CImageNet::CImageNet(COptions * options, bool bUseSSL):CImageBase()
{
  mess = new CMessages;
  netcl = new CNetClient(bUseSSL);

  // multi-thread
  pthread_mutex_init(&m_mutexNetwork, &m_mutexNetworkAttr);
}

// =======================================================
CImageNet::~CImageNet()
{
  lock(__LINE__);
  mess -> setMessage(MSG_QUIT);
  netcl -> SendMsg(mess);
  netcl -> RecvMsg(mess);
  unlock();

  // multi-thread
  pthread_mutex_destroy(&m_mutexNetwork);

  delete mess;
  delete netcl;
}

// =======================================================
void CImageNet::lock(int n) // Multi-thread -> avoid problems
{
  showDebug(5, "[#%d] required mutex\n", n);
  pthread_mutex_lock(&m_mutexNetwork);
  showDebug(5, "got mutex\n");
}

// =======================================================
void CImageNet::unlock() // Multi-thread -> avoid problems
{
  showDebug(5, "release mutex\n");
  pthread_mutex_unlock(&m_mutexNetwork);
}

// =======================================================
void CImageNet::Connect(COptions * options, char * login, char * passwd,
   char * server, int port)
{
  int nRes;
  lock(__LINE__);
  try { netcl -> Connect(server, port); }
  catch ( CExceptions * excep )
    {
      unlock();
      throw excep;
    }
  nRes = netcl -> SendPass(login, passwd);
  if (nRes)
    {
      unlock();
      showDebug(1, "return from SendPass: failed (%d)\n", nRes);
      THROW(ERR_PASSWD);
    }
  netcl -> Send(options, sizeof(COptions));
  unlock();
  showDebug(9, "return from SendPass: success\n");
}

// =======================================================
void CImageNet::set_bIsOpened(bool n)
{
  lock(__LINE__);

  mess -> setMessage(MSG_SET_IO, n?1:0);
  netcl -> SendMsg(mess);
  m_bIsOpened = n;

  unlock();
}

// =======================================================
void CImageNet::set_szPath(char * str)
{
  lock(__LINE__);

  mess -> setMessage(MSG_SET_P, strlen(str)+1);
  netcl -> SendMsg(mess);
  netcl -> Send((void *) str, strlen(str)+1);
  strcpy(m_szPath, str);

  unlock();
}

// =======================================================
void CImageNet::set_szImageFilename(char * str)
{
  lock(__LINE__);

  mess -> setMessage(MSG_SET_IF, strlen(str)+1);
  netcl -> SendMsg(mess);
  netcl -> Send((void *) str, strlen(str)+1);
  strcpy(m_szImageFilename, str);

  unlock();
}

// =======================================================
void CImageNet::set_szOriginalFilename(char * str)
{
  lock(__LINE__);

  mess -> setMessage(MSG_SET_OF, strlen(str)+1);
  netcl -> SendMsg(mess);
  netcl -> Send((void *) str, strlen(str)+1);
  strcpy(m_szOriginalFilename, str);

  unlock();
}

// =======================================================
void CImageNet::set_szImageFilename(char * str, int n)
{
  lock(__LINE__);

  mess -> setMessage(MSG_SET_IF, strlen(str)+1);
  netcl -> SendMsg(mess);
  netcl -> Send((void *) str, strlen(str)+1);
  strcpy(m_szImageFilename, str);

  unlock();
}

// =======================================================
void CImageNet::set_szOriginalFilename(char * str, int n)
{
  lock(__LINE__);

  mess -> setMessage(MSG_SET_OF, strlen(str)+1);
  netcl -> SendMsg(mess);
  netcl -> Send((void *) str, strlen(str)+1);
  strcpy(m_szOriginalFilename, str);

  unlock();
}

// =======================================================
bool CImageNet::get_bIsOpened()
{
  return m_bIsOpened;
}

// =======================================================
char * CImageNet::get_szImageFilename()
{
  return m_szImageFilename;
}

// =======================================================
char * CImageNet::get_szOriginalFilename()
{
  return m_szOriginalFilename;
}

// =======================================================
char * CImageNet::get_szPath()
{
  return m_szPath;
}

// =======================================================
void CImageNet::write(void *buf, DWORD dwLength, int *nResult) 
{
  lock(__LINE__);
  DWORD cmd;

  BEGIN;
  mess -> setMessage(MSG_WRITE, dwLength);
  netcl -> SendMsg(mess);
  netcl -> Send(buf, dwLength);
  netcl -> RecvMsg(mess);

  mess -> getCmd(&cmd);
  if (cmd != MSG_OK)
    {
      CExceptions * excep = netcl->RecvExcep("cin::write");
      unlock();
      throw excep;
    }

  mess->getSize((DWORD *) nResult); 

  unlock();
}

// =======================================================
int CImageNet::getCompressionLevelForImage(char *szFilename) // MBD
{
  DWORD cmd, size;
  lock(__LINE__);
  mess -> setMessage(MSG_GET_C, strlen(szFilename)+1);
  netcl -> SendMsg(mess);
  netcl -> Send(szFilename, strlen(szFilename)+1);
  
  netcl -> RecvMsg(mess);
  
  mess->getCmd(&cmd);
  if (cmd == MSG_OK)
    {
      unlock();
      mess->getSize(&size);
      return size;
    } 
  else
    {
      CExceptions * excep = netcl->RecvExcep("cin::netCompressLevel");
      unlock();
      throw excep;
    }
  return 0; // not reached but no warning from gcc
}

// =======================================================
DWORD CImageNet::read(void *buf, DWORD dwLength) 
{
  DWORD cmd, size;
  lock(__LINE__);

  mess -> setMessage(MSG_READ, dwLength);
  netcl -> SendMsg(mess);
  netcl -> RecvMsg(mess);

  mess->getCmd(&cmd);
  if (cmd == MSG_OK)
    {
      mess->getSize(&size);
      netcl -> Recv(buf, size);
      unlock();
      mess->getSize(&size);
      return size;
    }
  else
    {
      CExceptions * excep = netcl->RecvExcep("cin::read");
      unlock();
      throw excep;
    }
  return 0L; // not reached but no warning from gcc
}

// =======================================================
void CImageNet::close()
{
  DWORD cmd;
  lock(__LINE__);
  mess -> setMessage(MSG_CLOSE, 0);
  netcl -> SendMsg(mess);
  netcl -> RecvMsg(mess);

  mess->getCmd(&cmd);
  if (cmd != MSG_OK)
    {
      CExceptions * excep = netcl->RecvExcep("cin::close");
      unlock();
      throw excep;
    }
  unlock();
}

// =======================================================
void CImageNet::openWriting()
{
  DWORD cmd;
  lock(__LINE__);

  mess -> setMessage(MSG_OPENW, 0);
  netcl -> SendMsg(mess);
  netcl -> RecvMsg(mess);

  mess->getCmd(&cmd);
  if (cmd != MSG_OK)
    {
      CExceptions * excep = netcl->RecvExcep("cin::openw");
      unlock();
      throw excep;
    }
  unlock();
}


// =======================================================
void CImageNet::openReading()
{
  DWORD cmd;
  lock(__LINE__);

  mess -> setMessage(MSG_OPENR, 0);
  netcl -> SendMsg(mess);
  netcl -> RecvMsg(mess);

  mess->getCmd(&cmd);
  if (cmd != MSG_OK)
    {
      CExceptions * excep = netcl->RecvExcep("cin::openr");
      unlock();
      throw excep;
    }
  m_bIsOpened = true;
  unlock();
}	

// =======================================================
void CImageNet::getFreeSpace(QWORD * qwAvailSpace)
{
  DWORD cmd;
  lock(__LINE__);

  BEGIN;
  mess -> setMessage(MSG_GETSPACE, 0L);
  netcl -> SendMsg(mess);
  netcl -> RecvMsg(mess);
  
  mess->getCmd(&cmd);
  if (cmd == MSG_OK)
    {
      netcl -> Recv((void *) qwAvailSpace, sizeof(QWORD));
      showDebug(3, "freespace=%lld\n", *qwAvailSpace);
    }
  else
    {
      CExceptions * excep = netcl->RecvExcep("cin::getfreespace");
      unlock();
      throw excep;
    }

  unlock();
}

// =======================================================
bool CImageNet::doesFileExists(char *szPath)
{
  DWORD cmd;
  lock(__LINE__);

  mess -> setMessage(MSG_DOESFILEEXIST, strlen(szPath)+1);
  netcl -> SendMsg(mess);
  netcl -> Send(szPath, strlen(szPath)+1);
  netcl -> RecvMsg(mess);
  unlock();

  mess->getCmd(&cmd);
  return cmd == MSG_OK;
}

// =======================================================
void CImageNet::getDiskFreeSpaceForFile(QWORD *qwFreeSpace, char *szFilepath)
{
  lock(__LINE__);
  QWORD qwAux;

  mess -> setMessage(MSG_GETDISKSPACEFILE, strlen(szFilepath)+1);
  netcl -> SendMsg(mess);
  netcl -> Send(szFilepath, strlen(szFilepath)+1);
  netcl -> Recv(&qwAux, sizeof(QWORD));
  *qwFreeSpace = qwAux;

  unlock();
}

// =======================================================
void CImageNet::getFileSize(QWORD *qwFileSize, char *szFilepath)
{
  QWORD qwAux;
  lock(__LINE__);

  mess -> setMessage(MSG_GETFILESIZE, strlen(szFilepath)+1);
  netcl -> SendMsg(mess);
  netcl -> Send(szFilepath, strlen(szFilepath)+1);
  netcl -> Recv(&qwAux, sizeof(QWORD));
  *qwFileSize = qwAux;

  unlock();
}

// =======================================================
void CImageNet::get_qwCurrentVolumeSize(QWORD *qwSize)
{
  QWORD qwAux;

  lock(__LINE__);
  mess -> setMessage(MSG_GET_CVS);
  netcl -> SendMsg(mess);
  netcl -> Recv(&qwAux, sizeof(QWORD));
  *qwSize = qwAux;
  
  unlock();
}

// =======================================================
// pass szMountDevice, szMountPoint, szMountFS to server for it to
// automount the imagelocation mountpoint
// return 0 on success and (!0) on error (no error code returned)
int CImageNet::mountImageLocation(char * szMountDevice, char * szMountPoint,
   char * szMountFS)
{
  DWORD dwSize, cmd;
  BEGIN;
  lock(__LINE__);

// send szMountDevice to server
  mess -> setMessage(MSG_MOUNTIMAGELOCATION, strlen(szMountDevice)+1);
  netcl -> SendMsg(mess);
  netcl -> Send(szMountDevice, strlen(szMountDevice)+1);

// send szMountPoint to server
  dwSize = strlen(szMountPoint)+1;
  netcl -> Send(&dwSize, sizeof(DWORD));
  netcl -> Send(szMountPoint, strlen(szMountPoint)+1);

// send szMountFS to server
  dwSize = strlen(szMountFS)+1;
  netcl -> Send(&dwSize, sizeof(DWORD));
  netcl -> Send(szMountFS, strlen(szMountFS)+1);

// receive status of server
  netcl -> RecvMsg(mess);
  unlock();
  mess->getCmd(&cmd);
  return cmd != MSG_OK;
}

// =======================================================
// pass szMountPoint, to server for it to umount imagelocation mountpoint
// return 0 on success and (!0) on error (no error code returned)
int CImageNet::umountImageLocation(char * szMountPoint)
{
  DWORD cmd;
  BEGIN;
  lock(__LINE__);

// send szMountPoint to server
  mess -> setMessage(MSG_UMOUNTIMAGELOCATION, strlen(szMountPoint)+1);
  netcl -> SendMsg(mess);
  netcl -> Send(szMountPoint, strlen(szMountPoint)+1);

// receive status of server
  netcl -> RecvMsg(mess);
  unlock();
  mess->getCmd(&cmd);
  return cmd != MSG_OK;
}

void CImageNet::set_options(COptions * options)
{
  BEGIN;
  lock(__LINE__);

  mess -> setMessage(MSG_NEWOPTIONS);
  netcl -> SendMsg(mess);
  netcl -> Send(options, sizeof(COptions));
  netcl -> RecvMsg(mess);
  unlock();
}
