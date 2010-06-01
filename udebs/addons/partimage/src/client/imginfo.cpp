/***************************************************************************
                            imginfo.cpp  -  description
                             -------------------
    begin                : lun mai 22 18:04:54 CEST 2000
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

#include "common.h"
#include "imginfo.h"
#include "misc.h"
#include "mbr_backup.h"
#include "imagefile.h"
#include "interface_newt.h"

// =======================================================
void imageInfoShowRegular(char *szText, int nMaxTextLen, CMainHeader *head, char *szImagefile, DWORD dwCompression)
{
  char cTemp[512];
  char szFlags[2048];
  char szDescription[MAX_DESCRIPTION];
  char szEncryption[512];

  // format flags
  if (head->dwMainFlags)
    {
      /*if (head->dwMainFlags | FLAG_SPLITTED)
        printf("FLAG_SPLITTED ");*/
    }
  else
    SNPRINTF(szFlags, i18n("No flag set"));
    
  // format description
  if (strlen(head->szPartDescription) > 0)
    SNPRINTF(szDescription, "\n------------------\n%s\n------------------", head->szPartDescription);
  else
    SNPRINTF(szDescription, "%s", i18n("No description"));

  // encryption algorithm
  switch ( head->dwEncryptAlgo)
    {
    case ENCRYPT_NONE:
      SNPRINTF(szEncryption, i18n("none"));
      break;

    default:
      SNPRINTF(szEncryption, i18n("unknown"));
      break;
    }
  
  snprintf(szText, nMaxTextLen, i18n("Filesystem:............%s\n"
				     "Description:...........%s\n"
				     "Original device:.......%s\n"
				     "Original filepath:.... %s\n"
				     "Flags:.................%ld: %s\n"
				     "Creation date:.........%s"
				     "Partition size:........%s\n"
				     "Hostname:..............%s\n"
				     "Compatible Version:....%s\n"
				     "Encryption algorithm:..%ld -> %s\n"
				     "MBR saved count:.......%lu\n\n"
				     "System of the backup:\n"
				     "- machine:.............%s\n"
				     "- operating system:....%s\n"
				     "- release:.............%s\n"
				     "\n\n"), 
	   head->szFileSystem, szDescription, head->szOriginalDevice, head->szFirstImageFilepath, 
	   head->dwMainFlags, szFlags, asctime(&head->dateCreate), formatSize(head->qwPartSize, cTemp), head->szHostname,
	   head->szVersion, head->dwEncryptAlgo, szEncryption, head->dwMbrCount, head->szUnameMachine,
	   head->szUnameSysname, head->szUnameRelease);
}

// =======================================================
void imageInfoShowMBR(char *szText, int nMaxTextLen, int i, CMbr *mbr)	  
{
  snprintf(szText, nMaxTextLen, i18n("-------------------- MBR %.3d -------------------\n"
				     "Device:................%s\n"
				     "Device blocks count:...%llu\n"
				     "Device model:..........%s\n\n"),
	   i, mbr->szDevice, mbr->qwBlocksCount, mbr->szDescModel);
}

// =======================================================
void imageInfoShowVolume(char *szText, int nMaxTextLen, CVolumeHeader *head, char *szImagefile, DWORD dwCompression, QWORD qwImageSize)
{
  //QWORD qwImageSize = 0LL;
  char cTemp[512];
  char szCompression[512];

  // volume size
  //qwImageSize = getFileSize(szImagefile);

  // compression level
  switch (dwCompression)
    {
    case COMPRESS_NONE:
      SNPRINTF(szCompression, i18n("none"));
      break;

    case COMPRESS_GZIP:
      SNPRINTF(szCompression, "gzip");
      break;

    case COMPRESS_BZIP2:
      SNPRINTF(szCompression, "bzip2");
      break;

    case COMPRESS_LZO:
      SNPRINTF(szCompression, "lzo");
      break;

    default:
      SNPRINTF(szCompression, i18n("unknown"));
      break;
    }
  
  snprintf(szText, nMaxTextLen, i18n("Volume number:.........%lu\n"
				     "Volume size:...........%s\n"
				     "Compression level: ....%ld -> %s\n"
				     "Identificator:.........%llu=%llX\n\n"),
	   head->dwVolumeNumber, formatSize(qwImageSize, cTemp), dwCompression, 
	   szCompression, head->qwIdentificator, head->qwIdentificator);
}

// =======================================================
int showImgInfos(char *szImageFile, COptions *options)
{
  char aux[MAXPATHLEN];
  CMainHeader headMain;
  CVolumeHeader headVolume;
  QWORD qwFileSize;
  char szText[16384];
  DWORD i;
  CMbr mbr;
  DWORD dwCompression;

  CImage image(options);
  memset(szText, 0, sizeof(szText));

  if ((!*szImageFile) || (!image.getImageDisk()->doesFileExists(szImageFile)))
    {
      g_interface->msgBoxError(i18n("The image file \"%s\" cannot be found"), szImageFile);
      RETURN_int(-1);
    }

  // ----------- get the image compression level
  dwCompression = (DWORD) image.getCompressionLevelForImage(szImageFile);
  //debugWin("dwCompression=%lu",dwCompression);

  // image file path
  image.set_dwVolumeNumber(0L);
  extractFilenameFromFullPath(szImageFile, aux); // filename without path
  image.set_szOriginalFilename(aux);
  extractFilepathFromFullPath(szImageFile, aux); // filepath without filename
  image.set_szPath(aux);
  image.set_szImageFilename(szImageFile);

  // ----------- open the image file
  try { image.openReading(&headVolume); }
  catch ( CExceptions *excep )
    { 
      image.closeReading(true);
      g_interface->msgBoxError(i18n("Cannot read image file"));
      RETURN_int(-1); 
    }

  // ---- show informations
  image.getImageDisk()->getFileSize(&qwFileSize, szImageFile);
  imageInfoShowVolume(szText+strlen(szText), (sizeof(szText)-strlen(szText)), &headVolume, 
		      szImageFile, dwCompression, qwFileSize);

  if (headVolume.dwVolumeNumber == 0)
    {
      // the read buffer thread can now start working
      //increaseSemaphore(&g_semDataToProcess);

      // ----------- read header  
      try {image.read((char*)&headMain, sizeof(CMainHeader), true);}
      catch (CExceptions * excep)
	{
	  image.closeReading(true);
	  RETURN_int(-1);
	  //throw excep;
	}
      
      // read CRC of CMainHeader
      image.readAndCheckCRC((char*)&headMain, sizeof(CMainHeader));
      
      imageInfoShowRegular(szText+strlen(szText), (sizeof(szText)-strlen(szText)), &headMain, szImageFile, dwCompression);
      
      // 1. check the magic number
      try {image.readAndCheckMagic(MAGIC_BEGIN_MBRBACKUP); }
      catch (CExceptions * excep)
	{
	  image.closeReading(true);
	  RETURN_int(-1);
	  //throw excep;
	}
      
      // 2. read data of the saved MBR
      for (i=0; i < headMain.dwMbrCount; i++)
	{
	  try{image.read((char*)&mbr, sizeof(CMbr), true);}
	  catch (CExceptions * excep)
	    {
	      image.closeReading(true);
	      RETURN_int(-1);
	      //throw excep;
	    }
	  
	  imageInfoShowMBR(szText+strlen(szText), (sizeof(szText)-strlen(szText)), i, &mbr);	  
	}
    }
  
  // close image file
  image.closeReading(true);

  g_interface->msgBoxOk(i18n("Image informations"), szText);

  return 0;
}  

