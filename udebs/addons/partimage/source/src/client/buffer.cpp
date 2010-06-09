/***************************************************************************
                          buffer.cpp  -  description
                             -------------------
    begin                : Wed Dec 13 2000
    copyright            : (C) 2000 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/
// $Revision: 1.18 $
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

#include <semaphore.h>
#include <pthread.h>
#include <sys/resource.h>

#include "buffer.h"
#include "imagefile.h"
#include "misc.h"
#include "exceptions.h"

// MULTITHREAD
int g_nThreadState = 0; // is it the end of the operation
pthread_t g_threadBuffer;
bool g_bSpaceOnDisk = true;
int g_nThreadError; // manage exceptions from the buffer thread (send them to the main thread)

// for time report only
DWORD g_dwTimeThreadUser = 0;
DWORD g_dwTimeThreadSys = 0;

// =======================================================
void procWriteBufferToImage(void *ptr) // Thread which will write data to image file
{
  int nRes;
  char cBuf[BUFFER_BLOCK_SIZE];
  DWORD dwRes;

  CParam *param;
  CImage *image;
  CImageBase *cid;
  
  param = (CParam *)ptr;
  image = param -> image;
  cid = image -> getImageDisk();
  g_nDebugThreadBufferW = getpid();
  g_nThreadState = THREAD_RUNNING;

  BEGIN;

  FILE *fStream;

  g_bSpaceOnDisk = true;
  g_nThreadError = 0;

  fStream = fdopen(param->nFd, "rb");
  if (!fStream)
    {
      showDebug(1, "ERRORX\n");
      //MBD
    }

  while (!feof(fStream))
    {
      nRes = fread(cBuf, 1, sizeof(cBuf), fStream);
      if ((DWORD)nRes < sizeof(cBuf))
	{
	  showDebug(1, "EOF\n");
	  // MBD
	}

      dwRes = nRes;
      cid->write((void*)cBuf, dwRes, &nRes); // last param was updateCRC
      if (nRes)
	{
	  if (nRes == ERR_INCOMPLETE)
	    {
	      g_bSpaceOnDisk = false;
	      showDebug(1, "ERRORX: So space left\n");
	    }
	  else
	    {
	      showDebug(1, "ERROR received from cid::write(): %d\n", nRes);
	      g_nThreadError = nRes;
	    }
	}
    }

  fclose(fStream); //close(nFd);
  
  RETURN;
}

// =======================================================
#define checkFinished(nFd) \
{ \
  if (g_nThreadState == THREAD_ASKEXIT)\
    {\
      showDebug(1, "EXIT procReadBufferFromImage inside while and g_nThreadState=THREAD_ASKEXIT\n");\
      close(nFd);\
      g_nThreadState = THREAD_FINISHED;\
      RETURN;\
    }\
}

// =======================================================
void procReadBufferFromImage(void *ptr) // Thread which will read data from image file
{
  char cBuf[BUFFER_BLOCK_SIZE]; 
  DWORD dwRes;
  int nRes;

  CParam *param;
  CImage *image;
  CImageBase *cid;
  int nFd;
  
  param = (CParam *)ptr;
  image = param -> image;
  cid = image -> getImageDisk();
  g_nDebugThreadBufferW = getpid();
  nFd = param->nFd;

  BEGIN;

  g_nThreadState = THREAD_RUNNING;
  g_bSpaceOnDisk = true;
  g_nThreadError = 0;
  dwRes = 0;
  //g_nEss = 0;

  do
    {
      checkFinished(nFd);

      dwRes = cid->read((void*)cBuf, sizeof(cBuf));
      if (dwRes != sizeof(cBuf))
	showDebug(1, "EOF: dwRes=%lu and sizeof(cBuf)=%lu\n", dwRes, (DWORD)sizeof(cBuf));

      checkFinished(nFd);
      
      //if (dwRes < sizeof(cBuf))
      //showDebug(1, "EOF2: dwRes=%lu and total=%lu\n", dwRes, (DWORD)sizeof(cBuf));
      if (dwRes > 0)
	{
          if (dwRes != sizeof(cBuf))
            {
              showDebug(1, "unexpected end of file...\n");
	      showDebug(1, "%ld missing bytes\n", sizeof(cBuf)-dwRes);
	      memset(cBuf+dwRes, 0, sizeof(cBuf)-dwRes);
/* FIXME	      dwRes = sizeof(cBuf); */
	    }
	  nRes = write(nFd, cBuf, dwRes);
	  if (nRes == -1)
	    {
	      // MBD
	      showDebug(1, "ERRORX\n");
	    }
	}
      else
	{
	  showDebug(1, "cid->read() returned 0\n");
	}	
    }
  while ((dwRes == sizeof(cBuf))); // && (g_nThreadState != THREAD_ASKEXIT));

  showDebug(1, "EXIT procReadBufferFromImage after while and g_nThreadState=%d\n", g_nThreadState);

  close(nFd);
  g_nThreadState = THREAD_EOF;
  //g_bFinished = false;
  RETURN;
}

  
  
  
