/***************************************************************************
                          buffer.h  -  description
                             -------------------
    begin                : Web Dec 13 2000
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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdio.h>
#include <zlib.h>      
#include <pthread.h>
#include <semaphore.h>

#include "partimage.h"
#include "misc.h"

class CImage;

// MULTITHREAD
extern int g_nThreadState; // is it the end of the operation
extern pthread_t g_threadBuffer;
extern int g_nThreadError;
extern bool g_bSpaceOnDisk;
//extern pthread_mutex_t g_mutexBuffer; // lock the buffer while working inside it
//extern pthread_mutexattr_t g_mutexBufferAttr;
//extern sem_t g_semDataToProcess; // the procX() buffer thread has work to do
//extern sem_t g_semSpaceInBuffer;

// for time report only
extern DWORD g_dwTimeThreadUser;
extern DWORD g_dwTimeThreadSys;

// function (work thread) which will write the buffer 
void procWriteBufferToImage(void *ptr);

// function (work thread) to fill the buffer
void procReadBufferFromImage(void *ptr); // Thread which will read data from image file

#define BUFFER_SIZE           131072
#define BUFFER_BLOCK_SIZE     14336
#define ENDVOL_MARGIN         65536
#define SOME_SPACE            (1*1024*1024) // 1 MB

//#define COMPRESSED_BUFFER_SIZE (BUFFER_SIZE)
//#define BUFFER_BLOCK_SIZE 4096
//#define NB_OBJECTS 400

#define THREAD_RUNNING   0
#define THREAD_ASKEXIT   1
#define THREAD_FINISHED  2
#define THREAD_EOF       3

// =======================================================
struct CParam
{
  COptions options;
  CImage *image;
  int nFd;
};

#endif // _BUFFER_H_
