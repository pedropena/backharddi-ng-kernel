/***************************************************************************
                          mbr_backup.cpp  -  description
                             -------------------
    begin                : Thu Jun 29 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/
// $Revision: 1.16 $
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

#include "partimage.h"
#include "common.h"
#include "imagefile.h"
#include "misc.h"
#include "mbr_backup.h"
#include "interface_newt.h"

#include <stdio.h>
#include <sys/stat.h>

// =============================================================================
// This function must return TRUE if we can read data from device. 
// It's not a good way to try to read data, because the kernel could print 
// an error message in the current terminal.
bool isDriveReady(char *szDevice)
{
  BEGIN;
  
  char cTempBuf[1024];
  FILE *fTest;
  int nRes;

  //newtWinMessage("testing","ok", szDevice);
  
  fTest = fopen(szDevice, "rb");
  if (!fTest)
    {
      //newtWinMessage("CANOOT READ DEVICE 1","ok", szDevice); // HERE
      RETURN_bool(false);
    }

  nRes = fread(cTempBuf, my_min(sizeof(cTempBuf), 512), 1, fTest);
  fclose(fTest);

  if (nRes == 1) // if possible to read data from device
    RETURN_bool(true)
  else
    {
      //newtWinMessage("CANOOT READ DEVICE 2","ok", szDevice); // HERE
      RETURN_bool(false);
    }
}

// =============================================================================
int mbrGetInfoDir(char *szDevice, char *szDir)
{
  BEGIN;

  char szShort[256];
  int i;

  // ---- get the short name (ex: "/dev/hda -> hda")
  memset(szDir, 0, MAXPATHLEN);
  memset(szShort, 0, 256);
  for (i=strlen(szDevice); i && (szDevice[i] != '/'); i--);
  strncpy(szShort, szDevice+i+1, 256);

  // ---- get the directory
  if ((szShort[0] == 'h') && (szShort[1] == 'd')) // if an IDE device
    {
      snprintf(szDir, MAXPATHLEN, "/proc/ide/%s", szShort);
      RETURN_int(0);
    }
  /*else if ((szShort[0] == 's') && (szShort[1] == 'd')) // if an SCSI device
    {
    snprintf(szDir, ...);
    return 0;
    }*/
  else
    RETURN_int(-1)

}
// =============================================================================
// returns the number of physical hard disks (-1 on error)
// if nPartNb == 0 -> returns the number of hard disks
// if nPartNb != 0 -> get the name of the hard disk nb nPartNb
int mbrParseProcPart(char *szHdList, int nMaxLen, int nWhatMajorMustBe, int nMinorOfPartToSave) 
{
  BEGIN;

  FILE *fPart;
  int nMajor, nMinor, nBlocks;
  char szDevice[128];
  char cBuffer[32768];
  char szTemp[1024];
  char *cPtr;
  int nLines;
  int nSize;
  int i;
  int nPartNum;
  int nCount;
  
  memset(szHdList, 0, nMaxLen-1);

  errno = 0;
  fPart = fopen("/proc/partitions", "rb");
  if (!fPart)
    {
      showDebug(1, "cannot read /proc/partition -> not MBR will be saved\n");
      RETURN_int(0);	// no MBR will be saved, and no error
    }

  nSize = 0;
  nLines = 0;
  memset(cBuffer, 0, 32767);
  while (!feof(fPart))
    {
      cBuffer[nSize] = fgetc(fPart);
      if (cBuffer[nSize] == '\n')
	nLines++;		
      nSize++;
    }
  
  cPtr = cBuffer;
  nCount = 0;

  // skip two first lines
  nLines -= 2;
  for (i=0; i < 2; i++)
    {
      while (*cPtr != '\n')
	cPtr++;
      cPtr++;
    }
  
  showDebug(3, "Major must be: %d\n", nWhatMajorMustBe);

  for (i=0; i < nLines; i++)
    {
      cPtr = decodePartitionEntry(cPtr, &nMajor, &nMinor, &nBlocks, &nPartNum, szDevice);
      showDebug(3, "PARTITION_ENTRY: device=[%s] and maj=%d and min=%d and num=%d\n", szDevice, nMajor, nMinor, nPartNum);
      
      if ((nWhatMajorMustBe == -1) || ((nMajor == nWhatMajorMustBe) && (nMinor <= nMinorOfPartToSave)))
	{
	  if (nPartNum == 0) // if it is an hard drive (as hda, hdc)
	    {
              showDebug(3, "PARTITION_ENTRY: OKAY device=[%s] and maj=%d and min=%d and num=%d\n", szDevice, nMajor, nMinor, nPartNum);

	      // can we read the partition ?
	      SNPRINTF(szTemp, "/dev/%s", szDevice);
	      
	      //debugWin("isDriveReady(%s)=%d",szTemp,isDriveReady(szTemp));
	      //if (isDriveReady(szTemp))
		{
		  nCount++; // number of hard disks
		  SNPRINTF(szTemp, "/dev/%s#%lu#",szDevice, (DWORD)nBlocks);
		  if (szHdList)
		    strncat(szHdList, szTemp, nMaxLen);
		}
	    }
	}
    }
  
  fclose(fPart);

  RETURN_int(nCount);
}

// =============================================================================
int mbrCopyFileToString(char *cDest, char *szFilepath, int nMaxSize)
{
  BEGIN;

  FILE *fInfo;
  int i;
  int c;

  // ---- open file
  fInfo = fopen(szFilepath, "rb");
  if (!fInfo)
    {
      showDebug(1, "MBR (001): cannot open %s in rb mode\n", szFilepath);
      RETURN_int(-1);
    }

  // ---- copy data
  memset(cDest, 0, nMaxSize);
  for (i=0; (i < nMaxSize) && (!feof(fInfo)) && ((c = getc(fInfo)) != EOF); i++)
    cDest[i] = c;

  // ---- close file
  fclose(fInfo);

  return(0);
}

// =============================================================================
int mbrGetData(char *szDevice, CMbr *mbr, DWORD dwBlocksCount)
{
  BEGIN;

  FILE *fPart;
  int nRes;
  char szInfoDir[MAXPATHLEN];
  char szTemp[MAXPATHLEN];
  int i;

  //debugWin("mbrGetData: seDev=[%s] and dwBlock=%lu", szDevice, dwBlocksCount);

  // ---- init
  memset(mbr, 0, sizeof(CMbr));
  strncpy(mbr->szDevice, szDevice, MAX_DEVICENAMELEN);
  mbr->qwBlocksCount = (QWORD)dwBlocksCount;

  // ---- copy data
  fPart = fopen(szDevice, "rb");
  if (!fPart)
    {
      showDebug(1, "MBR (001): cannot open %s in rb mode\n", szDevice);
      RETURN_int(-1);
    }

  nRes = fread(mbr->cData, MBR_SIZE_WHOLE, 1, fPart);
  fclose(fPart);
  if (nRes != 1)
    {
      showDebug(1, "MBR (002): cannot read MBR\n", szDevice);
      RETURN_int(-1);
    }

  // ---- CRC of the data
  mbr->dwDataCRC = 0;
  for (i=0; i < MBR_SIZE_WHOLE; i++)
    mbr->dwDataCRC += mbr->cData[i];

  // ---- copy descriptions
  nRes = mbrGetInfoDir(szDevice, szInfoDir);
  if (nRes != -1) // for an IDE disk
    {
      // model
      SNPRINTF(szTemp, "%s/model", szInfoDir);
      nRes = mbrCopyFileToString(mbr->szDescModel, szTemp, MAX_DESC_MODEL);
      if (nRes == -1)
	{
	  showDebug(1, "MBR (003): error in mbrCopyFileToString\n", szDevice);
	  RETURN_int(-1);
	}
    }
  else
    {
      memset(mbr->szDescModel, 0, MAX_DESC_MODEL);      
    }

  // geometry
  /*
    sprintf(szTemp, "%s/geometry", szInfoDir);
    mbrCopyFileToString(mbr->szDescGeometry, szTemp, MAX_DESC_GEOMETRY);
    if (nRes == -1)
    RETURN_int(-1);
  */
  
  // geometry
  /*
    sprintf(szTemp, "%s/identify", szInfoDir);
    mbrCopyFileToString(mbr->szDescIdentify, szTemp, MAX_DESC_IDENTIFY);
    if (nRes == -1)
    RETURN_int(-1);
  */
  
  RETURN_int(0);
}
