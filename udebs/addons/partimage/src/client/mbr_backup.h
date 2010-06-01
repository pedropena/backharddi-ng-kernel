/***************************************************************************
                          mbr_backup.h  -  description
                             -------------------
    begin                : Thu Jun 29 2000
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

#ifndef MBR_BACKUP_H
#define MBR_BACKUP_H

#define MBR_SIZE_WHOLE 512
#define MBR_SIZE_BOOT  446
#define MBR_SIZE_TABLE 64

#define MAX_DESC_MODEL 128
#define MAX_DESC_GEOMETRY 1024
#define MAX_DESC_IDENTIFY 4096

#define MBR_MAGIC 0xAA55

// ========================================================================================
struct CMbr // must be 1024
{
  char cData[MBR_SIZE_WHOLE];
  DWORD dwDataCRC;
  char szDevice[MAX_DEVICENAMELEN]; // ex: "hda"
  
  // disk identificators
  QWORD qwBlocksCount;
  char szDescModel[MAX_DESC_MODEL];

  BYTE cReserved[884]; // for future use

  //char szDescGeometry[MAX_DESC_GEOMETRY];
  //char szDescIdentify[MAX_DESC_IDENTIFY];
} __attribute__ ((packed));

// ========================================================================================
int mbrGetData(char *szDevice, CMbr *mbr, DWORD dwBlocksCount);
int mbrGetInfoDir(char *szDevice, char *szDir);
int mbrCopyFileToString(char *cDest, char *szFilepath, int nMaxSize);
int mbrParseProcPart(char *szHdList, int nMaxLen, int nWhatMajorMustBe=-1, int nMinorOfPartToSave=-1);
void imageInfoShowMBR(int i, CMbr *mbr);
bool isDriveReady(char *szDevice);

#endif // MBR_BACKUP_H
