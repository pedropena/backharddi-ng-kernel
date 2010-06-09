/***************************************************************************
                        createdata.cpp  -  description
                             -------------------
    begin                : Sun May  6 15:18:11 CEST 2001
    copyright            : (C) 2001 by Francois Dupoux
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

#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define LEVELS   3
#define NB       10
#define BUFSIZE  65536

// file size
static char *g_szOptString = "d:c";
unsigned long g_nSize[NB] = {15968, 4546, 65465, 128963, 156321, 12589, 485987, 2456893, 237892, 12568};
#define MAXFILESIZE (1024 * 890) // 890 KB

char g_cBuffer[BUFSIZE];

#define ACTION_CREATE  1
#define ACTION_REMOVE  2

// ===============================================
void initBuffer()
{
  int i;
  
  for (i=0; i < BUFSIZE; i++)
    {
      g_cBuffer[i] = rand();
    }
}

// ===============================================
void changeBuffer()
{
  int i;
  
  for (i=0; i < BUFSIZE; i++)
    {
      g_cBuffer[i] ^= rand();
    }
}

// ===============================================
int createFile(char *szFilepath, unsigned long nSize)
{
  FILE *fStream;
  unsigned long i;
  char cCar;
  long nPos;
  char cRand;

  truncate(szFilepath, 0);
  fStream = fopen(szFilepath, "wb");
  if (!fStream)
    return -1;

  // ----

  nPos = rand() % BUFSIZE;
  cRand = ((int)(rand() % 256) + getpid());

  for (i=0; i < nSize; i++)
    {
      cCar = g_cBuffer[(nPos+i) % BUFSIZE] + cRand;
      fputc((int)cCar, fStream);
    }  

  // ----

  fclose(fStream);
  return 0;
}

// ===============================================
int createData(int nAction, char *szMainDir, char *szName, int nLevel, int nDeleteAndKeep)
{
  int i;
  char szFull[16];
  char szPath[MAXPATHLEN];
  int nRes;
  unsigned long nSize;
  int nNb;

  mkdir(szMainDir, 644);
  changeBuffer();
  nNb = 0;

  for (i=0; i < 26; i++)
    {
      snprintf(szFull, sizeof(szFull), "%s%c", szName, 'a'+i);
      snprintf(szPath, MAXPATHLEN, "%s/%s", szMainDir, szFull);
	  
      if (nLevel == LEVELS) // createFile
	{
	  if (nAction == ACTION_CREATE)
	    {
	      nSize = (rand() ^ g_nSize[rand() % NB]) % MAXFILESIZE;
	      printf ("creating \"%s\": size = %lu\n", szPath, (long unsigned)nSize);
  	      nRes = createFile(szPath, nSize);
	      if (nRes == -1)
		return -1;
	    }
	  else // ACTION_REMOVE
	    {
	      if ((nNb % nDeleteAndKeep) != 0) // keep 1/4 of files
		{
		  printf ("deleting \"%s\"\n", szPath);
		  errno=0;
		  nRes = unlink(szPath); // erase the file
		  if (nRes == -1)
		    return -1;
		}

	    }
	  nNb++;
	}
      else // createDir
	{
	  mkdir(szPath, 644);
	  nRes = createData(nAction, szPath, szFull, nLevel+1, nDeleteAndKeep);
	  if (nRes == -1)
	    return -1;
	}
    }

  return 0;
}

// ===============================================
int main(int argc, char *argv[])
{
  char szRoot[MAXPATHLEN+1];
  int nOpt;
  int nDeleteAndKeep = 0;
  bool bCreateFiles = true;

  while ((nOpt = getopt(argc, argv, g_szOptString)) != EOF)
    switch(nOpt)
      {
      case 'd':
	nDeleteAndKeep = atoi(optarg);
	break;

      case 'c':
	bCreateFiles = false;
	break;
      }

  if (argc - optind != 1)
    {
      fprintf(stderr, "command: [-d:X] createadata <path>\n"
	      "options:\n"
	      " -d:X  delete files after creation, and keep 1 file for X files.\n"
	      " -c    don't create any file.\n\n");
      return EXIT_FAILURE;
    }

  memset(szRoot, 0, MAXPATHLEN);
  strncpy(szRoot, argv[optind], MAXPATHLEN);
  while ((strlen(szRoot)>0) && (szRoot[strlen(szRoot)-1] == '/'))
    szRoot[strlen(szRoot)-1] = 0;

  initBuffer();

  // create files
  if (bCreateFiles)
    createData(ACTION_CREATE, szRoot, "", 0, 0);

  // make the bitmap fragmented
  if (nDeleteAndKeep)
    createData(ACTION_REMOVE, szRoot, "", 0, nDeleteAndKeep);

  return EXIT_SUCCESS;
}

