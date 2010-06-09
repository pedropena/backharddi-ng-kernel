/***************************************************************************
                          misc.h  -  description
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

#ifndef MISC_H
#define MISC_H

#include "partimage.h"
#include "common.h"

struct CImage;
struct CMainHeader;
struct CMainTail;
struct COptions;

// =======================================================
#ifndef major
  #define major(dev) (((dev) >> 8) & 0xff)
#endif

#ifndef minor
  #define minor(dev) ((dev) & 0xff)
#endif

#ifndef makedev
  #define makedev(maj,min) (((maj) << 8) | min))
#endif

// =======================================================
int isDevfsMounted();
int isDevfsEnabled();

// =======================================================
void savePartition(char *szDevice, char *szImageName, /*char *szFilesystem, */COptions *options);
void restorePartition(char *szDevice, char *szImageName, COptions *options);
void restoreMbr(char *szImageFile, COptions *options);

// =======================================================
void closeFilesSave(bool on_error, COptions options, CImage *image, FILE *fDeviceFile);
void closeFilesRestore(bool on_error, COptions options, CImage *image, FILE *fDeviceFile);
bool isFileSystemSupported(char *szFileSystem);
void lockFile(int nFileDesc, char *szDevice);
int getMajorMinor(char *szDevice, int *nMajor, int *nMinor);

// ================================================
/*
struct CVolumeHeader // size must be 512 (adjust the reserved data)
{
  char szMagicString[32];
  char szVersion[64]; // version of the image file
  DWORD dwVolumeNumber;
  QWORD qwIdentificator; // to avoid using volumes from differents image files on restore
  
  unsigned char cReserved[404]; // Adjust to fit with total header size
};
*/

// ================================================
/*
struct CMainTail // size must be 16384 (adjust the reserved data)
{
  QWORD qwCRC;
  DWORD dwVolumeNumber;
  
  unsigned char cReserved[16372]; // Adjust to fit with total header size
};
*/

//#define MAX_UNAMEINFOLEN 65 //SYS_NMLN

// ================================================
/*
struct CMainHeader // size must be 16384 (adjust the reserved data)
{
  char szFileSystem[512]; // ext2fs, ntfs, reiserfs, ...
  char szPartDescription[MAX_DESCRIPTION]; // user description of the partition
  char szOriginalDevice[MAX_DEVICENAMELEN]; // original partition name
  char szFirstImageFilepath[MAXPATHLEN]; // for splitted image files
  
  // system and hardware infos
  char szUnameSysname[MAX_UNAMEINFOLEN];
  char szUnameNodename[MAX_UNAMEINFOLEN];
  char szUnameRelease[MAX_UNAMEINFOLEN];
  char szUnameVersion[MAX_UNAMEINFOLEN];
  char szUnameMachine[MAX_UNAMEINFOLEN];

  DWORD dwCompression; // COMPRESS_XXXXXX
  DWORD dwMainFlags;
  struct tm dateCreate; // date of image creation
  QWORD qwPartSize; // size of the partition in bytes
  char szHostname[MAX_HOSTNAMESIZE];
  char szVersion[64]; // version of the image file

  // MBR backup
  DWORD dwMbrCount; // how many MBR are saved in the image file
  DWORD dwMbrSize; // size of a MBR record (allow to change the size in the next versions)

  // future encryption support
  DWORD dwEncryptAlgo; // algo used to encrypt data
  char cHashTestKey[16]; // used to test the password without giving it
  
  BYTE cReserved[6564]; // Adjust to fit with total header size
};
*/

/** prints the list of all options selected by user (compress level, splitting, ...)
 *  @param options Pointer to the COptions structure to show
 */ 
void showDebugOptions(COptions *options);

/** This functions tells if the file is existing
 *  @param szPath Path of the file
 *  @return @p true if the file exists
 *       or @p false if it does not exists
 */ 

// CRC 32
void initCrcTable(DWORD *dwCrcTable);

// =======================================================
QWORD swapNb(QWORD nb); // 64 bits
DWORD swapNb(DWORD nb); // 32 bits
WORD swapNb(WORD nb); // 16 bits


// =======================================================
#define DO1(buf) dwCrc = g_dwCrcTable[((int)dwCrc ^ (*buf++)) & 0xff] ^ (dwCrc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

// =======================================================
inline DWORD increaseCrc32(DWORD dwCrc, const char *buf, DWORD dwLen)
{
  dwCrc = dwCrc ^ 0xffffffffL;

  while (dwLen >= 8)
    {
      DO8(buf);
      dwLen -= 8;
    }
  
  if (dwLen) do 
    {
      DO1(buf);
    } while (--dwLen);
  
  return dwCrc ^ 0xffffffffL;
}

/** Detects the FAT type of a partition, and write the type description in the
    buffer pointed by swFileSys ("fat16", "fat32", "fat12")
 *  @param cBoot buffer with the boot sector of the partition
 *  @return @p -1 if the partition is not FAT
 *       or @p FS_FAT16 if partition is FAT16
 *       or @p FS_FAT32 if partition is FAT32
 */ 
//int detectFatType(BYTE *cBoot, char *szFileSys);

/** Shows the list of logical partitions contained in an UFS slice
 *  @param szDevice Device of the partition to look at (ex: /dev/hda3)
 *  @return @p 0 if success
 *       or @p -1 if error
 */ 
int showUfsDisklabel(char *szDevice);

/** This functions tells if the device is mounted
 *  @param szDevice The device partition to look for the mount (ex: "/dev/hda1")
 *  @param szMountPoint Buffer where to write the mount point of the device if it
 *   was mounted. This is a result.
 *  @return @p true if the partition is mounted
 *       or @p false if it is not mounted 
 */ 
int isMounted(const char *szDevice, char *szMountPoint);

/** Gives the size of a partition in bytes
 *  @param szDevice The device partition to look for the mount (ex: "/dev/hda1")
 *  @return the number of bytes the physical partition contains in a QWORD number
 */ 
QWORD getPartitionSize(const char *szDevice);

/** shows a  dialog box to ask a description of an image file to the user
 *  @param szDescription buffer where to write the description (result)
 *  @param dwMaxLen maximum length of the description (size of the buffer)
*/
void askDescription(char *szDescription, DWORD dwMaxLen);

/** try to detect the file system of a partition
 *  @param szDevice The device partition to look for the file system (ex: "/dev/hda1")
 *  @param szFileSys buffer where to write the detected file system (result)
 *  @return identificator of the file system
*/
int detectFileSystem(const char *szDevice, char *szFileSys);

/** check the inode file in the /dev/ directory is created, and then
    then the device will be opened. In deed, sometimes, big minor number
    partitions are not created (/dev/hda16) for example. If the inode does
    not exists, a message tells the user how to create it.
 *  @param szDevice The device partition to chek for inode (ex: "/dev/hda1")
 *  @return @p 0 if success, @p -1 if error
*/
int checkInodeForDevice(char *szDevice);

/** It allows to make a fseek (SEEK_CUR mode only) in a file with offsets greaters than 2 GB.
    For example, you can seek 8 GB with this function.
 *  @param fStream stream returned by fopen() of the file to seek
 *  @param qwOffset 64 bist number of the seek
 *  @return @p -1 if error of @p 0 if success
*/
int bigFseek(FILE *fStream, QWORD qwOffset); // SEEK_SET forced

/** It makes important checks before an operation:
 *  + check the inodes in /dev/ are created
 *  + check the user is logged as the root
 *  + check the partition is not mounted
 *  @param swDevice Device (ex: /dev/hda3) to check for the inode and the mount state
 *  @param bFailIfMounted: true if the function must return an error if the partition is mounted
 *  @return @p -1 if error or @p 0 if success
*/
int systemChecks(char *szDevice, bool bFailIfMounted);

/** This function receives data of the /proc/partitions files, pases it and
 *  returns the informations it contains (major, minor, blocks, partnum, device).
 *  It allows to show the partition list.
 *  @param cPtr pointer to the beginning of the line of the buffer to parse
 *  @param nMajor pointer to the number where to write the major number
 *  @param nMinor pointer to the number where to write the minor number
 *  @param nBlocks pointer to the number where to write the blocks count
 *  @param nPartNum pointer to the number where to write the number of the partition
 *  @param szDevice pointer to the buffer where to write the partition device name
 *  @return @p pointer to the end of the line which was parsed
*/
char *decodePartitionEntry(char *cPtr, int *nMajor, int *nMinor, int *nBlocks, int *nPartNum, char *szDevice);

/** Check the options are valid. For example, the size of a volume must be grater than 1024 KB, the port
 *  used for the network must not be null, ...
 *  @param options copy of a COptions structure to check
 *  @return @p -1 if error or @p 0 if success
*/
int checkOptions(COptions options, char *szDevice, char *szImageFile);

int CXfsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CReiserPartdetect(BYTE *cBootSect, char *szFileSystem);
int CExt2Partdetect(BYTE *cBootSect, char *szFileSystem);
int CAfsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CJfsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CFatPartdetect(BYTE *cBootSect, char *szFileSystem);
int CHpfsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CNtfsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CHfsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CBefsPartdetect(BYTE *cBootSect, char *szFileSystem);
int CUfsPartdetect(BYTE *cBootSect, char *szFileSystem, FILE *fDeviceFile);

bool isVersionStable(const char *szVersion);

#endif //MISC_H

