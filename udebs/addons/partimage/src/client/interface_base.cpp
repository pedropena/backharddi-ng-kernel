/***************************************************************************
                          interface_base.cpp  -  description
                             -------------------
    begin                : Wed Mar 14 2001
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "interface.h"
#include <string.h>
#include <stdlib.h>

// =======================================================
void CInterface::showError(signed int nErr, char *szFormat, ...)
{
  va_list args;
  char szText[4096];

  // format text
  va_start(args, szFormat);
  vsnprintf(szText, sizeof(szText), szFormat, args);
  va_end(args);

  showDebug(1, "showError: [%s]\n", szText);

  if (!m_bBatchMode)
    msgBoxCancel(i18n("Error"), "%s", szText);
}

// =======================================================
void CInterface::showAboutDialog()
{
  char szTitle[128];
  
  SNPRINTF(szTitle, i18n("About Partition Image %s"), PACKAGE_VERSION);
  
  msgBoxOk(szTitle, i18n("Partition Image is distributed under the GNU GPL 2"
     " (General Public License)\n\n"
     "version:...%s\n"
     "web:.......http://www.partimage.org/\n\n"
     "Authors:\nFrancois Dupoux  <fdupoux@partimage.org>\n"
     "Franck Ladurelle <ladurelf@partimage.org>"), PACKAGE_VERSION);
}

// =======================================================
void CInterface::ErrorWriting(DWORD block, signed int err)
{
  showError(err, i18n("Can't write block %ld to image"), block);
}
void CInterface::ErrorWritingHeader(char * header, signed int err)
{
  showError(err, i18n("Can't write %s to image"), header);
}
void CInterface::ErrorWritingBitmap(DWORD block, signed int err)
{
  showError(err, i18n("Can't write bitmap block %ld to image"), block);
}
void CInterface::ErrorReadingBitmap(DWORD block, signed int err)
{
  showError(err, i18n("Can't read bitmap block %ld from image"), block);
}
void CInterface::ErrorWritingDisk(DWORD block, signed int err)
{
  showError(err, i18n("Can't write block %ld to harddrive"), block);
}
void CInterface::ErrorReading(DWORD block, signed int err)
{
  showError(err, i18n("Can't read block %ld from image (%d)"), block);
}
void CInterface::ErrorReadingHeader(char * header, signed int err)
{
  showError(err, i18n("Can't read %s from image"), header);
}
void CInterface::ErrorReadingSuperblock(signed int err)
{
  showError(err, i18n("Can't read SuperBlock"));
}
void CInterface::ErrorWritingSuperblock(signed int err)
{
  showError(err, i18n("Can't write SuperBlock"));
}
void CInterface::ErrorReadingDisk(DWORD block, signed int err)
{
  showError(err, i18n("Can't read block %ld from harddrive partition"), block);
}
void CInterface::ErrorReadingDiskBitmap(DWORD block, signed int err)
{
  showError(err, i18n("Can't read bitmap block %ld from harddrive partition (%s)"),block);
}
// =======================================================
void CInterface::ErrorWritingMBR()
{
  showError(0, i18n("Can't write the MBR"));
}

// =======================================================
void CInterface::ErrorReadingMBR()
{
  showError(0, i18n("Can't read the MBR"));
}

// =======================================================
void CInterface::ErrorFileSystem(char * msg)
{
  showError(0, i18n("Not a %s partition. Check the filesystem type"), msg);
}

// =======================================================
void CInterface::ErrorNewerRelease()
{
  showError(0, i18n("The partition was saved with a newer version of Partion Image. "
		    "Please download the last one.")); 
}

// =======================================================
void CInterface::ErrorDiskFull()
{
  showError(0, i18n("Error while writing image file: disk full !"));
}

// =======================================================
void CInterface::ErrorDetectingFS(char * szDevice, signed int err)
{
  showError(0, i18n("Can't detect file system of partition %s"), szDevice);
}

// =======================================================
void CInterface::ErrorWritingMainHeader()
{
  showError(0, i18n("Can't write main header"));
}

// =======================================================
void CInterface::ErrorWritingMainTail()
{
  showError(0, i18n("Can't write main tail"));
}

// =======================================================
void CInterface::ErrorReadingMainTail()
{
  showError(0, i18n("Can't read main tail"));
}

// =======================================================
void CInterface::ErrorEncryption()
{
  showError(0, i18n("The image you try to restore was encrypted. The current "
		    "version doesn't support encryption. Please, download the latest version "
		    "of partimage."));
}

// =======================================================
void CInterface::ErrorAskFirstVolume(char * szVolume)
{
  showError(0, i18n("%s is not the first volume of an image. Please, retry with "
		    "the volume number 000"), szVolume);
}

// =======================================================
void CInterface::ErrorInvalidImagefile(char * szFilename)
{
  showError(0, i18n("%s is not a valid partition image file."), szFilename);
}

// =======================================================
void CInterface::ErrorTooSmall(QWORD qwOriginalSize, QWORD qwSize)
{
  showError(0, i18n("The partition is to small to be restored:\n"
		    "Original partition size:........%llu bytes\n"
		    "Destination partition size:.....%llu bytes"), qwOriginalSize, qwSize);
}

// =======================================================
void CInterface::ErrorCRC(QWORD qwOriginalCRC, QWORD qwCRC)
{
  showError(0, i18n("There was an error while restoring partition:\n"
		    "CRC64 are differents, the imagefile was damaged:"
		    "\nOriginal CRC64:.....%llu\nCurrent CRC64:......%llu"),
     qwOriginalCRC, qwCRC);  
}

// =======================================================
void CInterface::ErrorWrongVolumeNumber(DWORD dwExpectedVolume, DWORD dwVolume)
{
  showError(1, i18n("Invalid volume number (%ld instead of %ld)."),
	    dwVolume, dwExpectedVolume);
}

// =======================================================
void CInterface::ErrorClosing()
{
  showError(0, i18n("Error on close"));
}

// =======================================================
void CInterface::ErrorAlreadyLocked(char * szPartition, signed int err)
{
  showDebug(1, "STR3=%s\n", szPartition);
  showError(err, i18n("The partition %s is already locked by another process."
     " Can't work on it"), szPartition);
}

// =======================================================
/*void CInterface::Error()
{
  showError(0, i18n("There was an error"));
}*/

// =======================================================
void CInterface::ErrorOpeningPartition(char * szDevice, signed int err)
{
  showError(err, i18n("Can't open partition %s"), szDevice);
}

// =======================================================
void CInterface::ErrorLocking(char * szFilename, signed int err)
{
  showError(err, i18n("Can't lock partition %s"), szFilename);
}

// =======================================================
void CInterface::ErrorReadingMainHeader()
{
  showError(0, i18n("Can't read main header: imagefile was damaged"));
}

// =======================================================
void CInterface::ErrorWritingInfos()
{
  showError(0, i18n("Can't write informations"));
}

// =======================================================
void CInterface::ErrorReadingInfos()
{
  showError(0, i18n("Can't read informations"));
}

// =======================================================
void CInterface::ErrorReadingMBRMagic()
{
  showError(0, i18n("Can't read MBR Magic: imagefile was damaged"));
}

















// =======================================================
WORD CInterface::ErrorBugBzip2()
{
  msgBoxCancel(i18n("Know bug"), i18n("Cannot restore the MBR from an image compressed with bzip2. You have to change the compression level."));
  return 0;
}

// =======================================================
WORD CInterface::ErrorLogAsRoot()
{
  /*return = newtWinChoice(i18n("Warning"), i18n("Continue"), i18n("Cancel"), 
     i18n("You are not logged as root. You may have \"access denied\" errors"
     " when working."));*/
  return msgBoxContinueCancel(i18n("Warning"), i18n("You are not logged as root. You may have \"access denied\" errors when working."));
}

// =======================================================
WORD CInterface::WaitKeyPressed(char *szOld, char *szNewPath, 
   char *szOriginalFilename, DWORD dwVolume)
{
  /*return newtWinChoice(i18n("New Volume"), i18n("Continue"), i18n("Stop"),
     i18n("The volume has been changed.\nOld: %s\nNew: %s/%s.%.3ld\n"
     "Press Enter to continue"), szOld, szNewPath, szOriginalFilename, Volume);*/
  return msgBoxContinueCancel(i18n("New Volume"), i18n("The volume has been changed.\nOld: %s\nNew: %s/%s.%.3ld\n"
			      "Press Enter to continue"), szOld, szNewPath, szOriginalFilename, dwVolume);
}

// =======================================================
WORD CInterface::askRestore(char *szDevice, char *szImageFilename)
{
  /*return newtWinChoice(i18n("Confirmation"), i18n("Yes"), i18n("No"), 
     i18n("Do you really want to restore %s from the image file %s ?"),
     szDevice, szImageFilename);*/
  return msgBoxYesNo(i18n("Confirmation"),
		     i18n("Do you really want to restore %s from the image file %s ?"),
		     szDevice, szImageFilename);
}

// =======================================================
WORD CInterface::askIgnoreFSError(char *szFsck)
{
  return msgBoxContinueCancel(i18n("Warning"),
     i18n("%s found errors on the file system."), szFsck);
}

// =======================================================
WORD CInterface::askIgnoreNoFschk(char *szFsck)
{
  return msgBoxContinueCancel(i18n("Warning"),
     i18n("Partimage will not check partition because %s does't exist."), szFsck);
}

// =======================================================
WORD CInterface::askIgnoreDeniedFschk(char *szFsck)
{
  return msgBoxContinueCancel(i18n("Warning"),
     i18n("Partimage will not check partition because of run access denied for %s."), szFsck);
}

// =======================================================
void CInterface::WarnFS(char *szFileSys)
{
  //newtWinMessage(i18n("Warning"), i18n("Ok"), i18n("The current %s support is experimental !"), szFileSys);
  msgBoxOk(i18n("Warning"), i18n("The current %s support is experimental !"), szFileSys);
}

// =======================================================
void CInterface::WarnFsBeta(char *szFileSys)
{
  //newtWinMessage(i18n("Warning"), i18n("Ok"), i18n("The current %s support is experimental !"), szFileSys);
  msgBoxOk(i18n("Warning"), i18n("The current %s support is beta !"), szFileSys);
}

// =======================================================
void CInterface::WarnSimulate()
{
  /*newtWinMessage(i18n("Simulation mode"), i18n("Ok"), 
     i18n("You are using simulation mode: no write will be performed"));*/
  msgBoxOk(i18n("Simulation mode"), i18n("You are using simulation mode: no write will be performed"));
}

// =======================================================
WORD CInterface::WarnRestoreMBR(char * szCurrentDevice,
   QWORD qwCurrentSize, char * szOriginalDevice, QWORD qwOriginalSize)
{
  /*return newtWinChoice(i18n("Warning"), i18n("Continue"), i18n("Cancel"), 
     i18n("The MBR is a very important data of the hard disk. If you write "
     "a bad MBR, you will lose all the partitions of your hard drive. "
     "Don't continue unless you know what you are doing. Press 'Continue' "
     "to restore.\n"
     "Device with MBR to restore:.........%s [%llu]\n"
     "Original MBR:.......................%s [%llu]"),
     szCurrentDevice, qwCurrentSize, szOriginalDevice, qwOriginalSize);*/
  return msgBoxContinueCancel(i18n("Warning"), 
     i18n("The MBR is a very important data of the hard disk. If you write "
     "a bad MBR, you will lose all the partitions of your hard drive. "
     "Don't continue unless you know what you are doing. Press 'Continue' "
     "to restore.\n"
     "Device with MBR to restore:.........%s [%llu]\n"
     "Original MBR:.......................%s [%llu]"),
     szCurrentDevice, qwCurrentSize, szOriginalDevice, qwOriginalSize);
}

// =======================================================
WORD CInterface::WarnRestoreOtherMBR(char * szCurrentDevice,
   char * szOriginalDevice)
{
  /*return newtWinChoice(i18n("Warning"), i18n("Continue"), i18n("Cancel"), 
     i18n("The original hard disk does not fit with the current hard disk. "
     "The MBR you are trying to restore was saved from another hard disk. "
     "This means the original and current hard disks does not have the same "
     "size or they are not on the same device (%s and %s). "
     "Do you want to continue?"), szOriginalDevice, szCurrentDevice);*/
  return msgBoxContinueCancel(i18n("Warning"), 
     i18n("The original hard disk does not fit with the current hard disk. "
     "The MBR you are trying to restore was saved from another hard disk. "
     "This means the original and current hard disks does not have the same "
     "size or they are not on the same device (%s and %s). "
     "Do you want to continue?"), szOriginalDevice, szCurrentDevice);
}

// =======================================================
void CInterface::SuccessRestoringMBR(char * szDevice)
{
  msgBoxOk(i18n("Success"), i18n("The MBR of [%s] has been"
				 "successfully restored. You might need to reboot your computer if you want"
				 "the operating system to use the new MBR."), 
	   szDevice);
}

// ============================================================================
WORD CInterface::Error(CExceptions *excep, char *szFilename, char *szDevice/*=NULL*/)
{
  signed int err;
  DWORD dwArg1 = excep -> get_dwArg1();
  DWORD dwArg2 = excep -> get_dwArg2();
  //QWORD qwArg1 = excep -> get_qwArg1();
  //QWORD qwArg2 = excep -> get_qwArg2();
  char * szArg1 = excep -> get_szArg1();

  err = excep -> GetExcept();
  excep -> setCaught();
 
  if (err >= 0)
    {
      msgBoxError(i18n("Error: %s"), strerror(err));
      return ERR_QUIT;
    }

  switch (err)
    {
    case ERR_CANCELED:
      msgBoxError(i18n("Action canceled as user required"));
      return ERR_QUIT; 
    case ERR_ERRNO:
      msgBoxError(i18n("Error: %s"), strerror(dwArg1));
      return ERR_QUIT;
    case ERR_EXIST: 
      if (msgBoxYesNo(i18n("Overwrite image?"), i18n("The %s imagefile already exist. Do you want to overwrite it?"), 
		      szFilename) == MSGBOX_NO)
	return ERR_QUIT;
      else
	return ERR_RETRY;
      
    case ERR_ACCESS:
      msgBoxCancel(i18n("Access denied"), i18n("You don't have write access on %s"), szFilename);
      return ERR_QUIT;

    case ERR_COMP:
      msgBoxError(i18n("Invalid compression level for %s"), szFilename);
      return ERR_QUIT;

    case ERR_OPENED:
      msgBoxError(i18n("imagefile %s is already open"), szFilename);
      return ERR_QUIT;

    case ERR_EOF:
    case ERR_NOTFOUND:
    case ERR_PATH:
      // no code here
      return ERR_RETRY;

    case ERR_VOLUMEHEADER:
      msgBoxError(i18n("file %s is too short to be a partimage volume: "
		       "can't read header"), szFilename);
      return ERR_QUIT;

      /*case ERR_PARTITIONVOLUME:
        msgBoxError(i18n("file %s is not an partimage volume: "
	"magic string does not match"), szFilename);
        return ERR_QUIT;*/

    case ERR_WRONGMAGICINVALIMGFILE:// ERR_WRONGMAGIC:
      msgBoxError(i18n("file %s is not an partimage file: "
		       "magic string does not match"), szFilename);
      return ERR_QUIT;

    case ERR_WRONGMAGIC:
      msgBoxError(i18n("magic string does not match: error in the image file [%s]"), szFilename);
      return ERR_QUIT;

    case ERR_VOLUMEID:
      msgBoxError(i18n("the current volume doesn't fit with current image: "
		       "wrong identificator"));
      return ERR_QUIT;
      
    case ERR_NOMEM:
      msgBoxError(i18n("Out of memory."));
      return ERR_QUIT;

    case ERR_PASSWD:
      msgBoxError(i18n("incorrect password or user not in partimagedusers file"));
      return ERR_QUIT;

    case ERR_TOOMANY:
      msgBoxError(i18n("Too many clients connected to partimaged. "
		       "Please, retry later"));
      return ERR_QUIT;

    case ERR_BLOCKSIZE:
      msgBoxError(i18n("Only 4096 bytes per block %s volumes are supported. "
		       "Current one is %lu bytes/block"), szArg1, dwArg1);
      return ERR_QUIT;
    
    case ERR_LOCKED:
      msgBoxError(i18n("The partition %s id already locked by another"
		       "process. You can't work on it"), szArg1);
      return ERR_QUIT;

    case ERR_CHECK_CRC:
      ErrorCRC(excep->get_qwArg1(), excep->get_qwArg2());
      return ERR_QUIT;

    case ERR_CHECK_NUM:
      msgBoxError(i18n("Error while checking the data: Invalid block number."
		       "Maybe a volume or some data were skipped."));
      return ERR_QUIT;

    case ERR_VOLUMENUMBER:
      msgBoxError(i18n("This volume (%ld) is not the expected one: %ld."),
		  dwArg1, dwArg2);
      return ERR_QUIT;

    case ERR_HOST:
	msgBoxError(i18n("Hostname/IP Error!\n"
			 "Perhaps the hostname could not be resolved. "
			 "(Try again using IP address)"));
      return ERR_RETRY;

      /*
	case ERR_SSL_CONNECT:
        msgBoxError(i18n("Problem with SSL. Ensure partimaged uses SSL"));
        return ERR_QUIT;

	case ERR_SSL_CTX:
        msgBoxError(i18n("Problem with SSL. Contact partimage team for help"));
        return ERR_QUIT;

	case ERR_SSL_LOADCERT:
        msgBoxError(
	i18n("partimaged got trouble when loading SSL certificate %s"),CERTF);
        return ERR_QUIT;

	case ERR_SSL_LOADKEY:
        msgBoxError(
	i18n("partimaged got trouble when loading SSL keyfile %s"),CERTF);
        return ERR_QUIT;

	case ERR_SSL_PRIVKEY:
        msgBoxError("%s %s", i18n("SSL error: private key doesn't not match"),
	i18n("the certificate public key"));
        return ERR_QUIT;
      */

    case ERR_READ_BITMAP:
      ErrorReadingBitmap(dwArg1, dwArg2);
      return ERR_QUIT;
    case ERR_READING:
      ErrorReading(dwArg1, dwArg2);
      return ERR_QUIT;
    case ERR_WRITING: 
      ErrorWriting(dwArg1, dwArg2);
      return ERR_QUIT;
    case ERR_NOMBR:
      msgBoxError(i18n("No MBR was found in this imagefile. We can't "
		       "continue"));
      return ERR_QUIT;
    case ERR_FSCHK:
      msgBoxError(i18n("Partition checks found error(s) on partitions"));
      return ERR_QUIT;
    case ERR_OPENING_DEVICE:
      ErrorOpeningPartition(szFilename, dwArg1);
      return ERR_QUIT;
    case ERR_ENCRYPT:
      ErrorEncryption();
      return ERR_QUIT;
    case ERR_PART_TOOSMALL:
      //showDebug(1, "QW1=%llu and 2=%llu\n\n", qwArg1, qwArg2);
      ErrorTooSmall(excep->get_qwArg1(), excep->get_qwArg2());
      return ERR_QUIT;
    case ERR_FAT_MISMATCH:
      return ERR_QUIT;
    case ERR_SAVE_MBR:
      msgBoxError(i18n("Impossible to read MBR from disk")); 
      return ERR_QUIT;
    case ERR_WRONG_FS: 
      msgBoxError(i18n("Invalid or damaged filesystem %s"), szDevice); 
      return ERR_QUIT;
    case ERR_READ_SUPERBLOCK:
      ErrorReadingSuperblock(dwArg1);
      return ERR_QUIT;
    case ERR_ZEROING:
      msgBoxError(i18n("Impossible to zero the block on harddrive: %s"),
		  strerror(dwArg1));
      return ERR_QUIT;
    case ERR_REFUSED:
      msgBoxError(i18n("Connexion refused by server:"
		       " incompatibles networking options."
                       "Try disabling login option for server with '-L'"));
      return ERR_QUIT;
    case ERR_VERSIONSMISMATCH:
      msgBoxError(i18n("Connexion refused by server: versions mismatch"));
      return ERR_QUIT;

    case ERR_VERSION_NEWER:
      msgBoxError(i18n("The image file was created by a newer release (%s)" 
		       " of Partition Image. Please update this software and retry."),
		  szArg1);
      return ERR_QUIT;
    case ERR_VERSION_OLDER:
      msgBoxError(i18n("The image file was created by an older release (%s)" 
		       " of Partition Image. You must use the old version to restore"
		       "your image or you have to recreate it with current version"),
		  szArg1);
      return ERR_QUIT;

    case ERR_NOTAPARTIMAGEFILE:
      msgBoxError(i18n("%s is not a valid Partition Image file or it's"
		       " damaged. Sorry, can't continue"), szArg1);
      return ERR_QUIT;

    case ERR_NOTAREGULARFILE:
      msgBoxError(i18n("File %s is not a regular file. For security reasons,"
		       " we can't continue"), szArg1);
      return ERR_QUIT;

    case ERR_AUTOMOUNT:
      msgBoxError(i18n("Cannot mount %s"), szArg1 /*m_szMountDevice*/);
      return ERR_QUIT;

    case ERR_AUTOUMOUNT:
      msgBoxError(i18n("Cannot unmount %s"), szArg1 /*m_szMountDevice*/);
      return ERR_QUIT;

    case ERR_CREATESPACEFILE:
      msgBoxError(i18n("Cannot create temp file [%s]. Please, check there is space enough and you have access rights."), szArg1);
      return ERR_QUIT; 

    case ERR_CREATESPACEFILENOSPC:
      msgBoxError(i18n("Cannot create temp file [%s]. No space left on device."), szArg1);
      return ERR_QUIT; 

    case ERR_CREATESPACEFILEDENIED:
#ifdef MUST_CHEUID
      msgBoxError(i18n("Cannot create temp file in [%s]. Write access is denied"
         " for user 'partimag'"), szArg1);
#else
      msgBoxError(i18n("Cannot create temp file in [%s]. Write access is denied."), szArg1);
#endif
      return ERR_QUIT; 

    case ERR_FILETOOLARGE:
      msgBoxError(i18n("File too large. (larger than 2 GB). Can't continue. This problem can come from the kernel, the glibc, the file system which where the image is written"));
      return ERR_QUIT;       

    default:
      msgBoxError(i18n("Unknown error %d -> %s"), err, szFilename);
      exit(-5);
    }
}

