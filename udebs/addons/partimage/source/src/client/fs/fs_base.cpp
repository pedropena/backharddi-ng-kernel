/***************************************************************************
       fs_base.cpp  -  Base class of all File System with common code
                             -------------------
    begin                : Thu Jul 13 2000
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

#include "fs_base.h"
#include "partimage.h"
#include "imagefile.h"
#include "common.h"
#include "cbitmap.h"
#include "exceptions.h"

// =======================================================
CFSBase::CFSBase(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize)
{
  BEGIN;

  m_fDeviceFile = fDeviceFile;
  strncpy(m_szDevice, szDevice, MAX_DEVICENAMELEN);
  memset(&m_header, 0, sizeof(CLocalHeader));
  m_qwPartSize = qwPartSize;
  showDebug(1, "qwPartSize=%llu=%llu * 512\n", qwPartSize, qwPartSize/512LL);
  m_bCheckSuperBlockUsedSize = false;
  m_bCheckSuperBlockFreeSize = false;
  m_bCanBeMore = false;

  RETURN;
}

// =======================================================
CFSBase::~CFSBase()
{
}

// =======================================================
int CFSBase::readData(void *cBuffer, QWORD qwOffset, DWORD dwLength)
{
   BEGIN;

   int nRes;

   errno = 0;
   nRes = bigFseek(m_fDeviceFile, qwOffset);
   if (nRes != 0)
   {
      showDebug(4, "CFSBase::readData(qwOffset=%llu): bigFseek() error %d=%s\n", qwOffset, errno, strerror(errno));
      RETURN_int(-1);
   }

   errno = 0;
   nRes = fread((char *)cBuffer, dwLength, 1, m_fDeviceFile);
   if (nRes != 1)
   {
      showDebug(4, "CFSBase::readData(dwLength=%lu): fread() error %d=%s\n", dwLength, errno, strerror(errno));
      RETURN_int(-1);
   }
  
   RETURN_int(0); // success
}


// =======================================================
void checkForEscape()
{
  int nRes;

  if (g_bSigInt == true)
    {
      nRes = g_interface->msgBoxYesNo(i18n("Exit partimage ?"), i18n("Do you really want to cancel the current operation ?"));
      if (nRes == MSGBOX_YES)
	{
	  errno = 0;
	  THROW(ERR_CANCELED, errno);
	}
      else // NO
	{
	  g_bSigInt = false;
	}
    }
}

// =======================================================
// may throw: errno, readbitmap, reading, writing
void CFSBase::saveUsedBlocksToImageFile(COptions *options)
{
  BEGIN;

  char cBuffer[COPYBUF_SIZE];
  bool bUsed;
  DWORD dwCount;
  int nRes;
  CCheck check;
  QWORD i;
  char *cBegin;
  DWORD dwStats;

  DWORD dwSavedBlocks = 0L;
  DWORD dwBlocksInBuffer;
  DWORD dwCheckCount = 0L;
  DWORD dwDataSize;
  DWORD dwTemp;

  // init ESC key
  g_bSigInt = false;

  // go to the beginning of the disk
  nRes = fseek(m_fDeviceFile, 0L, SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);
  
  // init the bitmap optimizations
  m_bitmap.setMaxFreeBlocksCopiedPerOper(MAX_FREE_BLOCKS_SKIPPED); // could be another value
  dwBlocksInBuffer = (COPYBUF_SIZE-(COPYBUF_SIZE % m_header.qwBlockSize)) / (m_header.qwBlockSize); 
  m_bitmap.setMaxUsedBlocksCopiedPerOper(dwBlocksInBuffer);
  showDebug(3, "CPY: setMax=%lu\n", dwBlocksInBuffer);
  check.dwCRC = increaseCrc32(0, NULL, 0);

  // set magic
  check.cMagic[0] = 'C';
  check.cMagic[1] = 'H';
  check.cMagic[2] = 'K';

  dwStats = 0;
  i = 0;
  while (i < m_header.qwBlocksCount) // while there are blocks to copy
    {
      nRes = m_bitmap.readBitmap(i, &bUsed, &dwCount);
      if (nRes == -1)
        THROW(ERR_READ_BITMAP, i);

      // stop at the end of the partition
      dwCount = my_min(dwCount, m_header.qwBlocksCount-i);

      showDebug(3, "CPY: dwCount=%lu, bUsed=%d\n", dwCount, bUsed);

      if (bUsed) // if used blocks are marked in the bitmap
	{
	  dwDataSize = m_header.qwBlockSize*dwCount;
	  
	  errno = 0;
	  nRes = fread(cBuffer, dwDataSize, 1, m_fDeviceFile);
	  if (nRes != 1)
	    {
              THROW(ERR_READING, (DWORD)i, errno);
	    }
	  
	  // ---- begin -----

	  cBegin = cBuffer;
	  while (dwDataSize > 0)
	    {
	      // calculate length of data to write
	      if (dwCheckCount + dwDataSize < CHECK_FREQUENCY) // no check to write
		dwTemp = dwDataSize;
	      else // there is a check to write
		dwTemp = CHECK_FREQUENCY - dwCheckCount;
	      
	      // increase CRC
	      check.dwCRC = increaseCrc32(check.dwCRC, cBegin, dwTemp);
	      //for (j=0; j < dwTemp; j++)
		//check.qwCRC += *(cBegin+j);

	      try { m_image->write(cBegin, dwTemp, true); }
	      catch (CExceptions * excep)
		{
                  showDebug(1, "saveUsedBlocksToImageFile: excep %d\n", 
                     excep->GetExcept());
                  throw excep;
		}

	      dwCheckCount += dwTemp;

	      if (dwCheckCount == CHECK_FREQUENCY)
		{
		  showDebug(3, "CHECKING(%lu): check=%lu and CRC=%lu\n", i, dwCheckCount, check.dwCRC);
	      
		  check.qwPos = i; 
                  showDebug(3, "BEFORE WRITING CHK\n");
		  try { m_image->write((char*)&check, sizeof(CCheck), true); }
		  catch (CExceptions * excep)
		    {
                      showDebug(1, "saveUsedBlocksToImageFile: excep %d\n", 
                         excep->GetExcept());
                      throw excep;
		    }

		  check.dwCRC = increaseCrc32(0, NULL, 0);
		  dwCheckCount = 0;
		  
		  // show stats
		  if (dwStats == 8)
		    {
		      m_guiSave->showStats(g_timeStart, m_header.qwBlockSize, dwSavedBlocks, m_header.qwUsedBlocks, options->szFullyBatchMode);
		      dwStats = 0;
		    }
		  dwStats++;
		}

	      cBegin += dwTemp;
	      dwDataSize -= dwTemp;
	      
	      checkForEscape();
	    }
	  
	  // ---- end -----

	  dwSavedBlocks += dwCount;
	}
      else // if free blocks were marked
	{
	  nRes = fseek(m_fDeviceFile, m_header.qwBlockSize*dwCount, SEEK_CUR);
	  if (nRes == -1)
            {
              THROW(ERR_ERRNO, errno);
            }
	}

      i += dwCount;

      // draw progress
      //m_guiSave->showStats(g_timeStart, m_header.qwBlockSize, dwSavedBlocks, m_header.qwUsedBlocks);
    }

  m_guiSave->showStats(g_timeStart, m_header.qwBlockSize, dwSavedBlocks, m_header.qwUsedBlocks, options->szFullyBatchMode);
  g_qwCopiedBytesCount = m_header.qwBlockSize * dwSavedBlocks; // for final message box

  RETURN;
}

// =======================================================
// may raise errno, check_num, check_crc,zer, writing, wrong_magic
// NEW optimized version
void CFSBase::restoreUsedBlocksToDisk(COptions *options)
{
  BEGIN;

  char cZeroBuffer[ZERO_BLOCK_SIZE];
  char cBuffer[COPYBUF_SIZE];
  bool bUsed;
  DWORD dwCount;
  int nRes;
  QWORD i;
  DWORD dwBlocksInBuffer;
  DWORD dwTemp;
  DWORD dwCheckCount = 0L;
  DWORD dwDataSize;
  DWORD dwSavedBlocks = 0L;
  CCheck check;
  char *cBegin;
  DWORD dwCRC;
  DWORD dwStats;

  memset(cZeroBuffer, 0, ZERO_BLOCK_SIZE);
  dwCRC = increaseCrc32(0, NULL, 0);  

  // init ESC key
  g_bSigInt = false;

  // go to the beginning of the disk
  nRes = fseek(m_fDeviceFile, 0L, SEEK_SET);
  if (nRes == -1)
    THROW(ERR_ERRNO, errno);

  // init the bitmap optimizations
  m_bitmap.setMaxFreeBlocksCopiedPerOper(MAX_FREE_BLOCKS_SKIPPED); // could be another value
  dwBlocksInBuffer = (COPYBUF_SIZE-(COPYBUF_SIZE % m_header.qwBlockSize)) / 
     (m_header.qwBlockSize); 
  m_bitmap.setMaxUsedBlocksCopiedPerOper(dwBlocksInBuffer);
  showDebug(3, "CPY: setMax=%lu\n", dwBlocksInBuffer);
  
  dwStats = 0;
  i = 0;
  while (i < m_header.qwBlocksCount)
    {
      showDebug(2, "i = %lld, m_header.qwBlocksCount = %lld\n", i,
        m_header.qwBlocksCount);

      m_bitmap.readBitmap(i, &bUsed, &dwCount);
      dwCount = my_min(dwCount, m_header.qwBlocksCount-i); // stop at the end of the partition

      showDebug(3, "CPY: dwCount=%lu, bUsed=%d\n", dwCount, bUsed);

      if (bUsed) // if used blocks are marked in the bitmap
	{
	  dwDataSize = m_header.qwBlockSize*dwCount;
	  dwSavedBlocks += dwCount;

	  // ---- begin -----

	  cBegin = cBuffer;
	  while (dwDataSize > 0)
	    {
              showDebug(8, "dwDataSize=%ld\n", dwDataSize);
	      // calculate length of data to write
	      if (dwCheckCount + dwDataSize < CHECK_FREQUENCY) // no check to write
		dwTemp = dwDataSize;
	      else // there is a check to write
		dwTemp = CHECK_FREQUENCY - dwCheckCount;
	      
	      try { m_image->read(cBegin, dwTemp, true); }
	      catch (CExceptions * excep)
		{
                  showDebug(1, "restoreUsedBlocksToDisk: except %d\n",
                     excep -> GetExcept());
                  throw excep;
		}	

	      // increase CRC
              dwCRC = increaseCrc32(dwCRC, cBegin, dwTemp);
	      showDebug(3, "CRC = %lu\n", dwCRC);

	      dwCheckCount += dwTemp;
	      showDebug(3, "dwCheckCount= %ld\n", dwCheckCount);

	      if (dwCheckCount == CHECK_FREQUENCY)
		{
                  showDebug(3, "BEFORE READING CHK\n");
		  try { m_image->read((char*)&check, sizeof(CCheck), true); }
		  catch (CExceptions * excep)
		    {
                      showDebug(1, "restoreUsedBlocksToDisk: except %d\n",
                         excep -> GetExcept());
                      throw excep;
		    }

		  showDebug(3, "CHECKING(%lu): check=%lu and CRC=%lu\n", i, 
                     dwCheckCount, dwCRC);

		  // check magic
		  if ((check.cMagic[0] != 'C') || (check.cMagic[1] != 'H')
                     || (check.cMagic[2] != 'K'))
		    {
		      showDebug(1, "CHECK-MAGIC error %3s %3s\n", "CHK", check.cMagic);
		      nRes=g_interface->msgBoxYesNo(i18n("local check error"),
                         i18n("Impossible to find a local check structure. "
                         "Data may be damaged. Do you want to continue ?"));
		      if (nRes == MSGBOX_NO)
		        THROW(ERR_WRONGMAGIC);
		    }
                  else
                    {
		      if (i != check.qwPos)
    		        {
		          showDebug(1, "CHECK-POS: old=%llu and new=%llu\n",
                             check.qwPos, i);
		     
		          nRes=g_interface->msgBoxYesNo(i18n("local CRC error"),
                             i18n("There was a position error in CCheck. "
                             "Data may be damaged. Do you want to continue ?"));
		          if (nRes == MSGBOX_NO)
			    THROW(ERR_CHECK_NUM, i, check.qwPos);
		        }

		      if (dwCRC != check.dwCRC)
		        {
		          showDebug(1, "CHECK-CRC(%lu): old=%lu and new=%lu\n", 
                             i, check.dwCRC, dwCRC);

		          nRes=g_interface->msgBoxYesNo(i18n("local CRC error"),
                             i18n("There was a CRC error in CCheck. "
                             "Data may be damaged. Do you want to continue ?"));
		          if (nRes == MSGBOX_NO)
			    THROW(ERR_CHECK_CRC, dwCRC, check.dwCRC);
		        }
                    }

                  dwCRC = increaseCrc32(0, NULL, 0);
		  dwCheckCount = 0;
		  
		  // show stats
		  if (dwStats == 8)
		    {
		      // update stats
		      m_guiRestore->showStats(g_timeStart,
                         m_header.qwBlockSize, dwSavedBlocks,
                         m_header.qwUsedBlocks,m_header.qwBlocksCount,
			 options->bEraseWithNull, options->szFullyBatchMode);
		      dwStats = 0;
		    }
		  dwStats++;

		}

	      errno = 0;
	      if (!options->bSimulateMode)
		{
		  nRes = fwrite(cBegin, dwTemp, 1, m_fDeviceFile);
		  if (nRes != 1)
		    {
                      THROW(ERR_WRITING, (DWORD) 0, errno);
		    }				
		}
	      else
		showDebug(10, "fwrite simulated\n");

	      cBegin += dwTemp;
	      dwDataSize -= dwTemp;

	      checkForEscape();
	    }
          showDebug(9, "end while dwDataSize\n");
	  
	  // ---- end -----
	  
	}
      else // if free blocks were marked in the bitmap
	{
	  if (options->bEraseWithNull == true) // if free and we must write 0 bytes on unused clusters
	    {
	      dwTemp = m_header.qwBlockSize * dwCount; // data to copy
	      
	      while (dwTemp > 0)
		{
		  errno = 0;
                  if (!options->bSimulateMode)
		    {
		      nRes = fwrite(cZeroBuffer, my_min(ZERO_BLOCK_SIZE, dwTemp),
                         1, m_fDeviceFile);
		      if (nRes != 1)
                        THROW(ERR_ZEROING, errno);
		    }
                  else
                    showDebug(10, "fwrite simulated\n");
		  
		  dwTemp -= my_min(ZERO_BLOCK_SIZE, dwTemp);
		}
	      dwSavedBlocks += dwCount;
	    }
	  else // don't write zero bytes
	    {
	      nRes = fseek(m_fDeviceFile, m_header.qwBlockSize*dwCount,
                 SEEK_CUR);
	      if (nRes == -1)
                THROW(ERR_ERRNO, errno);
	    }			
	}
      i += dwCount;
    }
  
  // update stats
  m_guiRestore->showStats(g_timeStart, m_header.qwBlockSize, dwSavedBlocks,
     m_header.qwUsedBlocks, m_header.qwBlocksCount, options->bEraseWithNull, options->szFullyBatchMode);
  g_qwCopiedBytesCount = m_header.qwBlockSize * dwSavedBlocks; // for final message box
  
  RETURN;
}

// =======================================================
void CFSBase::saveToImage(CImage *image, COptions *options, CSavingWindow *gui)
{
  m_guiSave = gui;

  BEGIN;
  
  m_image = image;
  
  // ---- 0. Initializations
  
  // read informations
  showDebug(3, "TRACE: 001 (init)\n");  
  g_interface -> StatusLine(i18n("reading partition properties"));
  try { readSuperBlock(); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "saveToImage: excep %d\n", excep->GetExcept());
      THROW(ERR_READ_SUPERBLOCK);
    }	
  
  // check the file system
  showDebug(3, "TRACE: 002 (fsck)\n");  
  if (options->bCheckBeforeSaving == true)
    {
      g_interface -> StatusLine(i18n("checking the file system with fsck"));

      try { fsck(); }
      catch ( CExceptions * excep )
        { throw excep; }
    }

  // read bitmap into memory (must be done before writing header)
  showDebug(3, "TRACE: 003 (readBitmap)\n");  
  showDebug(1, "begin of readBitmap()\n");
  readBitmap(options);
  showDebug(1, "end of readBitmap()\n");

  // check the bitmap has been read without error
  checkBitmapInfos();

  // print informations	
  if (options->bBatchMode == false)
    printfInformations();
  
  // write header
  showDebug(3, "TRACE: 004 (writeHeader)\n");  
  g_interface -> StatusLine(i18n("writing header"));
  try { m_image->writeMagic(MAGIC_BEGIN_LOCALHEADER); }
  catch ( CExceptions * excep )
    { throw excep; }
  errno = 0;
  try { m_image->write(&m_header, sizeof(CLocalHeader), true); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "saveToImage: excep %d\n", excep->GetExcept());
      throw excep;
    }

  // write CRC of CLocaleader
  m_image->writeCRC((char*)&m_header, sizeof(CLocalHeader));
  
  // copy bitmap to image file
  showDebug(3, "TRACE: 005 (writeBitmap)\n");  
  try { m_image->writeMagic(MAGIC_BEGIN_BITMAP); }
  catch ( CExceptions * excep )
    { throw excep; }

  try { m_image->write(m_bitmap.data(), m_header.qwBitmapSize, true); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "saveToImage: excep %d\n", excep->GetExcept());
      throw excep;
    }

  // write informations specific for the FS
  showDebug(3, "TRACE: 006 (writeInfo)\n");  
  try { m_image->writeMagic(MAGIC_BEGIN_INFO); }
  catch ( CExceptions * excep )
    { throw excep; }

  try { m_image->write((char*)getInfos(), INFOS_STRUCT_SIZE, true); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "saveToImage: excep %d\n", excep->GetExcept());
      throw excep;
    }
  
  // write CRC of info
  m_image->writeCRC((char*)getInfos(), INFOS_STRUCT_SIZE);

  // write magic string
  try { m_image->writeMagic(MAGIC_BEGIN_DATABLOCKS); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "saveToImage: excep %d\n", excep->GetExcept());
      throw excep;
    }
  
  // time of beginning
  g_timeStart = time(0);
    
  // ---- write all used blocks
  showDebug(3, "TRACE: 007 (copyUsedBlocks)\n");  
  g_interface -> StatusLine(i18n("copying used data blocks"));

  showDebug(3, "begin of saveUsedBlocksToImageFile()\n");  
  try { saveUsedBlocksToImageFile(options); }
  catch ( CExceptions * excep)
    {
      showDebug(1, "saveToImage: excep %d\n", excep->GetExcept());
      throw excep;
    }
  
  RETURN;
}

// =======================================================
void CFSBase::restoreFromImage(CImage *image, COptions *options, CRestoringWindow *gui)
{
  int nRes;

  BEGIN;
  
  // init
  m_image = image;
  m_guiRestore = gui;
  
  g_interface -> StatusLine(i18n("reading partition informations"));
  
  // 1. ---- Read header
  showDebug(3, "TRACE: 001 (readHeader)\n");  
  try { m_image->readAndCheckMagic(MAGIC_BEGIN_LOCALHEADER); } 
  catch ( CExceptions * excep )
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }

  try {m_image->read((char*)&m_header, sizeof(CLocalHeader), true);}
  catch(CExceptions * excep)
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }

  // read CRC of info
  m_image->readAndCheckCRC((char*)&m_header, sizeof(CLocalHeader));

  
  // check the current format is supported
  /*if (strncmp(headReiser.szAppFormat, CURRENT_REISER_FORMAT, 
    strlen(CURRENT_REISER_FORMAT)) < 0)
    {
    g_interface->ErrorNewerRelease();
    RETURN_int(-1);
    }*/	
    
  // 2. ---- Print informations and ask confirmation to restore
  /*showDebug(1, "TRACE: 002 (printfInfos)\n");  
    if (options->bBatchMode == false)
    {
    printfInformations();
      
    // ask confirmation
    nRes = g_interface -> askRestore(m_szDevice,
    image->get_szImageFilename());
    if (nRes == 2) // 2 --> cancel
    RETURN_int(-1);
    }*/
  
  // time of beginning
  g_timeStart = time(0);
  
  // ---- 5. read bitmap
  showDebug(3, "TRACE: 003 (readBitmap)\n");  
  try { m_image->readAndCheckMagic(MAGIC_BEGIN_BITMAP); } 
  catch ( CExceptions * excep )
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }
  
  // init bitmap size
  m_bitmap.init(m_header.qwBitmapSize);
  
  // copy bitmap blocks to bitmap in memory
  try{m_image->read((char*)m_bitmap.data(), (long)(m_header.qwBitmapSize), true);}
  catch (CExceptions * excep)
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }

  // write informations specific for the FS
  showDebug(3, "TRACE: 004 (readInfo)\n");  
  try {m_image->readAndCheckMagic(MAGIC_BEGIN_INFO); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }

  try { m_image->read((char*)getInfos(), INFOS_STRUCT_SIZE,true); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }

  // read CRC of info
  m_image->readAndCheckCRC((char*)getInfos(), INFOS_STRUCT_SIZE);

  // 2. ---- Print informations and ask confirmation to restore
  showDebug(3, "TRACE: 002 (printfInfos)\n");  
  if (options->bBatchMode == false)
    {
      printfInformations();
      
      // ask confirmation
      if (!options->bSimulateMode)
	{
	  nRes = g_interface -> askRestore(m_szDevice, image->get_szImageFilename());
	  if (nRes == MSGBOX_NO) // 2 --> cancel
	    THROW(ERR_CANCELED);
	}
    }

  // read and check magic string
  showDebug(3, "TRACE: 005 (copyUsedBlocks)\n");  
  try { m_image->readAndCheckMagic(MAGIC_BEGIN_DATABLOCKS); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }
  
  // ---- 7. Restore all used blocks
  errno = 0;
  g_interface -> StatusLine(i18n("copying used data blocks"));
  showDebug(1, "begin of restoreUsedBlocksToDisk()\n");  
  try { restoreUsedBlocksToDisk(options); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "restoreFromImage: %d\n", excep->GetExcept()); 
      throw excep;
    }
  showDebug(9, "end of restoreUsedBlocksToDisk()\n");  
  
  RETURN;
}

// =======================================================
int CFSBase::getStdInfos(char *szDest, int nMaxLen, bool bShowBlocksInfo)
{
  BEGIN;

  char szTemp1[512];
  char szTemp2[512];
  char szTemp3[512];
  char szBlocksInfo[8192];

  if (bShowBlocksInfo)
    {
      SNPRINTF(szBlocksInfo, i18n("Block size....................%llu bytes\n"
				  "Total blocks count............%llu\n"
				  "Used blocks count.............%llu\n"
				  "Free blocks count.............%llu\n"),
	       m_header.qwBlockSize, 
	       m_header.qwBlocksCount,
	       m_header.qwUsedBlocks,
	       m_header.qwBlocksCount-m_header.qwUsedBlocks);     
    }
  else
    {
      *szBlocksInfo = 0; // empty
    }
 
  snprintf(szDest, nMaxLen, i18n("%s" // Blocks infos
				 "Space usage:..................%llu %%\n"
				 "Used space....................%s\n"
				 "Free space....................%s\n"
				 "Bitmap size...................%s\n"
				 "Label.........................%s\n"),
	   szBlocksInfo,
	   (100 * m_header.qwUsedBlocks)/m_header.qwBlocksCount, 
	   formatSize(m_header.qwUsedBlocks * m_header.qwBlockSize, szTemp1),
	   // used space
	   formatSize((m_header.qwBlocksCount-m_header.qwUsedBlocks) *
		      m_header.qwBlockSize, szTemp2), // free space
	   formatSize(m_header.qwBitmapSize, szTemp3),
	   m_header.szLabel);

  showDebug(1, "\n====================\n%s====================\n\n", szDest);
 
  RETURN_int(0);
}			

// =======================================================
int CFSBase::calculateSpaceFromBitmap()
{ 
  BEGIN;

  QWORD qwUsedBlocksCount = 0;
  QWORD qwFreeBlocksCount = 0;
  QWORD i;

  for (i=0; i < m_header.qwBlocksCount; i++)
    {
      if (m_bitmap.isBitSet(i))
	qwUsedBlocksCount++;
      else
	qwFreeBlocksCount++;
    }

  m_header.qwUsedBlocks = qwUsedBlocksCount;

  showDebug(1, "BITMAP: qwUsedBlocks = %llu\n", qwUsedBlocksCount);
  showDebug(1, "BITMAP: qwFreeBlocks = %llu\n", qwFreeBlocksCount);

  RETURN_int(0);
}

// =======================================================
/*int CFSBase::checkBitmapInfos()
{
  QWORD qwBitmapUsedSize;
  QWORD qwBitmapFreeSize;
  qwBitmapUsedSize = m_header.qwUsedBlocks * m_header.qwBlockSize;
  qwBitmapFreeSize = (m_header.qwBlocksCount - m_header.qwUsedBlocks) * m_header.qwBlockSize;

  showDebug(1, "CHECK BITMAP: qwSuperBlockUsedSize.....%llu = %llu sectors\n", m_qwSuperBlockUsedSize, m_qwSuperBlockUsedSize/512LL);
  showDebug(1, "CHECK BITMAP: qwBitmapUsedSize.........%llu = %llu sectors\n", qwBitmapUsedSize, qwBitmapUsedSize/512LL);

  if ((m_bCheckSuperBlockUsedSize) && (qwBitmapUsedSize != m_qwSuperBlockUsedSize))
    {
      showDebug(1, "ERROR BITMAP Used\n");
      THROW(ERR_READ_BITMAP, (DWORD)0);
    }

  showDebug(1, "CHECK BITMAP: qwSuperBlockFreeSize.....%llu = %llu sectors\n", m_qwSuperBlockFreeSize, m_qwSuperBlockFreeSize/512LL);
  showDebug(1, "CHECK BITMAP: qwBitmapFreeSize.........%llu = %llu sectors\n", qwBitmapFreeSize, qwBitmapFreeSize/512LL);

  if ((m_bCheckSuperBlockFreeSize) && (qwBitmapFreeSize != m_qwSuperBlockFreeSize))
    {
      showDebug(1, "ERROR BITMAP Free\n");
      THROW(ERR_READ_BITMAP, (DWORD)0);
    }

  RETURN_int(0);
}*/

// =======================================================
int CFSBase::checkBitmapInfos()
{
  QWORD qwBitmapUsedSize;
  QWORD qwBitmapFreeSize;
  qwBitmapUsedSize = m_header.qwUsedBlocks * m_header.qwBlockSize;
  qwBitmapFreeSize = (m_header.qwBlocksCount - m_header.qwUsedBlocks) * m_header.qwBlockSize;

  showDebug(1, "CHECK BITMAP: qwSuperBlockUsedSize.....%llu = %llu sectors\n", m_qwSuperBlockUsedSize, m_qwSuperBlockUsedSize/512LL);
  showDebug(1, "CHECK BITMAP: qwBitmapUsedSize.........%llu = %llu sectors\n", qwBitmapUsedSize, qwBitmapUsedSize/512LL);
  showDebug(1, "CHECK BITMAP: qwSuperBlockFreeSize.....%llu = %llu sectors\n", m_qwSuperBlockFreeSize, m_qwSuperBlockFreeSize/512LL);
  showDebug(1, "CHECK BITMAP: qwBitmapFreeSize.........%llu = %llu sectors\n", qwBitmapFreeSize, qwBitmapFreeSize/512LL);

  if (m_bCheckSuperBlockUsedSize)
    {
      if ((!m_bCanBeMore && (qwBitmapUsedSize != m_qwSuperBlockUsedSize)) || (m_bCanBeMore && (qwBitmapUsedSize < m_qwSuperBlockUsedSize)))
	{
	  showDebug(1, "ERROR BITMAP Used and m_bCanBeMore=%d\n", m_bCanBeMore);
	  THROW(ERR_READ_BITMAP, (DWORD)0);
	}
    }
  
  if (m_bCheckSuperBlockFreeSize)
    {
      if ((!m_bCanBeMore && (qwBitmapFreeSize != m_qwSuperBlockFreeSize)) || (m_bCanBeMore && (qwBitmapFreeSize > m_qwSuperBlockFreeSize)))
	{
	  showDebug(1, "ERROR BITMAP Free and m_bCanBeMore=%d\n", m_bCanBeMore);
	  THROW(ERR_READ_BITMAP, (DWORD)0);
	}
    }
  
  RETURN_int(0);
}


// =======================================================
void CFSBase::setSuperBlockInfos(BOOL bCheckSuperBlockUsedSize, BOOL bCheckSuperBlockFreeSize, 
				 QWORD qwSuperBlockUsedSize, QWORD qwSuperBlockFreeSize, BOOL bCanBeMore/*=false*/) 
{
  m_bCanBeMore = bCanBeMore;
  m_qwSuperBlockUsedSize = qwSuperBlockUsedSize; m_qwSuperBlockFreeSize = qwSuperBlockFreeSize;
  m_bCheckSuperBlockUsedSize = bCheckSuperBlockUsedSize;
  m_bCheckSuperBlockFreeSize = bCheckSuperBlockFreeSize;
}
