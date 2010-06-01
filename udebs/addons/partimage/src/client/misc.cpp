/***************************************************************************
                          misc.cpp  -  description
                             -------------------
    begin                : Thu Jun 29 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/
// $Revision: 1.86 $
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
#include <unistd.h>

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif
#ifdef HAVE_SYS_STAT_H
  #include <sys/stat.h>
#endif

#ifdef HAVE_MNTENT_H
  #include <mntent.h>
#else
  #warning "mntent.h not found"
#endif

#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/utsname.h>

#include "misc.h"
#include "partimage.h"
#include "imagefile.h"
#include "interface.h"
#include "interface_newt.h"
#include "mbr_backup.h"

#include "fs/fs_reiser.h"
#include "fs/fs_ufs.h"
#include "fs/fs_ext2.h"
#include "fs/fs_hpfs.h"
#include "fs/fs_ntfs.h"
#include "fs/fs_fat.h"
#include "fs/fs_jfs.h"
#include "fs/fs_xfs.h"
#include "fs/fs_hfs.h"
#include "fs/fs_afs.h"
#include "fs/fs_be.h"

DWORD g_dwCrcTable[256];

// ----- reserved for future use defs --------
#define FUTURE_SKIPDATA(magic) \
  { \
    image.readAndCheckMagic(magic); \
    image.read((char*)&dwLength, sizeof(DWORD), true); \
    for (i=0; i < dwLength; i++) \
      image.read(&cTemp, sizeof(cTemp), true); \
  }

#define FUTURE_WRITEDATA(magic) \
  { \
    image.writeMagic(magic); \
    image.write(&dwLength, sizeof(DWORD), true); \
  }

// =======================================================
void restoreMbr(char *szImageFile, COptions *options)
{
  BEGIN;

  CRestoreMbrWindow optGui;
  DWORD dwDiskCount;
  DWORD dwOriginalMbrNb;
  DWORD dwCurrentMbrNb;
  char szHdList[2048];
  char szTemp[512];
  char szTemp2[512];
  char *cCurPart;
  DWORD i, j;
  char aux[MAXPATHLEN];
  int nRes=0;
  CMbr *mbrOriginal;
  CMbr *mbrCurrent;
  DWORD dwCRC;
  int nRestoreMode;

  // structures to stock informations about the current partition
  CImage image(options);
  
  // ----------- check the user is logged as root
  if (geteuid() != 0) // 0 is the UID of the root
    {	
      nRes = g_interface -> ErrorLogAsRoot();
      if (nRes == 2)
        THROW(ERR_CANCELED);
    }		

  // image file path
  image.set_dwVolumeNumber(0L);
  extractFilenameFromFullPath(szImageFile, aux); // filename without path
  image.set_szOriginalFilename(aux);
  extractFilepathFromFullPath(szImageFile, aux); // filepath without filename
  image.set_szPath(aux);
  image.set_szImageFilename(szImageFile);

  // ----------- get the image compression level
  options->dwCompression = (DWORD) image.getCompressionLevelForImage(szImageFile);
  if (options->dwCompression == COMPRESS_BZIP2)
  {
     g_interface -> ErrorBugBzip2();
     THROW(ERR_CANCELED);
  }
  
  // ----------- open the image file
  image.set_options(options);
  try { image.openReading(); }
  catch ( CExceptions * excep )
    { throw excep; }

  // the read buffer thread can now start working
  //increaseSemaphore(&g_semDataToProcess);

  // ----------- read header
  CMainHeader headMain;
  
  try{image.read((char*)&headMain, sizeof(CMainHeader), true);}
  catch (CExceptions * excep)
    {
      image.closeReading(true);
      throw excep;
    }
  
  // read CRC of CMainHeader
  image.readAndCheckCRC((char*)&headMain, sizeof(CMainHeader));

  // check there are MBR in the image file
  if (headMain.dwMbrCount == 0)
    {
      image.closeReading(true);
      THROW( ERR_NOMBR);
    }

  dwDiskCount = mbrParseProcPart(szHdList, sizeof(szHdList));

  // allocate memory for original+current MBR
  mbrOriginal = new CMbr[headMain.dwMbrCount];
  mbrCurrent = new CMbr[dwDiskCount];
  if ((!mbrOriginal) || (!mbrCurrent))
    {
      image.closeReading(true);
      THROW( ERR_NOMEM);
    }

  // MBR window
  if (options->bBatchMode == false)
    optGui.create();

  // ---- add hard disks
  cCurPart = szHdList;
  for (i=0; i < dwDiskCount; i++)
    {
      memset(&mbrCurrent[i], 0, sizeof(CMbr));
      for (j=0; (j < 512) && (*cCurPart != '#'); j++, cCurPart++)
	mbrCurrent[i].szDevice[j] = *cCurPart;
      cCurPart++;
      for (j=0; (j < 512) && (*cCurPart != '#'); j++, cCurPart++)
	szTemp[j] = *cCurPart;
      cCurPart++;
      mbrCurrent[i].qwBlocksCount = atol(szTemp);
      SNPRINTF(szTemp2, "%s [%s blocks]", mbrCurrent[i].szDevice, szTemp);
      if (options->bBatchMode == false)
	optGui.addHardDisk(szTemp2, i);
    }

  // ---- add MBR available

  // 1. check the magic number
  try {image.readAndCheckMagic(MAGIC_BEGIN_MBRBACKUP); }
  catch (CExceptions * excep)
    {
      image.closeReading(true);
      throw excep;
    }

  // 2. read data of the saved MBR
  for (i=0; i < headMain.dwMbrCount; i++)
    {
      try{image.read((char*)&(mbrOriginal[i]), sizeof(CMbr), true);}
      catch (CExceptions * excep)
	{
	  image.closeReading(true);
          throw excep;
	}
      SNPRINTF(szTemp, "%.3lu: %s [%llu blocks]", i, mbrOriginal[i].szDevice, mbrOriginal[i].qwBlocksCount);
      if (options->bBatchMode == false)
	optGui.addMbr(szTemp, i);
      
      // check the CRC
      dwCRC = 0;
      for (j=0; j < MBR_SIZE_WHOLE; j++)
	dwCRC += mbrOriginal[i].cData[j];

      if (dwCRC != mbrOriginal[i].dwDataCRC)
	{
	  image.closeReading(true);
          THROW(ERR_CHECK_CRC, mbrOriginal[i].dwDataCRC, dwCRC);
	}	  
    }

  // close image file
  image.closeReading(true);

  if (options->bBatchMode == false)
    {
      nRes = optGui.runForm();
      optGui.getValues(&dwCurrentMbrNb, &dwOriginalMbrNb, &nRestoreMode);
      optGui.destroyForm();

      if (nRes == KEY_EXIT)
	THROW( ERR_CANCELED);
      if (nRes == KEY_BACK)
	THROW( ERR_COMEBACK);

      nRes = g_interface->WarnRestoreMBR(mbrCurrent[dwCurrentMbrNb].szDevice,
					 mbrCurrent[dwCurrentMbrNb].qwBlocksCount,
					 mbrOriginal[dwOriginalMbrNb].szDevice,
					 mbrOriginal[dwOriginalMbrNb].qwBlocksCount);

      if (nRes == MSGBOX_CANCEL) // cancel
	THROW( ERR_CANCELED);  
 
      // check the original MBR was on the same disk, or show warning
      if ((strcmp(mbrOriginal[dwCurrentMbrNb].szDevice, mbrOriginal[dwOriginalMbrNb].szDevice) != 0) || 
	  (mbrCurrent[dwCurrentMbrNb].qwBlocksCount != mbrOriginal[dwOriginalMbrNb].qwBlocksCount))
	{
	  nRes = g_interface -> WarnRestoreOtherMBR(mbrCurrent[dwCurrentMbrNb].szDevice,
						    mbrOriginal[dwOriginalMbrNb].szDevice);
	  if (nRes == MSGBOX_CANCEL) // cancel
	    THROW( ERR_CANCELED);
	}    
    }
  
  // ---- do the write operation
  FILE *fDevice;
  
  // open device
  fDevice = fopen(mbrCurrent[dwCurrentMbrNb].szDevice, "r+b");
  if (!fDevice)
    {
      THROW( ERR_OPENING_DEVICE, errno);
    }
  
  // lock the partition
  try { lockFile(fileno(fDevice), mbrCurrent[dwCurrentMbrNb].szDevice); }
  catch ( CExceptions * excep )
    { throw excep; }
  
  // restore data
  if (nRestoreMode == MBR_RESTORE_WHOLE)
    nRes = fwrite(mbrOriginal[dwOriginalMbrNb].cData, MBR_SIZE_WHOLE, 1, fDevice);
  else if (nRestoreMode == MBR_RESTORE_BOOT)
    {
      nRes = fwrite(mbrOriginal[dwOriginalMbrNb].cData, MBR_SIZE_BOOT, 1, fDevice);
      if (nRes != -1) // Restore 0xAA55
	{
	  nRes = fseek(fDevice, MBR_SIZE_TABLE, SEEK_CUR); // skip the partition table
	  if (nRes != -1)
	    nRes = fwrite(mbrOriginal[dwOriginalMbrNb].cData+MBR_SIZE_BOOT+MBR_SIZE_TABLE, 2, 1, fDevice); // table + 0xAA55
	}

    }
  else if (nRestoreMode == MBR_RESTORE_TABLE)
    {
      nRes = fseek(fDevice, MBR_SIZE_BOOT, SEEK_SET);
      if (nRes != -1)
	nRes = fwrite(mbrOriginal[dwOriginalMbrNb].cData+MBR_SIZE_BOOT, MBR_SIZE_TABLE+2, 1, fDevice); // table + 0xAA55
    }

  // re-read partition table
  if (ioctl(fileno(fDevice), BLKRRPART))
    {
      showDebug(1, "Cannot reread partition table: errno=%s\n", strerror(errno));
      THROW(ERR_ERRNO, errno);
    }
  else
    {
      showDebug(1, "Partition table was reread\n");
    }

  // close and check error
  flock(fileno(fDevice), LOCK_UN);
  fclose(fDevice);
  
  // free memory
  delete []mbrOriginal;
  delete []mbrCurrent;
  
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);

  RETURN;
}

// =======================================================
void savePartition(char *szDevice, char *szImageName, //, char *szFileSystem,
   COptions *options)
{
  BEGIN;

  int nRes;
  time_t dt;
  FILE *fDeviceFile;
  int nFs;
  CSavingWindow guiInfo;
  CSaveOptWindow guiOpt;
  CMainHeader headMain;
  char aux[MAXPATHLEN];
  char szHdList[2048];
  char szTemp[512];
  char szTemp2[512];
  DWORD dwBlocks;
  DWORD dwLength;
  DWORD i, j;
  int nMajor, nMinor;
  char szFileSystem[1024];

  // structures to stock informations about the current partition
  CImage image(options);
  memset(&headMain, 0, sizeof(CMainHeader));
  
  // ----------- calculate filenames
  extractFilenameFromFullPath(szImageName, aux); // filename without path
  image.set_szOriginalFilename(aux);
  extractFilepathFromFullPath(szImageName, aux); // filepath without filename
  image.set_szPath(aux);
  
  //if (options->dwSplitMode == SPLIT_AUTO)
  if (!options->qwSplitSize)
    {
      SNPRINTF(aux, "%s", szImageName); *(aux+MAXPATHLEN-1)='\0';
      image.set_szImageFilename(aux);
    }
  else
    {
      SNPRINTF(aux, "%s.000", szImageName);
      *(aux+MAXPATHLEN-1) = '\0';
      image.set_szImageFilename(aux);
    }
  
  // show operation window
  if (options->bBatchMode == false)
    {
      guiOpt.create(image.get_szImageFilename(), *options);
    }

  g_interface -> StatusLine(i18n("initializing the operation. Please wait..."));
  
  // ----------- system checks
  nRes = systemChecks(szDevice, options->bFailIfMounted);
  if (nRes == -1)
    THROW( ERR_FSCHK);
  
  // ----------- detect file system
  nFs = detectFileSystem(szDevice, szFileSystem);
  if (nFs == -1)
    THROW( ERR_WRONG_FS);
  
  // show operation window
  if (options->bBatchMode == false)
    {
      nRes = guiOpt.runForm();

      guiOpt.getValues(options);
      image.set_options(options); 
      guiOpt.destroyForm();
      
      if (nRes == KEY_EXIT)
        THROW( ERR_CANCELED);
      if (nRes == KEY_BACK)
        THROW( ERR_COMEBACK);

      // check options
      nRes = checkOptions(*options, szDevice, szImageName);
      if (nRes == -1)
        THROW( ERR_CANCELED);
    }
  
  // ask image description to the user
  if (options->bAskDesc)
    {
      g_interface -> askDescription(headMain.szPartDescription,
				    MAX_DESCRIPTION);
    }

  // bzip2 bug workaround
  if ((options->dwCompression == COMPRESS_BZIP2) 
    && (!g_interface->getBatchMode()))
    {
      nRes = g_interface -> msgBoxContinueCancel(i18n("Bzip2 bug workaround"),
         i18n("Because of a bug, you won't be able to restore MBR from any "
           "bzip2 compressed image unless you manualy run bzip2 -d on them"));
      if (nRes == MSGBOX_CANCEL)
        THROW(ERR_CANCELED);
    }
 
  // ----------- open partition file
  errno = 0;
  fDeviceFile = fopen(szDevice, "rb");
  if (fDeviceFile == NULL)
    THROW( ERR_OPENING_DEVICE, errno);
  
  // lock the partition
  try { lockFile(fileno(fDeviceFile), szDevice); }
  catch ( CExceptions * excep )
    { throw excep; } 
  
  // ----------- open the image file
  try { image.openWriting(); }
  catch ( CExceptions * excep )
    { throw excep; }
  
  // ----------- init of the info window --------
  guiInfo.create(szDevice, image.get_szImageFilename(), szFileSystem,
		 getPartitionSize(szDevice), *options);
  nRes = guiInfo.runForm();
  if (nRes == -1)
    THROW( ERR_CANCELED);

  image.setGuiSave(&guiInfo);
  
  // ----------- write main header
  
  // fill header infos	
  strcpy(headMain.szFileSystem, szFileSystem);
  strcpy(headMain.szOriginalDevice, szDevice);
  
  if (gethostname(headMain.szHostname, MAX_HOSTNAMESIZE))
    strcpy(headMain.szHostname, "Unknown local machine");

  // filename without path  
  extractFilenameFromFullPath(szImageName, headMain.szFirstImageFilepath);
  headMain.dwCompression = options->dwCompression;
  headMain.dwMainFlags = 0;
  headMain.dwEncryptAlgo = ENCRYPT_NONE; 
  time(&dt);

  // current time/date (date of image creation)
  headMain.dateCreate = *localtime(&dt);

  // file format version
  strcpy(headMain.szVersion, CURRENT_IMAGE_FORMAT); 
  headMain.dwMbrSize = sizeof(CMbr);

  nRes = getMajorMinor(szDevice, &nMajor, &nMinor);

  if (options->bBackupMBR)
    headMain.dwMbrCount = mbrParseProcPart(szHdList, sizeof(szHdList), nMajor, nMinor); // get number of hard disks to save MBR
  else
    headMain.dwMbrCount = 0; // do not backup the MBR

  //debugWin("headMain.dwMbrCount=%lu and szHdList=[%s]", headMain.dwMbrCount, szHdList);

  // current architecture
  utsname arch;
  uname(&arch);
  strncpy(headMain.szUnameSysname, arch.sysname, MAX_UNAMEINFOLEN);
  strncpy(headMain.szUnameNodename, arch.nodename, MAX_UNAMEINFOLEN);
  strncpy(headMain.szUnameRelease, arch.release, MAX_UNAMEINFOLEN);
  strncpy(headMain.szUnameVersion, arch.version, MAX_UNAMEINFOLEN);
  strncpy(headMain.szUnameMachine, arch.machine, MAX_UNAMEINFOLEN);
  
  // reserved for future use: number of structures savec. 
  // (can be number of disklabels saved for ext000, can be number
  // of extended partition tables saved for ext001, ...)
  headMain.dwReservedFuture000 = 0;
  headMain.dwReservedFuture001 = 0;
  headMain.dwReservedFuture002 = 0;
  headMain.dwReservedFuture003 = 0;
  headMain.dwReservedFuture004 = 0;
  headMain.dwReservedFuture005 = 0;
  headMain.dwReservedFuture006 = 0;
  headMain.dwReservedFuture007 = 0;
  headMain.dwReservedFuture008 = 0;
  headMain.dwReservedFuture009 = 0;

  // split mode
  /*if (options->dwSplitMode != SPLIT_NONE)
    headMain.dwMainFlags |= FLAG_SPLITTED;*/
  //image.set_bIsSplitted((options->dwSplitMode != SPLIT_NONE));
  
  headMain.qwPartSize = getPartitionSize(szDevice);
  
  //image.set_dwCurrentVolumeSize(0L);
  image.set_dwVolumeNumber(0L);
  //image.set_qwIdentificator(headMain.qwIdentificator);
  
  try { image.write(&headMain, sizeof(CMainHeader), true); }
  catch ( CExceptions * excep )
    {
      closeFilesSave(true, *options, &image, fDeviceFile);
      throw excep;
    }
  
  // write CRC of CMainHeader
  image.writeCRC((char*)&headMain, sizeof(CMainHeader));

  // ----------- save the MBR in the beginning of the image file
  image.writeMagic(MAGIC_BEGIN_MBRBACKUP);

  CMbr mbr;
  char *cCurPart;

  showDebug(9, "SAVEMBR_TRACE_001\n");

  cCurPart = szHdList;
  for (i=0; i < headMain.dwMbrCount; i++)
    {
      showDebug(9, "SAVEMBR_TRACE_002\n");
  
      memset(szTemp, 0, 512);
      for (j=0; (j < 512) && (*cCurPart != '#'); j++, cCurPart++)
	szTemp[j] = *cCurPart;
      cCurPart++;
      memset(szTemp2, 0, 512);
      for (j=0; (j < 512) && (*cCurPart != '#'); j++, cCurPart++)
	szTemp2[j] = *cCurPart;
      cCurPart++;
      dwBlocks = atol(szTemp2);
      
      showDebug(9, "SAVEMBR_TRACE_003\n");

      nRes = mbrGetData(szTemp, &mbr, dwBlocks); // BUG HERE
      if (nRes == -1)
	{
	  closeFilesSave(true, *options, &image, fDeviceFile);
          THROW( ERR_SAVE_MBR);
	}

      showDebug(9, "SAVEMBR_TRACE_004\n");
      
      try { image.write(&mbr, sizeof(CMbr), true); }
      catch ( CExceptions * excep )
	{
	  closeFilesSave(true, *options, &image, fDeviceFile);
          throw excep;
	}

      showDebug(9, "SAVEMBR_TRACE_005\n");
    }
  showDebug(9, "SAVEMBR_TRACE_006\n");

  // copy reserved for future data
  try
    {
      dwLength = 0; // length of data written in an extension

      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT000);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT001);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT002);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT003);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT004);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT005);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT006);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT007);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT008);
      FUTURE_WRITEDATA(MAGIC_BEGIN_EXT009);
    }
  catch ( CExceptions * excep )
    {
      closeFilesSave(true, *options, &image, fDeviceFile);
      throw excep;
    }

  // copy options to debug file
  showDebugOptions(options);

  g_interface -> StatusLine(i18n("Saving partition to the image file..."));

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
  CFSBase *FS = NULL;
  
  if ((strcmp(szFileSystem, "ext2fs") == 0) || (strcmp(szFileSystem, "ext3fs") == 0)) // if EXT2 / EXT3
    FS = new CExt2Part(szDevice, fDeviceFile, headMain.qwPartSize);
  else if (strncmp(szFileSystem, "reiserfs", 8) == 0) // if REISER
    FS = new CReiserPart(szDevice, fDeviceFile, headMain.qwPartSize);
  else if ((strncmp(szFileSystem, "fat16", 5) == 0) || (strncmp(szFileSystem, "fat32", 5) == 0)) // if FAT
    FS = new CFatPart(szDevice, fDeviceFile, headMain.qwPartSize);
  else if (strcmp(szFileSystem, "hpfs") == 0) // if HPFS
    FS = new CHpfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
  else if (strcmp(szFileSystem, "hfs") == 0) // if HFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFsBeta("HFS");
      FS = new CHfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(szFileSystem, "jfs") == 0) // if JFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFsBeta("JFS");
      FS = new CJfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(szFileSystem, "xfs") == 0) // if XFS
    {
      if (!options->bBatchMode)
	g_interface -> WarnFsBeta("XFS");
      FS = new CXfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(szFileSystem, "ufs") == 0) // if UFS
    {
      if (!options->bBatchMode)
	g_interface -> WarnFsBeta("UFS");
      FS = new CUfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(szFileSystem, "afs") == 0) // if AFS
    {
      if (!options->bBatchMode)
	g_interface -> WarnFsBeta("AFS");
      FS = new CAfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(szFileSystem, "ntfs") == 0) // if NTFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFS("NTFS");
      FS = new CNtfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else // if UNKNOWN
    THROW( ERR_WRONG_FS);

  if (FS)
    {
      showDebug(9, "begin of virtual saveToImage()\n");
      try { FS -> saveToImage(&image, options, &guiInfo); }
      catch ( CExceptions * excep )
        {
	  delete FS;
          guiInfo.destroyForm();
          closeFilesSave(true, *options, &image, fDeviceFile);
          showDebug(9, "end of virtual saveToImage(), failed\n");
          throw excep;
        }
      showDebug(9, "end of virtual saveToImage(), ok\n");
      
      delete FS;
    }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
  
  // ---- write tail 
  CMainTail tailMain;
  memset(&tailMain, 0, sizeof(CMainTail));

  image.writeMagic(MAGIC_BEGIN_TAIL);

  //strcpy(tailMain.szMagicString, MAGIC_TAIL);
  tailMain.qwCRC = image.get_qwCRC();
  tailMain.dwVolumeNumber = image.get_dwVolumeNumber();

  try { image.write(&tailMain, sizeof(CMainTail), false); }
  catch (CExceptions * excep)
    {
      guiInfo.destroyForm();
      closeFilesSave(true, *options, &image, fDeviceFile);
      throw excep;
    }

  showDebug(1, "Final 64 bits CRC = %llu = %Xh\n", image.get_qwCRC(),
	    image.get_qwCRC());

  // ---- Close files (end of all previous operations with success)
  guiInfo.destroyForm();
  closeFilesSave(false, *options, &image, fDeviceFile);

  RETURN;
}

// =======================================================
void restorePartition(char *szDevice, char *szImageName, COptions *options)
{
  BEGIN;

  int nRes;
  QWORD qwPartitionSize;
  CRestoreOptWindow guiOpt;
  CRestoringWindow guiInfo;
  char aux[MAXPATHLEN];
  DWORD dwLength;
  char cTemp;
  DWORD i;
  
  // structures to stock informations about the current partition
  CImage image(options);
  
  g_interface -> StatusLine(i18n("initializing the operation"));
  
  // ----------- system checks
  nRes = systemChecks(szDevice, options->bFailIfMounted);
  if (nRes == -1)
    THROW( ERR_FSCHK);
  
  // initialize interface
  if (options->bBatchMode == false)
    guiOpt.create(szDevice, szImageName, *options);
  
  // ----------- calculate filenames
  //image.set_dwCurrentVolumeSize(0L);
  image.set_dwVolumeNumber(0L);
  if (strcmp(szImageName, "stdin"))
    {
// filename without path
      extractFilenameFromFullPath(szImageName, aux);
      image.set_szOriginalFilename(aux);
// filepath without filename
      extractFilepathFromFullPath(szImageName, aux);
      image.set_szPath(aux);
      image.set_szImageFilename(szImageName);
    }
  else
    {
      image.set_szOriginalFilename("stdin");
      image.set_szPath("");
      image.set_szImageFilename("stdin");
    }

  // ----------- open partition file
  errno = 0;
  
  // ----------- get the image compression level
  //options->dwCompression = (DWORD) getCompressionLevelForImage(image.get_szImageFilename());

  // ----------- open the image file
  try { image.openReading(); }
  catch ( CExceptions * excep )
    { throw excep; }

  // the read buffer thread can now start working
  //increaseSemaphore(&g_semDataToProcess);
  
  // ----------- read header
  CMainHeader headMain;
  
  try{image.read((char*)&headMain, sizeof(CMainHeader), true);}
  catch (CExceptions * excep)
    {
      if (!excep->getCaught())
        g_interface -> ErrorReadingMainHeader();
      closeFilesRestore(true, *options, &image, NULL);
      throw excep;
    }

  // encryption is not supported
  if (headMain.dwEncryptAlgo != ENCRYPT_NONE)
    {
      closeFilesRestore(true, *options, &image, NULL);
      THROW(ERR_ENCRYPT);
    }  

  // read CRC of CMainHeader
  image.readAndCheckCRC((char*)&headMain, sizeof(CMainHeader));

  // ---- skip the MBR data

  // 1. check the magic number
  try {image.readAndCheckMagic(MAGIC_BEGIN_MBRBACKUP); }
  catch (CExceptions * excep)
    {
      closeFilesRestore(true, *options, &image, NULL);
      throw excep;
    }

  // 2. skip contents
  CMbr mbr;

  for (i=0; i <headMain.dwMbrCount; i++)
    {
      try{image.read((char*)&mbr, sizeof(CMbr), true);}
      catch (CExceptions * excep)
	{
	  closeFilesRestore(true, *options, &image, NULL);
          throw excep;
	}
    }

  // 3. skip reserved for future data
  try 
    {
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT000);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT001);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT002);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT003);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT004);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT005);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT006);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT007);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT008);
      FUTURE_SKIPDATA(MAGIC_BEGIN_EXT009);
    }
  catch (CExceptions * excep)
    {
      closeFilesRestore(true, *options, &image, NULL);
      throw excep;
    }

  // ----------- check header data
  /*if (strcmp(headMain.szAppName, "PartImage") != 0)
    {	
    // is it a volume header ?
    if (strcmp(headMain.szAppName, "Partimage-volume") == 0)
    g_interface -> ErrorAskFirstVolume(szImageName);
    else
    g_interface -> ErrorInvalidImagefile(szImageName);
      
    closeFilesRestore(true, *options, &image, fDeviceFile);
    RETURN_int(-1);
    }*/
  
  // ----------- check image version
  /*
    if (strcmp(headMain.szVersion, CURRENT_IMAGE_FORMAT) > 0)
    {	
    RETURN_int(-1);
    }
    else if (strcmp(headMain.szVersion, CURRENT_IMAGE_FORMAT) < 0)
    {	
    RETURN_int(-1);
    }
  */
  
  //image.set_bIsSplitted(!!(headMain.dwMainFlags & FLAG_SPLITTED));
  
  // ----------- image ?= splitted
  //image.set_szOriginalFilename(headMain.szFirstImageFilepath, MAXPATHLEN);
  //image.set_qwIdentificator(headMain.qwIdentificator);
  //image.set_bIsSplitted( (bool) (headMain.dwMainFlags & FLAG_SPLITTED));
  
  // check the destination partition is large enough to restore data
  qwPartitionSize = getPartitionSize(szDevice);
  if (qwPartitionSize < headMain.qwPartSize)
    THROW(ERR_PART_TOOSMALL, headMain.qwPartSize, qwPartitionSize);

  // -------- show the description
  if (options->bBatchMode == false)
    {
      headMain.szPartDescription[MAX_DESCRIPTION-1] = 0;
      if (*headMain.szPartDescription)
	g_interface->msgBoxOk(i18n("Partition description"), headMain.szPartDescription);
      
      // show operation window
      nRes = guiOpt.runForm();
      guiOpt.getValues(options);
      guiOpt.destroyForm();

      if (nRes == KEY_EXIT)
        THROW(ERR_CANCELED);
      if (nRes == KEY_BACK)
        THROW(ERR_COMEBACK);
      /*if (nRes == -1)
        throw neew CExceptions("restorePartition", ERR_CANCELED);*/ 
      
      // check options
      nRes = checkOptions(*options, szDevice, szImageName);
      if (nRes == -1)
        THROW(ERR_CANCELED); 
    }

  // copy options to debug file
  showDebugOptions(options);

  // open the partition file
  FILE *fDeviceFile;
  if (options->bSimulateMode)
    g_interface -> WarnSimulate();

  if (options->bSimulateMode)
    fDeviceFile = fopen(szDevice, "rb"); // read only mode
  else
    fDeviceFile = fopen(szDevice, "r+b");// read write mode (open at beginning)

  if (fDeviceFile == NULL)
    THROW(ERR_OPENING_DEVICE, errno);
  

  // lock the partition
  try { lockFile(fileno(fDeviceFile), szDevice); }
  catch ( CExceptions * excep )
    { throw excep; }


  // --------- initialize info window -------------
  guiInfo.create(szDevice, szImageName, qwPartitionSize, options->dwCompression, headMain.szOriginalDevice, headMain.szFileSystem, headMain.dateCreate, headMain.qwPartSize, options);
  image.setGuiRestore(&guiInfo);
  
  /*if (image.get_bIsSplitted())
    guiInfo.showSplittedInfo(i18n("The image was splitted"));
    else
    guiInfo.showSplittedInfo(i18n("The image was not splitted")); */
  
  //guiInfo.showPartitionInfo(szDevice, qwPartitionSize);
  
  // ----------- print informations about image file
  //guiInfo.showPartitionInfos(options->dwCompression, headMain.szOriginalDevice, headMain.szFileSystem, headMain.dateCreate, headMain.qwPartSize);
  guiInfo.showImageFileInfo(szImageName, -1, options->szFullyBatchMode);

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
  CFSBase *FS = NULL; 

  if ((strcmp(headMain.szFileSystem, "ext2fs") == 0) || (strcmp(headMain.szFileSystem, "ext3fs") == 0)) // if EXT2 / EXT3
    FS = new CExt2Part(szDevice, fDeviceFile, headMain.qwPartSize);
  else if (strncmp(headMain.szFileSystem, "reiserfs", 8) == 0) // if REISER
    FS = new CReiserPart(szDevice, fDeviceFile, headMain.qwPartSize);
  else if ((strncmp(headMain.szFileSystem, "fat16", 5) == 0) || (strncmp(headMain.szFileSystem, "fat32", 5) == 0)) // if FAT
  //else if (strncmp(headMain.szFileSystem, "fat", 3) == 0) // if FAT
    FS = new CFatPart(szDevice, fDeviceFile, headMain.qwPartSize);
  else if (strcmp(headMain.szFileSystem, "hpfs") == 0) // if HPFS
    FS = new CHpfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
  else if (strcmp(headMain.szFileSystem, "hfs") == 0) // if HFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFsBeta("HFS");
      FS = new CHfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(headMain.szFileSystem, "jfs") == 0) // if JFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFsBeta("JFS");
      FS = new CJfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(headMain.szFileSystem, "xfs") == 0) // if XFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFsBeta("XFS");
      FS = new CXfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(headMain.szFileSystem, "afs") == 0) // if AFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFsBeta("AFS");
      FS = new CAfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(headMain.szFileSystem, "ntfs") == 0) // if NTFS
    {
      if (!options->bBatchMode)
        g_interface -> WarnFS("NTFS");
      FS = new CNtfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else if (strcmp(headMain.szFileSystem, "ufs") == 0) // if UFS
    {
      if (!options->bBatchMode)
	g_interface -> WarnFsBeta("UFS");
      FS = new CUfsPart(szDevice, fDeviceFile, headMain.qwPartSize);
    }
  else // if UNKNOWN
    THROW(ERR_WRONG_FS);

  if (FS)
    {
      showDebug(1, "begin of virtual saveToImage()\n");
      try { FS -> restoreFromImage(&image, options, &guiInfo); }
      catch ( CExceptions * excep )
        {
          showDebug(9, "end of virtual saveToImage(), failed\n");
          closeFilesRestore(true, *options, &image, fDeviceFile);
          delete FS;
          throw excep;
        }

      showDebug(9, "end of virtual saveToImage(), ok\n");
      delete FS;
    }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
  
  // ---- check tail
  CMainTail tailMain;
  
  try
    { 
      image.readAndCheckMagic(MAGIC_BEGIN_TAIL);
      image.read((char*)&tailMain, sizeof(CMainTail), false);
    }
  catch (CExceptions * excep)
    {
      showDebug(1,"ERROR reading MAGIC-BEGIN-TAIL\n");
      nRes = g_interface -> msgBoxContinueCancel(i18n("Error in Main Tail"),
		i18n("Impossible to read the magic: the image is damaged"));
      if (nRes != MSGBOX_CONTINUE)
        {
          closeFilesRestore(true, *options, &image, fDeviceFile);
          throw excep;
        }
    }
  
  // check CRC 64
  if (image.get_qwCRC() != tailMain.qwCRC)
    {
      showDebug(1, "CRC64 are differents: image.get_qwCRC()=%llu, tailMain.qwCRC=%llu\n",image.get_qwCRC(), tailMain.qwCRC);
      closeFilesRestore(true, *options, &image, fDeviceFile);
      nRes = g_interface -> msgBoxContinueCancel(i18n("Error in Main Tail"),
		i18n("CRC changed: the image is damaged"));
      if (nRes != MSGBOX_CONTINUE)
        THROW(ERR_CHECK_CRC, image.get_qwCRC(), tailMain.qwCRC);
    }
  
  // check volumes number
  if (image.get_dwVolumeNumber() != tailMain.dwVolumeNumber)
    {
      showDebug(1, "volume numbers are differents: image.get_dwVilumeNumber()=%lu, tailMain.dwVolumeNumber=%lu\n",image.get_dwVolumeNumber(), tailMain.dwVolumeNumber);
      nRes = g_interface -> msgBoxContinueCancel(i18n("Error in Main Tail"),
		i18n("volume numbers changed: the image is damaged"));
      if (nRes != MSGBOX_CONTINUE)
        {
          closeFilesRestore(true, *options, &image, fDeviceFile);
          THROW(ERR_VOLUMENUMBER, image.get_dwVolumeNumber(), tailMain.dwVolumeNumber);
        }
    }

  // ---- close files
  closeFilesRestore(false, *options, &image, fDeviceFile);

  RETURN;
}

// =======================================================
// on_error: call with true only if error is detected
void closeFilesSave(bool on_error, COptions options, CImage *image, FILE *fDeviceFile)
{
  BEGIN;
  
  if (fDeviceFile)
    {
      if (flock(fileno(fDeviceFile), LOCK_UN) == -1)
        THROW(ERR_ERRNO, errno);
      fclose(fDeviceFile);
      fDeviceFile = 0;
    }

  //if (!on_error) (Removed this line in order to fix a bug in 0.6.0-final: the '*' key made pi to freeze, and the thread2 was not closed)
    {
      try { image->closeWriting(); }
      catch ( CExceptions * excep)
        { throw excep; }
    }
  RETURN;
}

// =======================================================
// on_error: call with true only if error is detected
void closeFilesRestore(bool on_error, COptions options, CImage *image, FILE *fDeviceFile)
{
  BEGIN;

  if (fDeviceFile)
    {
      if (flock(fileno(fDeviceFile), LOCK_UN) == -1)
	      showDebug(1, "WARNING: unlocking file failed %s\n", strerror(errno));
//        THROW(ERR_ERRNO, errno);
      fclose(fDeviceFile);
      fDeviceFile = 0;
    }

  if (!on_error)	// close was called after an error was detected
    {
      try { image->closeReading(); }
      catch ( CExceptions * excep)
      { showDebug(1, "WARNING: closeReading failed\n"); }
//        { throw excep; }
    }
  RETURN;
}

// =============================================================================
bool isFileSystemSupported(char *szFileSystem)
{
  char *szPtr;
  char szTemp[1024];
  WORD i;

  showDebug(1, "SUPPORTED_FS=[%s]\n", g_szSupportedFileSystems);
  
  szPtr = g_szSupportedFileSystems;

  while (*szPtr)
    {
      // extract current name
      memset(szTemp, 0, sizeof(szTemp));
      for (i=0; (i < sizeof(szTemp)) && (*szPtr) && (*szPtr != '*') && (*szPtr != ','); i++, szPtr++)
	szTemp[i] = *szPtr;
      if (*szPtr)
	szPtr++;

      // compare names
      if (strncmp(szTemp, szFileSystem, sizeof(szTemp)) == 0)
	return true;
    }

  return false;
}

// =============================================================================
// always LOCK_EX because here we lock only partitions
// images are locked in CImageDisk.
void lockFile(int nFileDesc, char *szDevice)
{
  BEGIN;
  
  int nRes;

  errno = 0;
  nRes = flock(nFileDesc, LOCK_EX | LOCK_NB);
  
  if (nRes == -1)
    {
      showDebug(1, "lockFile: (%d) %s already locked\n", nFileDesc, szDevice);
      THROW(ERR_LOCKED, szDevice);
    }
  
  RETURN;
}

// =======================================================
void initCrcTable(DWORD *dwCrcTable)
{
  DWORD c;
  unsigned int n, k;
  DWORD poly;            // polynomial exclusive-or pattern 
  // terms of polynomial defining this crc (except x^32): 
  static const BYTE p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};
  
  // make exclusive-or pattern from polynomial (0xedb88320L) 
  poly = 0L;
  for (n = 0; n < sizeof(p)/sizeof(BYTE); n++)
    poly |= 1L << (31 - p[n]);
  
  for (n = 0; n < 256; n++)
    {
      c = (DWORD)n;
      for (k = 0; k < 8; k++)
	c = c & 1 ? poly ^ (c >> 1) : c >> 1;
      dwCrcTable[n] = c;
    }
}

// =======================================================
// copy options to debug file
void showDebugOptions(COptions *options)
{
  showDebug(1, "\n\n============= begin OPTIONS =====================\n"
	    "options.bOverwrite = %d\n" 
	    //"options.dwSplitMode = %lu\n" 
	    "options.qwSplitSize = %llu\n" 
	    "options.bEraseWithNull = %d\n" 
	    "options.dwCompression = %lu\n" 
	    "options.bCheckBeforeSaving = %d\n"
	    "options.bFailIfMounted = %d\n" 
	    "options.bAskDesc = %d\n" 
	    "options.dwFinish = %lu\n" 
	    "options.szServerName = %s\n" 
	    "options.dwServerPort = %lu\n" 
	    "options.bBatchMode = %d\n" 
	    "options.bSplitWait = %d\n" 
	    "options.bSync = %d\n" 
	    "options.bUseSSL = %d\n" 
	    "options.dwDebugLevel = %lu\n" 
	    "options.bBackupMBR = %d\n" 
	    "options.bSimulateMode = %d\n" 
	    "options.szAutoMount = %s\n" 
	    "============= end OPTIONS =====================\n\n\n",
	    (int) options->bOverwrite, 
	    //options->dwSplitMode,
	    options->qwSplitSize, (int) options->bEraseWithNull,
	    options->dwCompression, (int) options->bCheckBeforeSaving,
	    (int)options->bFailIfMounted, (int) options->bAskDesc, 
	    options->dwFinish, options->szServerName, 
	    options->dwServerPort, (int) options->bBatchMode, (int)options->bSplitWait,
	    (int)options->bSync, (int)options->bUseSSL, options->dwDebugLevel, 
	    (int)options->bBackupMBR, (int)options->bSimulateMode, options->szAutoMount);
}

// =======================================================
/*bool isMountedOld(const char *szDevice, char *szMountPoint)
{
  FILE * f;
  struct mntent * mnt;
  bool bMounted = false;
  
  if ((f = setmntent (MOUNTED, "r")) == 0)
    RETURN_int(-1);
  
  while ((mnt = getmntent (f)) != 0)
    {	
      if (strcmp(szDevice, mnt->mnt_fsname) == 0)
	{
	  bMounted = true;
	  strcpy(szMountPoint, mnt->mnt_dir);
	}
    }
  endmntent (f);
  
  return bMounted;
}*/

// =======================================================
/*int isMountedVer1(const char *szDevice, char *szMountPoint)
{
  FILE * f;
  struct mntent * mnt;
  bool bMounted = false;
  char buf [MAXPATHLEN+1];
  char rbuf [MAXPATHLEN+1];
  char * ptrDevice;
  
  if ((f = setmntent (MOUNTED, "r")) == 0)
    RETURN_int(-1);
  
  while (((mnt = getmntent (f)) != 0) && (!bMounted))
    {
      ptrDevice = (char*) szDevice;
      if (realpath(ptrDevice, rbuf) != NULL)
	ptrDevice = rbuf;
      
      //IS_MOUNTED(ptrDevice, mnt->mnt_fsname);
      if (strcmp(ptrDevice, mnt->mnt_fsname) == 0)
	{
	  bMounted = true; 
	  strcpy(szMountPoint, mnt->mnt_fsname); 
	} 
      
      memset(buf, 0, MAXPATHLEN);
      if (readlink (szDevice, buf, MAXPATHLEN-1) != -1)
	{
	  if (realpath(buf, rbuf) != NULL)
	    buf = rbuf;
	  
	  //IS_MOUNTED(buf, mnt->mnt_fsname);
	  if (strncmp(buf, mnt->mnt_fsname, sizeof(buf)) == 0)
	    { 
	      bMounted = true; 
                 strcpy(szMountPoint, mnt->mnt_fsname);
	    } 
	}
      
      if (strcmp(szDevice, mnt->mnt_fsname) == 0)
	{
	  bMounted = true;
	  strcpy(szMountPoint, mnt->mnt_dir);
	}
    }

  endmntent (f);
  
  return bMounted;
}*/


// =======================================================
#ifdef OS_LINUX
int isMounted(const char *szDevice, char *szMountPoint)
{
  FILE *f;
  struct mntent *mnt;
  bool bMounted;
  char szBuf1 [MAXPATHLEN+1];
  char szBuf2 [MAXPATHLEN+1];
  char *ptrDevice;

  // init
  bMounted = false;
  memset(szBuf1, 0, MAXPATHLEN);
  memset(szBuf2, 0, MAXPATHLEN);

  if ((f = setmntent (MOUNTED, "r")) == 0)
    RETURN_int(-1);

  // relative path -> full path [result in ptrDevice]
  ptrDevice = (char*)szDevice;
  if (realpath(ptrDevice, szBuf1) != NULL)
    ptrDevice = szBuf1;

  // symbolic link -> regular device file [result in ptrDevice]
  if (readlink (ptrDevice, szBuf2, MAXPATHLEN-1) != -1)
    ptrDevice = szBuf2;

  showDebug(3, "isMounted(%s) = isMounted(%s)\n",szDevice, ptrDevice);
      
  // loop: compare device with each mounted device
  while (((mnt = getmntent (f)) != 0) && (!bMounted))
    {
      if (strcmp(ptrDevice, mnt->mnt_fsname) == 0)
	{
	  bMounted = true; 
	  strcpy(szMountPoint, mnt->mnt_dir); 
	  showDebug(3, "partition [%s] mounted on [%s]\n",mnt->mnt_fsname,szMountPoint);
	} 
    }

  endmntent (f);
  
  return bMounted;
}
#else // OS_LINUX
int isMounted(const char *szDevice, char *szMountPoint)
{
  #warning isMounted always return false
  return 0;
}
#endif

// =======================================================
// return true if this is a stable version (x.y.z with y even)
//        false           an unstable                  y odd     
bool isVersionStable(const char *szVersion)
{
  int i;
  char szMinor[512];

  memset(szMinor, 0, sizeof(szMinor));
  for (; *szVersion && *szVersion != '.'; szVersion++);
  szVersion++; // skip the '.'
  for (i=0; szVersion[i] && szVersion[i] != '.'; i++)
    szMinor[i] = szVersion[i]; // copy the minor

  return ((atoi(szMinor) % 2) == 0);
}

// =======================================================
QWORD getPartitionSize(const char *szDevice)
{
  QWORD qwSize;
  long lSize;
  int fdDevice;
  
  qwSize = 0;
  
  fdDevice = open(szDevice, O_RDONLY);
  if (fdDevice < 0)
    return 0LL;
 
#ifdef OS_LINUX 
  if (ioctl(fdDevice, BLKGETSIZE, &lSize) >= 0)
    qwSize = ((QWORD) 512) * ((QWORD) lSize);
#endif
#ifdef OS_FBSD
  #warning qwSize not initialized
#endif
  
  close(fdDevice);
  return qwSize;
}

// =======================================================
int showUfsDisklabel(char *szDevice)
{
  FILE *fStream;
  char cBoot[512];
  CUFSDiskLabel dl;
  int i, j;
  int nRes;
  CUFSPartition part;
  char letter='a';
  char szTemp[512];

  // open the partition
  fStream = fopen(szDevice, "rb");
  if (!fStream)
    {
      RETURN_int(-1);
    }

  // read the super block
  nRes = fread(cBoot, 512, 1, fStream);
  if (nRes != 1)
    {
      RETURN_int(-1);
    }

  // TODO: check the magic number
  /*DWORD dwUfsMagic = UFS_MAGIC;
  if (memcmp(cBuffer+5678, &dwUfsMagic, 4) != 0)
    {
      fclose(fStream);
      RETURN_int(-1);
    }*/

  //for (i=0; (dl.d_magic == MAGIC) || (i==0); i++)
    {
      nRes = fread(&dl, sizeof(CUFSDiskLabel), 1, fStream);
      if (nRes != 1)
	  RETURN_int(-1);

      printf("%s\n", dl.d_typename);
      
      for (j=0; j < dl.d_npartitions; j++)
	{
	  SNPRINTF(szTemp, "%s%c", dl.d_typename, letter);
	  part = dl.d_partitions[j];
	  /*printf("    %s: sectors=%lu, startsect=%lu, fragsize=%lu, fragperblk=%lu, type=%s\n", szTemp, (DWORD)part.p_size, (DWORD)part.p_offset, (DWORD)part.p_fsize, (DWORD)part.p_frag, fstypenames[part.p_fstype]);*/
	  letter++;
	}
	  
      i++;
    } 

  // close the partition
  fclose(fStream);
  return 0;
}

// =======================================================
int detectFileSystem(const char *szDevice, char *szFileSys)
{
  FILE *fDeviceFile;
  int nRes;
  const int nBufSize = 128 * 1024; // 128 KB
  BYTE cBuffer[nBufSize];

  BEGIN;

  // init
  strcpy(szFileSys, "-unknown-");

  // 0. Open device
  fDeviceFile = fopen(szDevice, "rb");
  if (fDeviceFile == NULL)
    {
      showDebug(4, "cannot open device [%s]\n", szDevice);
      RETURN_int(-1);
    }

  showDebug(4, "001 = after fopen\n");
  showDebug(4, "======= DETECT FOR %s =========\n", szDevice);

  // 1. Read boot sector / beginning of the memory
  nRes = fseek(fDeviceFile, 0L, SEEK_SET);
  if (nRes == -1)
    RETURN_int(-1);

  nRes = fread(cBuffer, nBufSize, 1, fDeviceFile);
  if (nRes != 1)
    RETURN_int(-1);
  
  showDebug(4, "002 = after fread\n");

  // **** check for UFS file system {magic @ 72020}
  nRes = CUfsPartdetect(cBuffer, szFileSys, fDeviceFile);
  if (nRes > 0)
    return nRes;

  /*DWORD dwUfsMagic = UFS_MAGIC;
    if (memcmp(cBuffer+512, &dwUfsMagic, 4) == 0)
    {
      strcpy(szFileSys, "ufs");
      return FS_UFS;
    }
  */
  
  fclose(fDeviceFile);

  showDebug(4, "003 = after UFS\n");

  // **** check for REISER file system [magic @ 65536]
  nRes = CReiserPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "004 = after reiser\n");

  //  **** check for JFS file system [magic @ 32768]
  nRes = CJfsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "005 = after Jfs\n");
  
  //  **** check for AFS file system [magic @ 1092]
  nRes = CAfsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "006 = after Afs\n");

  //  **** check for EXT2FS file system [magic @ 1080]
  nRes = CExt2Partdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "007 = after Ext2\n");

  //  **** check for HFS file system [magic @ 1024]
  nRes = CHfsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "008 = after Hfs\n");

  //  **** check for BeFS file system [magic @ 544]
  nRes = CBefsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "009 = after Befs\n");

  //  **** check for NTFS file system [magic @ 3]
  nRes = CNtfsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "010 = after Ntfs\n");

  // **** check for HPFS file system [magic @ 54]
  nRes = CHpfsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "011 = after Hpfs\n");

  //  **** check for XFS file system  [magic @ 0]
  nRes = CXfsPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;

  showDebug(4, "012 = after Xfs\n");
  
  // **** check for swap space
  char cTemp[512];
  memset(cTemp, 0, 511);
  memcpy(cTemp, cBuffer+getpagesize()-10, 10);
  //*(cTemp+10) = '\0';

  if (strcmp(cTemp, "SWAP-SPACE")==0)
    {
      strcpy(szFileSys, "swap (v0)");
      return FS_SWAP0;
    }
  else if (strcmp(cTemp, "SWAPSPACE2")==0)
    {
      strcpy(szFileSys, "swap (v1)");
      return FS_SWAP1;
    }

  showDebug(4, "013 = after swap\n");

  // **** check for FAT file system
  nRes = CFatPartdetect(cBuffer, szFileSys);
  if (nRes > 0)
    return nRes;
      
  showDebug(4, "014 = after FAT\n");

  // G. Extended ==> TODO
  /*if (0)
    {
      fclose(fDeviceFile);
      strcpy(szFileSys, "extended");
      delete []cBuffer;
      return FS_EXTENDED;
    }*/
    
  // end on error
  RETURN_int(-1);
}

// =======================================================
int getMajorMinor(char *szDevice, int *nMajor, int *nMinor)
{
  char szTemp[MAXPATHLEN];
  struct stat fStat;
  int i, j;
  int nRes;

  // get the major/minor number of the device
  memset(szTemp, 0, MAXPATHLEN);

  strncpy(szTemp, szDevice, MAXPATHLEN-1);
  szTemp[MAXPATHLEN-1] = '\0';
  i = strlen(szTemp)-1;
  while (i && isdigit(szTemp[i]))
    szTemp[i--] = '\0';

// devfs: change part into disc
  if (i > 3 && !strncmp("part", szTemp+i-3, 4)) 
    strcpy(szTemp+i-3, "disc");

  showDebug(1, "getMajorMinor: device=%s\n", szTemp);

  nRes = stat(szTemp, &fStat);
  if (nRes == -1)
    {
      showDebug(1, "stat failed: %d\n", errno);
      RETURN_int(-1);
    }
  
  *nMajor = major(fStat.st_rdev);
  *nMinor = minor(fStat.st_rdev); // it is often 64 (for hdb, hdd, ...) or 0 (hda, hdc, ...)
  
  // get the minor number
  memset(szTemp, 0, MAXPATHLEN);
  for (i=0; (i < MAXPATHLEN) && (szDevice[i]) && (!isdigit(szDevice[i])); i++);
  for (j=0; (i < MAXPATHLEN) && (szDevice[i]) && (isdigit(szDevice[i])); i++, j++)
    szTemp[j] = szDevice[i];
  *nMinor += atoi(szTemp);

  //debugWin("for device %s, maj=%d and min=%d",szDevice,*nMajor,*nMinor);

  return 0;
}

// =======================================================
int checkInodeForDevice(char *szDevice) // NEW FUNCTION
{
  int nRes;
  struct stat fStat;
  int nMajor, nMinor;

  // must fail if the szDevice does not begins with "/dev/"
  if (strncmp(szDevice, "/dev/", 5) != 0)
    RETURN_int(-1);

  // check the inode exists (for example, hda17 can be missing in /dev)
  nRes = access(szDevice, F_OK);
  if (nRes == -1)
    {
      nRes = g_interface->msgBoxYesNo(i18n("inode missing"), i18n("%s inode doesn't exists. Partimage can create it for you. You can also use the manual mknod "
								  "command. Do you want this inode to be created for you now ?"), szDevice);
      if (nRes == MSGBOX_NO)
	  RETURN_int(-1);

      nRes = getMajorMinor(szDevice, &nMajor, &nMinor);
      if (nRes == -1)
	RETURN_int(-1);

      // create the inode
      nRes = mknod(szDevice, S_IFBLK | S_IREAD | S_IWRITE | S_IRGRP | S_IROTH, makedev(nMajor, nMinor));
    }	
  
  // check the device is a block one
  nRes = stat(szDevice, &fStat);
  if (nRes == -1 || !S_ISBLK(fStat.st_mode))
    {
      g_interface->msgBoxError(i18n("%s is not a block device."), szDevice);
      RETURN_int(-1);
    }	
  
  return 0;
}

// =======================================================
int bigFseek(FILE *fStream, QWORD qwOffset) // SEEK_SET forced
{
  const long lGigaByte = (QWORD) (1024L * 1024L * 1024L);
  int nRes;
  
  // go to the beginning
  nRes = fseek(fStream, 0L, SEEK_SET);
  
  while (qwOffset > 0)
    {
      //showDebug(1, "big_fseek: qwOffset=%llu\n", qwOffset);
      
      if (qwOffset >= (QWORD)lGigaByte)
	{
	  nRes = fseek(fStream, (long)lGigaByte, SEEK_CUR);
	  //showDebug(1, "big_fseek: big seek\n");
	  if (nRes == -1)
	    RETURN_int(-1);
	  qwOffset -= ((QWORD)lGigaByte);	
	}
      else
	{
	  //showDebug(1, "big_fseek: small seek: %llu\n", qwOffset);
	  nRes = fseek(fStream, (long)qwOffset, SEEK_CUR);
	  return nRes;
	}
    }
  
  return 0;
}

/*
 * -1: fatal error
 *  1: devfs mounted
 *  0: devfs not mounted
 */

int isDevfsMounted()
{
	FILE *f;
	char buf[256];

	if (access("/proc/mounts",R_OK)==-1)
	{
		g_interface->msgBoxError(i18n("You haven't access to /proc/mounts."));
		return(-1);
	}

	if ((f=fopen("/proc/mounts","r"))==NULL)
	{
		g_interface->msgBoxError(i18n("An error occurred trying to read /proc/mounts."));
		return(-1);
	}

	while (!feof(f))
	{
		fscanf(f,"%255s",buf); *(buf+255)='\0';
		if (!strcmp(buf,"devfs"))
		{
			fclose(f);
			return(1);
		}
	}

	fclose(f);
	return(0);
}




// =======================================================
int isDevfsEnabled()
{
  FILE *fPart;
  char cBuffer[32768];
  char *cPtr;
  int nSize;
  
  errno = 0;
  fPart = fopen("/proc/partitions", "rb");
  if (!fPart)
    {	
      g_interface->msgBoxError(i18n("Cannot read \"/proc/partitions\" (%s). "
         "Then, you must use the "
	 "command line to run Partition Image. Type \"partimage --help\" "
         "for help."), strerror(errno));
      RETURN_int(-1);	
    }
  
  nSize = 0;
  memset(cBuffer, 0, 32767);
  while (!feof(fPart))
    {
      cBuffer[nSize] = fgetc(fPart);
      nSize++;
    }
  
  cPtr = cBuffer;
  
  fclose(fPart);

  if (strstr(cBuffer, "part"))
    return 1; // devfs is enabled
  else
    return 0; // devfs is disabled
}

// =======================================================
char *decodePartitionEntry(char *cPtr, int *nMajor, int *nMinor,
   int *nBlocks, int *nPartNum, char *szDevice)
{
  char cTemp[128];
  char *cCur;
  char *cDevPtr1;
  char *cDevPtr2;

  showDebug(9, "start decoding entry\n");
 
  // Major number
  while (!isalnum(*cPtr))
    cPtr++;
  memset(cTemp, 0, 127);
  cCur = cTemp;
  while(isalnum(*cPtr))
    {
      *cCur = *cPtr;
      cCur++;
      cPtr++;
    }
  *nMajor = atoi(cTemp);
  
  // Minor number
  while (!isalnum(*cPtr))
    cPtr++;
  memset(cTemp, 0, 127);
  cCur = cTemp;
  while(isalnum(*cPtr))
    {
      *cCur = *cPtr;
      cCur++;
      cPtr++;
    }
  *nMinor = atoi(cTemp);
  
  // Block number
  while (!isalnum(*cPtr))
    cPtr++;
  memset(cTemp, 0, 127);
  cCur = cTemp;
  while(isalnum(*cPtr))
    {
      *cCur = *cPtr;
      cCur++;
      cPtr++;
    }
  *nBlocks = atoi(cTemp);
  
  // Device
  while (!isalnum(*cPtr))
    cPtr++;
  memset(szDevice, 0, 127);
  cCur = szDevice;
  while(isalnum(*cPtr) || *cPtr == '/')
    {
      *cCur = *cPtr;
      cCur++;
      cPtr++;
    }
  
  // go to the next line
  while((*cPtr != '\n') && (*cPtr != 0))
    cPtr++;
  
  // extract number from device
  cDevPtr1 = szDevice;
  cDevPtr2 = cTemp;
  memset(cTemp, 0, 127);
  strcpy(cTemp, "0");
  
  char *cDevTmpPtr = strrchr(cDevPtr1, '/');
  if (cDevTmpPtr)
    cDevPtr1 = ++cDevTmpPtr;
  
  while (isalpha(*cDevPtr1)) // skip letters
    {
      cDevPtr1++;
    }
  while (isdigit(*cDevPtr1))
    {
      *cDevPtr2 = *cDevPtr1;
      cDevPtr1++;
      cDevPtr2++;
    }
  *nPartNum = atoi(cTemp);
  
  showDebug(9, "entry found: %s %d %d %d %d\n", szDevice, nMajor, nMinor,
    nBlocks, nPartNum); 
 
  return cPtr;
}

// =======================================================
int systemChecks(char *szDevice, bool bFailIfMounted)
{
  char szMountPoint[1024];
  int nRes;
  
  // ----------- check for inode
  nRes = checkInodeForDevice(szDevice);
  if (nRes == -1)
    RETURN_int(-1);	
  
  // ----------- check the user is logged as root
  if (geteuid() != 0) // 0 is the UID of the root
    {	
      nRes = g_interface->msgBoxContinueCancel(i18n("Warning"), i18n("You are not logged as root. You may have \"access denied\" errors when working."));	
      if (nRes == MSGBOX_CANCEL)
	RETURN_int(-1);
    }		
  
  // ----------- check the partition is not mounted
  if ((bFailIfMounted == true) && (isMounted(szDevice, szMountPoint) != false))
    {
      nRes = g_interface->msgBoxContinueCancel(i18n("Error"), i18n("The %s partition is mounted. Partition Image can't work on mounted partitions. "
								   "Please, unmount it before. You can do it with \"umount %s\""), szDevice, szMountPoint);
      if (nRes == MSGBOX_CANCEL)
	RETURN_int(-1);
    }	
  
  return 0;
}

// =======================================================
int checkOptions(COptions options, char *szDevice, char *szImageFile)
{
  int i;
  int nCount;
  struct stat fStat;

  // incorrect split size
  //if ((options.dwSplitMode == SPLIT_SIZE) && (options.qwSplitSize < (QWORD)(1024*1024)))
  if ((options.qwSplitSize) && (options.qwSplitSize < (QWORD)(1024*1024)))
    {
      g_interface->msgBoxError(i18n("Split volumes size must be an integer greater than 1024 (size >= 1024 KB)"));	
      RETURN_int(-1);
    }

  // invalid port number
  if ((options.dwServerPort < 1) || (options.dwServerPort > 65535))
    {	
      g_interface->msgBoxError(i18n("server's port must be between 1 and 65535\n"));	
      RETURN_int(-1);
    }

  // check mount options
  if (*options.szAutoMount) // if enabled
    {
      nCount=0;
      for (i=0; (i < MAXPATHLEN) && (options.szAutoMount[i]); i++)
	if (options.szAutoMount[i] == '#')
	  nCount++;
      if (nCount != 2)
	{
	  g_interface->msgBoxError(i18n("Invalid automount options. Read the manual for more details"));	
	  RETURN_int(-1);
	}
    }

  // check for a valid device
  if ((szDevice) && (*szDevice) && ((strncmp(szDevice, "/dev/", 5) != 0) || ((stat(szDevice, &fStat) != -1) && (!S_ISBLK(fStat.st_mode)))))
    {
      g_interface->msgBoxError(i18n("The second argument [%s] must be a valid device (ex: /dev/hda1)"), szDevice);	
      RETURN_int(-1);
    }

  // check for an image file
  if ((szImageFile) && (*szImageFile) && (stat(szImageFile, &fStat) != -1))
    {
      if (!S_ISREG(fStat.st_mode))
	{
	  g_interface->msgBoxError(i18n("The third argument [%s] must be a valid regular file (not a directory, a symlink, ...)"), szImageFile);	
	  RETURN_int(-1);
	}

    }

  return 0;
}

// =======================================================
int CReiserPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;
  
  if (memcmp(cBootSect+REISERFS_DISK_OFFSET_IN_BYTES+52, REISERFS_SUPER_MAGIC_STRING, strlen(REISERFS_SUPER_MAGIC_STRING)) == 0)	
    {
      strcpy(szFileSystem, "reiserfs-3.5");
      return FS_REISER_3_5;
    }
  
  showDebug(4, "001 = after check for Reiser-3.5\n");
  
  if (memcmp(cBootSect+REISERFS_DISK_OFFSET_IN_BYTES+52, REISER2FS_SUPER_MAGIC_STRING, strlen(REISER2FS_SUPER_MAGIC_STRING)) == 0)	
    {
      strcpy(szFileSystem, "reiserfs-3.6");
      return FS_REISER_3_6;
    }
  
  showDebug(4, "002 = after check for Reiser-3.6\n");

  RETURN_int(0); // Not a REISER partition
}

// =======================================================
int CExt2Partdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  //WORD wTemp;
  CExt2Super sb;

  memcpy(&sb, cBootSect+1024, sizeof(sb));
  //memcpy(&wTemp, cBootSect+0x0438, sizeof(WORD));

  if (sb.s_magic == CpuToLe16(EXT2_SUPER_MAGIC))
    {
      // is it ext2fs or ext3fs ?
      if (sb.s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL)
	{
	  strcpy(szFileSystem, "ext3fs");
	  RETURN_int(FS_EXT3);
	}
      else
	{
	  strcpy(szFileSystem, "ext2fs");
	  RETURN_int(FS_EXT2);
	}
    }
  else
    RETURN_int(0);
}

// =======================================================
int CJfsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  DWORD dwTemp;

  memcpy(&dwTemp, cBootSect+32768, sizeof(DWORD));
  if (dwTemp == CpuToLe32(JFS_SUPER_MAGIC))
    {
      strcpy(szFileSystem, "jfs");
      RETURN_int(FS_JFS);
    }
  else
     RETURN_int(0);
}

// =======================================================
int CXfsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  DWORD dwTemp;
  memcpy(&dwTemp, cBootSect, sizeof(DWORD));
  
  //if (INT_GET(dwTemp, ARCH_CONVERT) == XFS_SUPER_MAGIC)
  if (CpuToBe32(dwTemp) == XFS_SUPER_MAGIC)
    {
      strcpy(szFileSystem, "xfs");
      RETURN_int(FS_XFS);
    }
  else
    RETURN_int(0);
}

// =======================================================
int CHfsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  WORD wTemp;
  memcpy(&wTemp, cBootSect+1024, sizeof(WORD));
  
  if (CpuToBe16(wTemp) == HFS_SUPER_MAGIC)
    {
      strcpy(szFileSystem, "hfs");
      RETURN_int(FS_HFS);
    }
  else if (memcmp(cBootSect+1024, "H+", 2) == 0)
    {
      strcpy(szFileSystem, "hfs+");
      RETURN_int(FS_HFSPLUS);
    }
  else
    RETURN_int(0);
}

// =======================================================
int CFatPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  WORD wTemp;
  DWORD dwTemp;
  
  WORD wBytesPerSector;
  BYTE cSectorsPerCluster;
  WORD wReservedSectorsCount;
  BYTE cNumberOfFATs;
  DWORD dwTotalSectorsCount;
  WORD wRootEntriesCount;
  DWORD dwClustersCount;
  DWORD dwSectorsPerFAT;
  DWORD dwDataSectors;
  DWORD dwRootDirSectors;

  if ((cBootSect[510] == 0x55) && (cBootSect[511] == 0xAA))
    {
      showDebug(4, "FAT-001\n");
      
      // 2. calculate infos
      memcpy(&wBytesPerSector, cBootSect+11, 2); wBytesPerSector = Le16ToCpu(wBytesPerSector);
      memcpy(&cSectorsPerCluster, cBootSect+13, 1); 
      memcpy(&wReservedSectorsCount, cBootSect+14, 2); wReservedSectorsCount = Le16ToCpu(wReservedSectorsCount);
      memcpy(&cNumberOfFATs, cBootSect+16, 1);
      memcpy(&wRootEntriesCount, cBootSect+17, 2); wRootEntriesCount = Le16ToCpu(wRootEntriesCount);
      
      // check data
      if ((wBytesPerSector % 512 != 0) || (wBytesPerSector < 512))
	RETURN_int(-1);
      if (cSectorsPerCluster == 0)
	RETURN_int(-1);
      if (wReservedSectorsCount == 0)
	RETURN_int(-1);
      if ((cNumberOfFATs < 1) || (cNumberOfFATs > 4))
	RETURN_int(-1);
      
      // Total number of sectors
      memcpy(&wTemp, cBootSect+19, 2); wTemp = Le16ToCpu(wTemp);
      memcpy(&dwTemp, cBootSect+32, 4); dwTemp = Le32ToCpu(dwTemp);
      if (wTemp)
	dwTotalSectorsCount = (DWORD)wTemp;
      else
	dwTotalSectorsCount = dwTemp;
      
      // Count of sectors occupied by one FAT
      memcpy(&wTemp, cBootSect+22, 2); wTemp = Le16ToCpu(wTemp);
      memcpy(&dwTemp, cBootSect+36, 4); dwTemp = Le32ToCpu(dwTemp);
      if (wTemp)
	dwSectorsPerFAT = (DWORD)wTemp;
      else
	dwSectorsPerFAT = dwTemp;		
      
      // calculate the FAT Type with the cluster count (FAT12 or FAT16 or FAT32)
      dwRootDirSectors = ((wRootEntriesCount * 32) + (wBytesPerSector-1)) / wBytesPerSector;
      
      dwDataSectors = dwTotalSectorsCount - (wReservedSectorsCount + (cNumberOfFATs * dwSectorsPerFAT) + dwRootDirSectors);
      
      if (cSectorsPerCluster == 0)
	RETURN_int(-1);
      
      dwClustersCount = dwDataSectors / cSectorsPerCluster;
      
      if (dwClustersCount < 4085)
	{	
	  strcpy(szFileSystem,"fat12");
	  RETURN_int(FS_FAT12);
	}
      else if (dwClustersCount < 65525)
	{
	  strcpy(szFileSystem,"fat16");
	  RETURN_int(FS_FAT16);
	}
      else
	{
	  strcpy(szFileSystem,"fat32");
	  RETURN_int(FS_FAT32);
	}
      
      showDebug(4, "FAT-002\n");
    }

  RETURN_int(0); // Not a FAT partition
}

// =======================================================
int CHpfsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  if (memcmp(cBootSect+54, "HPFS", 4) == 0)	
    {	
      strcpy(szFileSystem, "hpfs");
      RETURN_int(FS_HPFS);
    }
  else
    RETURN_int(0); // Not an HPFS partition
}

// =======================================================
int CNtfsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;

  if (memcmp(cBootSect+3, "NTFS", 4) == 0)	
    {
      strcpy(szFileSystem, "ntfs");
      RETURN_int(FS_NTFS);
    }
  else
    RETURN_int(0); // Not an NTFS partition
}

// =======================================================
int CUfsPartdetect(BYTE *cBootSect, char *szFileSystem, FILE *fDeviceFile)
{
  BEGIN;
  
  ufsSuperBlock sbUfs;
  cylinderGroupHeader cgp;
  DWORD i;
  int nRes;

  // init
  memcpy(&sbUfs, cBootSect+8192, sizeof(sbUfs));
  
  if (CpuToLe32(sbUfs.fs_magic) != ((DWORD)UFS_SUPER_MAGIC) && (CpuToBe32(sbUfs.fs_magic) != ((DWORD)UFS_SUPER_MAGIC)))
    RETURN_int(0); 

  for (i=0; i < (DWORD)sbUfs.fs_ncg; i++)
    {
      nRes = bigFseek(fDeviceFile, ufs_cgcmin_sb(sbUfs,i)*1024);
      if (nRes == -1)
	{
	  showDebug(2, "DETECT_UFS: bigFseek() failed\n");
	  RETURN_int(0); 
	}

      nRes = fread(&cgp, sizeof(cgp), 1, fDeviceFile);
      if (nRes != 1)
	{
	  showDebug(2, "DETECT_UFS: readData() failed\n");
	   RETURN_int(0); 
	}

      if ((DWORD)cgp.cg_magic != (DWORD)CG_MAGIC)
	{
	  showDebug(2, "DETECT_UFS: Bad magic number\n");
	   RETURN_int(0); 
	}
    }

  // found an UFS partition
  strcpy(szFileSystem, "ufs");
  RETURN_int(FS_UFS);
}

// =======================================================
int CAfsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;
  
  CAfsSuper sbAfs;

  // init
  memcpy(&sbAfs, cBootSect+AFS_SUPERBLOCK_OFFSET, sizeof(sbAfs));
  
  //showDebug(1, "magic1=%lu\n", ((DWORD)&sbAfs.as_nMagic1) - ((DWORD)&sbAfs));
  //showDebug(1, "magic2=%lu\n", ((DWORD)&sbAfs.as_nMagic2) - ((DWORD)&sbAfs));
  //showDebug(1, "magic3=%lu\n", ((DWORD)&sbAfs.as_nMagic3) - ((DWORD)&sbAfs));

  if (CpuToLe32(sbAfs.as_nMagic1) != SUPER_BLOCK_MAGIC1)
    RETURN_int(0); 

  if (CpuToLe32(sbAfs.as_nMagic2) != SUPER_BLOCK_MAGIC2)
    RETURN_int(0); 

  //if (CpuToLe32(sbAfs.as_nMagic3) != SUPER_BLOCK_MAGIC3)
  //  RETURN_int(0); 

  // found an AFS partition
  strcpy(szFileSystem, "afs");
  RETURN_int(FS_AFS);
}

// =======================================================
int CBefsPartdetect(BYTE *cBootSect, char *szFileSystem)
{
  BEGIN;
  
  CBefsSuper sbBefs;

  // init
  memcpy(&sbBefs, cBootSect+512, sizeof(sbBefs));
  
  if (sbBefs.magic1 != BEFS_SUPER_BLOCK_MAGIC1) // native endian
    RETURN_int(0); 

  if (sbBefs.magic2 != BEFS_SUPER_BLOCK_MAGIC2) // native endian
    RETURN_int(0); 

  if (sbBefs.magic3 != BEFS_SUPER_BLOCK_MAGIC3) // native endian
    RETURN_int(0); 

  // found an AFS partition
  strcpy(szFileSystem, "befs");
  RETURN_int(FS_BEFS);
}
