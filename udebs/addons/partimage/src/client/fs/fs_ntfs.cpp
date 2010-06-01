/***************************************************************************
              cntfspart.cpp  - NTFS (Windows NT) File System Support
                             -------------------
    begin                : Mon May 22 2000
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

#include "partimage.h"
#include "imagefile.h"
#include "fs_ntfs.h"

// =======================================================
CNtfsPart::CNtfsPart(char *szDevice, FILE *fDeviceFile, QWORD qwPartSize): CFSBase(szDevice, fDeviceFile, qwPartSize)
{
  BEGIN;
  RETURN;
}

// =======================================================
CNtfsPart::~CNtfsPart()
{
  BEGIN;
  RETURN;
}

// =======================================================
void CNtfsPart::fsck()
{
// SUCCESS
}

// =======================================================
void CNtfsPart::readSuperBlock()
{
  BEGIN;

  int nRes;
  const int nMaxBootSectSize = 16384;
  BYTE cBoot[nMaxBootSectSize+1];
  
  // init
  memset(&m_info, 0, sizeof(CInfoNtfsHeader));

  nRes = fread(cBoot, nMaxBootSectSize, 1, m_fDeviceFile);
  if (nRes != 1)
    THROW(ERR_ERRNO, errno);
  
  // 1. check partition has an ntfs file system
  if (memcmp(cBoot+3, "NTFS", 4) != 0)
    {
      g_interface->msgBoxError(i18n("%s is not an NTFS file system partition"), m_szDevice);
      THROW(ERR_ERRNO, errno);
    }
  
  showDebug(1, "\n======== begin NTFS BOOT INFOS ========\n");
  
  char cClustersPerMftRecord;
  
  m_info.nBytesPerSector = NTFS_GETU16(cBoot+0xB);
  m_info.cSectorsPerCluster = NTFS_GETU8(cBoot+0xD);
  m_info.qwTotalSectorsCount = NTFS_GETU64(cBoot+0x28);
  m_info.qwLCNOfMftDataAttrib = NTFS_GETU64(cBoot+0x30);
  cClustersPerMftRecord = NTFS_GETS8(cBoot+0x40);
  
  // check informations validity
  if (m_info.nBytesPerSector % 512 != 0)
    THROW(ERR_ERRNO, errno);
  
  // Calculate clusters and misc informations
  m_info.dwClusterSize = (DWORD) (m_info.nBytesPerSector * m_info.cSectorsPerCluster);
  //m_dwFileRecordSize *= m_dwClusterSize;
  //m_qwTotalClustersCount = (QWORD) (m_qwTotalSectorsCount / m_cSectorsPerCluster);
  
  if (cClustersPerMftRecord > 0)
    m_info.dwFileRecordSize = cClustersPerMftRecord * m_info.dwClusterSize;
  else
    m_info.dwFileRecordSize = 1 << (-cClustersPerMftRecord);
  
  showDebug(1, "m_nBytesPerSector = %d\n", m_info.nBytesPerSector);
  showDebug(1, "m_cSectorsPerCluster = %d\n", m_info.cSectorsPerCluster);
  showDebug(1, "m_qwTotalSectorsCount = %llu\n", m_info.qwTotalSectorsCount);
  showDebug(1, "m_qwLCNOfMftDataAttrib = %llu\n", m_info.qwLCNOfMftDataAttrib);
  showDebug(1, "cClustersPerMftRecord = %d\n", cClustersPerMftRecord);
  showDebug(1, "Calculated m_dwFileRecordSize = %lu\n", m_info.dwFileRecordSize);
  
  showDebug(1, "======== end NTFS BOOT INFOS ========\n\n");
  
  // read label
  showDebug(1, "begin of readVolumeLabel()\n");  
  nRes = readVolumeLabel();
  showDebug(1, "end of readVolumeLabel(), return = %d\n", nRes);
  
  // set abstract informations
  m_header.qwBlockSize = (DWORD) m_info.dwClusterSize; // 1 cluster
  m_header.qwBlocksCount = (QWORD) (m_info.qwTotalSectorsCount / m_info.cSectorsPerCluster);

  setSuperBlockInfos(false, false, 0 , 0);

  RETURN;
}

// =======================================================
void CNtfsPart::readBitmap(COptions *options)
{
  BEGIN;

  int nRes;
  BYTE *cFileRecord; // 1024 in most cases
  CNtfsRunList runlistBitmap, runlistMft;
  QWORD qwBitmapSize;
  QWORD qwAllocatedSize;
  QWORD qwOffset;
  QWORD i;
  
  // used in experimental search
  char cBitmapNameUnicode[] = {0x24, 0, 0x42, 0, 0x69, 0, 0x74, 0, 0x6d, 0, 0x61, 0, 0x70, 0}; // len = 14
  
  // allocate memory
  cFileRecord = new BYTE[m_info.dwFileRecordSize+1];
  if (!cFileRecord)
    {
      showDebug(1, "Out of memory: need %lu bytes to read file record in readBitmap()\n", m_info.dwFileRecordSize);
      
      g_interface->msgBoxError(i18n("Out of memory: need %lu bytes to read file record in readBitmap()"), m_info.dwFileRecordSize);
      THROW(ERR_ERRNO, errno);
    }	
  
  // 0. MFT Record: find $Bitmap file record
  /*qwOffset = ((QWORD) m_qwLCNOfMftDataAttrib * m_dwClusterSize);
    nRes = fseek(m_fDeviceFile, (long)qwOffset, SEEK_SET);
    if (nRes == -1)
    throw neew CExceptions("ntfs::readBitmap", ERR_ERRNO, errno);
    
    nRes = fread(cFileRecord, m_dwFileRecordSize, 1, m_fDeviceFile);
    if (nRes != 1)
    throw neew CExceptions("ntfs::readBitmap", ERR_ERRNO, errno);
    
    nRes = readFileRecord(cFileRecord, m_dwFileRecordSize, &runlistMft, &qwBitmapSize, &qwAllocatedSize);*/
  //runlistBitmap.show();
  
  // read bitmap file record of the MFT (record number 6
  // TODO: bug if the begin of the data attrib of the MFT begins after 2 GB
  // TODO: bug if the MFT is fragmented, the $Bitmap record (6 th) may be in another location of the disk
  //       then we must read the MFT runlist, to know where the MFT is written. But we need to read the
  //       "attribute list" to do it... ==> very difficult
  
  // 1. read $Bitmap record
  qwOffset = ((QWORD) m_info.qwLCNOfMftDataAttrib * m_info.dwClusterSize) + ((QWORD) (6 * m_info.dwFileRecordSize));
  showDebug(1, "qwOffset = %llu\n", qwOffset);
  /*nRes = fseek(m_fDeviceFile, (long)qwOffset, SEEK_SET);
    if (nRes == -1)
    {
    showDebug(1, "failed: cannot run fseek(to bitmap record)\n");
    delete []cFileRecord;
    THROW(ERR_ERRNO, errno);
    }
  
    nRes = fread(cFileRecord, m_info.dwFileRecordSize, 1, m_fDeviceFile);*/
  nRes = readData(cFileRecord, qwOffset, m_info.dwFileRecordSize);
  if (nRes == -1)
    {
      showDebug(1, "failed: cannot run readData(bitmap record)\n");
      delete []cFileRecord;
      THROW(ERR_ERRNO, errno);
    }
  
  // check we read the $bitmap file record
  showDebug(1, "begin of checkFilenameForRecordIs()\n");
  nRes = checkFilenameForRecordIs(cFileRecord, "$bitmap");
  showDebug(1, "end of checkFilenameForRecordIs(), return = %d\n", nRes);
  
  if (nRes == -1) // The file record we read was not the $bitmap one...
    {
      nRes = g_interface->msgBoxYesNo(i18n("Error"), i18n("Unable to read the $Bitmap MFT record. "
							  "Do you want to make an experimental search for it ? (it can take a long time)"));
      if (nRes == MSGBOX_NO)
	{
	  showDebug(1, "readBitmap(): user doesn't want to search for $bitmap MFT record\n");
	  delete []cFileRecord;
          THROW(ERR_ERRNO, errno);
	}
      
      showDebug(1, "begin of searchForRecordFilename()\n");
      nRes = searchForRecordFilename(cFileRecord, "$bitmap", cBitmapNameUnicode, 14);
      showDebug(1, "end of searchForRecordFilename(), result = %d\n", nRes);
      if (nRes == -1) // if found
	{
	  showDebug(1, "readBitmap(): failed: Cannot find the $bitmap with experimental search\n");
	  g_interface->msgBoxError(i18n("Cannot find the $bitmap with experimental search"));
	  delete []cFileRecord;
          THROW(ERR_ERRNO, errno);
	}
    }
  
  // -------- decode the MFT File record
  showDebug(1, "begin of readFileRecord()\n");  
  nRes = readFileRecord(cFileRecord, &runlistBitmap, &qwBitmapSize, &qwAllocatedSize);
  showDebug(1, "end of readFileRecord(), return = %d\n", nRes);
  if (nRes == -1)
    {
      g_interface->msgBoxError(i18n("Cannot use $bitmap file record"));
      showDebug(1, "readBitmap(): failed: Cannot use $bitmap file record\n");	
    }
  
  //runlistBitmap.show();
  
  // If the bitmap returned cannot map all clusters of the partition (because bitmap too small)
  if ((runlistBitmap.clustersCount() * m_info.dwClusterSize * 8) < m_header.qwBlocksCount)
    {
      g_interface->msgBoxError(i18n("NTFS $Bitmap is to small"));
      showDebug(1, "readBitmap(): NTFS $Bitmap is to small\n");
      delete []cFileRecord;
      THROW(ERR_ERRNO, errno);
    }
  
  // -------- read bitmap file from runlist
  m_header.qwBitmapSize = (DWORD) qwAllocatedSize;
  //fprintf(stderr,"m_dwBitmapSize=%lu\n", m_dwBitmapSize);
  //fprintf(stderr,"m_dwallocated=%llu\n", qwAllocatedSize);
  
  // allocate memory for bitmap
  nRes = m_bitmap.init(m_header.qwBitmapSize);
  if (nRes == -1)
    {
      showDebug(1, "readBitmap(): failed: Out of memory in bitmap.init()\n");	
      g_interface->msgBoxError(i18n("Out of memory"));
      delete []cFileRecord;
      THROW(ERR_ERRNO, errno);
    }
  
  // debug log file
  showDebug(1, "readBitmap(): =========== begin of decoded run list ===============\n");
  for (i=0L; i < runlistBitmap.clustersCount(); i++)
    showDebug(1, "i=%llu, cluster=%llu, offset=%llu\n", i, (runlistBitmap.offset(i)), (QWORD)(runlistBitmap.offset(i) * (QWORD)m_info.dwClusterSize));
  showDebug(1, "readBitmap(): =========== end of decoded run list ===============\n");
  
  // read bitmap
  for (i=0L; i < runlistBitmap.clustersCount(); i++)
    {
      errno = 0;
      /*showDebug(1, "begin of big_fseek()\n");	
	nRes = bigFseek(m_fDeviceFile, (QWORD)(runlistBitmap.offset(i) * ((QWORD)m_info.dwClusterSize)));
	showDebug(1, "end of big_fseek() (%s) (error %d)\n", strerror(errno), errno);
	if (nRes == -1)
	{
	showDebug(1, "readBitmap(): failed: error in fseek() (%s) (error %d)\n", strerror(errno), errno);	
	delete []cFileRecord;
	THROW(ERR_ERRNO, errno);
	}
      
	errno = 0;
	nRes = fread(m_bitmap.data()+(i*m_info.dwClusterSize), m_info.dwClusterSize, 1, m_fDeviceFile);
	if (nRes != 1)
	{
	showDebug(1, "readBitmap(): failed: error in fread() (%s) (error %d)\n", strerror(errno), errno);	
	delete []cFileRecord;
	THROW(ERR_ERRNO, errno);
	}*/

      nRes = readData(m_bitmap.data()+(i*m_info.dwClusterSize), (QWORD)(runlistBitmap.offset(i) * ((QWORD)m_info.dwClusterSize)), m_info.dwClusterSize);
      if (nRes == -1)
	{
	  showDebug(1, "ERROR: error in readData() (%s) (error %d)\n", strerror(errno), errno);	
	  delete []cFileRecord;
          THROW(ERR_ERRNO, errno);
	}
    }
  
  // fill informations about free/used clusters count
  calculateSpaceFromBitmap();

  /*QWORD qwUsedClustersCount = 0L;
  QWORD qwFreeClustersCount = 0L;
  
  for (i=0; i < m_header.qwBlocksCount; i++)
    {
      if (m_bitmap.isBitSet(i))
	qwUsedClustersCount++;
      else
	qwFreeClustersCount++;
    }

  m_header.qwUsedBlocks = qwUsedClustersCount;*/
  
  delete []cFileRecord;

  showDebug(9, "end\n");  
}

// =======================================================
void CNtfsPart::printfInformations()
{
  char szFullText[8192];
  char szText[8192];
  
  getStdInfos(szText, sizeof(szText), true);
  
  SNPRINTF(szFullText, i18n("%s" // standard infos
			    "Bytes per sector..............%u\n"
			    "Sectors per cluster...........%u\n"
			    "File record size..............%lu\n"
			    "LCN of MFT Data attrib........%llu\n"),
	   szText, m_info.nBytesPerSector, m_info.cSectorsPerCluster, 
	   m_info.dwFileRecordSize, m_info.qwLCNOfMftDataAttrib);
    
  g_interface->msgBoxOk(i18n("NTFS informations"), szFullText);
}

// =======================================================
int CNtfsPart::readVolumeLabel()
{
  int nRes;
  BYTE *cFileRecord; // 1024 in most cases
  QWORD qwOffset;
  WORD nOffsetSequenceAttribute;
  BYTE *cData;
  DWORD dwAttribType;
  DWORD dwAttribLen;
  bool bAttribResident;	
  BYTE *cDataResident;
  WORD nAttrSize;
  QWORD i;
  
  // init
  memset(m_header.szLabel, 0, 127);
  
  // allocate memory
  cFileRecord = new BYTE[m_info.dwFileRecordSize+1];
  if (!cFileRecord)
    {
      g_interface->msgBoxError(i18n("Out of memory: need %lu bytes to read file record in readVolumeLabel()"), m_info.dwFileRecordSize);
      RETURN_int(-1);
    }	
  
  // TODO: read the MFT to have the $volume record location
  
  // 1. read $Volume record
  qwOffset = ((QWORD) m_info.qwLCNOfMftDataAttrib * m_info.dwClusterSize) + ((QWORD) (3 * m_info.dwFileRecordSize));
  /*nRes = fseek(m_fDeviceFile, (long)qwOffset, SEEK_SET);
    if (nRes == -1)
    {	
    showDebug(1, "readVolumeLabel(): fseek() failed\n");
    delete []cFileRecord;
    RETURN_int(-1);
    }
  
    nRes = fread(cFileRecord, m_info.dwFileRecordSize, 1, m_fDeviceFile);
    if (nRes != 1)
    {
    showDebug(1, "readVolumeLabel(): fread() failed\n");
    delete []cFileRecord;
    RETURN_int(-1);
    }*/

  nRes = readData(cFileRecord, qwOffset, m_info.dwFileRecordSize);
  if (nRes == -1)
    {
      showDebug(1, "readVolumeLabel(): failed in readData()\n");
      delete []cFileRecord;
      RETURN_int(-1);
    }

  // -------- decode the MFT File record
  nOffsetSequenceAttribute = NTFS_GETU16(cFileRecord+0x14);
  cData = cFileRecord + nOffsetSequenceAttribute;
  
  do
    {
      // szData points to the beginning of an attribute
      dwAttribType = NTFS_GETU32(cData);
      dwAttribLen = NTFS_GETU32(cData+4);
      bAttribResident = (NTFS_GETU8(cData+8)==0);
      
      if(dwAttribType == 0x60) // "volume_name"
	{
	  if (bAttribResident == false)
	    {
	      showDebug(1, "readVolumeLabel(): failed: $volume_name attribute is not resident\n");
	      delete []cFileRecord;
	      RETURN_int(-1);
	    }
	  
	  nAttrSize = NTFS_GETU16(cData+0x10);
	  cDataResident = cData+NTFS_GETU16(cData+0x14);
	  
	  for (i=0; i < (DWORD)(nAttrSize/2); i++)
	    m_header.szLabel[i] = cDataResident[2*i];
	}
      if(dwAttribType == 0x70) // "volume_information"
	{	
	  m_info.nNtfsVersion = 0;
	  if (bAttribResident == true)
	    {	
	      nAttrSize = NTFS_GETU16(cData+0x10);
	      cDataResident = cData+NTFS_GETU16(cData+0x14);
	      m_info.nNtfsVersion = (((BYTE) cDataResident[8]) << 8) | ((BYTE) cDataResident[9]);
	      showDebug(1, "readVolumeLabel(): version is %d.%d\n", cDataResident[8], cDataResident[9]);
	    }
	}
      
      cData += dwAttribLen;
    } while(dwAttribType != (DWORD)-1); // attribute list ends with type -1
  
  delete []cFileRecord;
  return 0;
}
			
// =======================================================
int CNtfsPart::readFileRecord(BYTE *cRecordData, CNtfsRunList *runlist, QWORD *qwBitmapSize, QWORD *qwAllocatedSize)
{
  BYTE *cData;
  WORD nOffsetSequenceAttribute;
  DWORD dwAttribType;
  DWORD dwAttribLen;
  bool bAttribResident;
  int nRes;
  
  nOffsetSequenceAttribute = NTFS_GETU16(cRecordData+0x14);
  cData = cRecordData + nOffsetSequenceAttribute;
  
  showDebug(1, "\n\n=============== begin FILE RECORD ================\n");
  fwrite(cRecordData, m_info.dwFileRecordSize, 1, g_fDebug);
  showDebug(1, "\n=============== end FILE RECORD ================\n\n\n");
  
  do
    {
      // szData points to the beginning of an attribute
      dwAttribType = NTFS_GETU32(cData);
      dwAttribLen = NTFS_GETU32(cData+4);
      bAttribResident = (NTFS_GETU8(cData+8)==0);
      
      if(dwAttribType == 0x20) // "attribute list"
	{
	  g_interface->msgBoxError(i18n("NTFS: \"Attribute List\" attribute is not yet supported."));
	  showDebug(1, "readFileRecord(): NTFS: \"Attribute List\" attribute is not yet supported.\n");
	  RETURN_int(-1);
	}
      else if(dwAttribType == 0x80) // "data"
	{
	  showDebug(1, "begin of readDataAttribute()\n");
	  nRes = readDataAttribute(cData, runlist, qwBitmapSize, qwAllocatedSize);
	  showDebug(1, "end of readDataAttribute(), return = %d\n", nRes);
	  if (nRes == -1)
	    RETURN_int(-1);
	  
	}
      cData += dwAttribLen;
    } while(dwAttribType != (DWORD)-1); // attribute list ends with type -1
  
  return 0;
}

// =======================================================
int CNtfsPart::readDataAttribute(BYTE *cAttribData, CNtfsRunList *runlist, QWORD *qwBitmapSize, QWORD *qwAllocatedSize)
{
  bool bAttribResident;
  WORD nCompressed;
  int nRes;
  
  // Resident flag
  bAttribResident = NTFS_GETU8(cAttribData+8)==0;
  
  // Misc
  nCompressed = NTFS_GETU16(cAttribData+0xC);
  //fprintf(stderr,"Comp=%ld=%X with type=data\n", nCompressed, nCompressed);
  if (nCompressed)
    {
      g_interface->msgBoxError(i18n("NTFS: Compression flag for DATA attribute not null: %ld"), nCompressed);
      showDebug(1, "readDataAttribute(): NTFS: Compression flag for DATA attribute not null: %u\n", nCompressed);
      RETURN_int(-1);
    }
  
  if(bAttribResident)
    {
      // TODO: decode resident data
      WORD nAttrSize;
      BYTE *cDataResident;
      nAttrSize = NTFS_GETU16(cAttribData+0x10);
      cDataResident = cAttribData+NTFS_GETU16(cAttribData+0x14);
      /*WORD nOffsetOfStream;
	nOffsetOfStream=NTFS_GETU16(cAttribData+0x14);
	qwDataSize = (QWORD) NTFS_GETU16(cAttribData+0x10);*/
      g_interface->msgBoxError(i18n("NTFS: Resident data attribute not supported"));
      showDebug(1, "readDataAttribute(): NTFS: Resident data attribute not supported\n");
      RETURN_int(-1);
    }
  else
    {
      //*qwAllocatedSize = NTFS_GETU32(cAttribData+0x28);
      //*qwBitmapSize = NTFS_GETU32(cAttribData+0x30);
      
      showDebug(1, "begin of processRunList()\n");  
      nRes = processRunList(cAttribData, runlist, qwAllocatedSize, qwBitmapSize);
      showDebug(1, "end of processRunList(), return = %d\n", nRes);
      if (nRes == -1)
	g_interface->msgBoxError(i18n("NTFS: processRunList() returned -1"));
      return nRes;
    }
  
  RETURN_int(-1);
}

// =======================================================
int CNtfsPart::readFilenameAttribute(BYTE *cAttribData, char *szFilename)
{
  bool bAttribResident;
  char szTemp[4096];
  WORD nAttrSize;
  BYTE *cDataResident;
  int i;
  
  // Resident flag
  bAttribResident = NTFS_GETU8(cAttribData+8)==0;
  if (bAttribResident == false)
    {
      g_interface->msgBoxCancel(i18n("Error in readFilenameAttribute"), i18n("The \"filename\" attribute must be resident !"));	
      showDebug(1, "readFilenameAttribute(): NTFS: The \"filename\" attribute must be resident !\n");
      RETURN_int(-1);
    }
  
  nAttrSize = NTFS_GETU16(cAttribData+0x10)-0x42;
  cDataResident = cAttribData+NTFS_GETU16(cAttribData+0x14)+0x42;
  
  // decode the unicode filename
  memset(szTemp, 0, 4095);
  for (i=0; i < nAttrSize/2; i++)
    szTemp[i] = cDataResident[2*i];
  
  /*fprintf(stderr,"==============> Encoded file name: (");
    printBuf(stderr, (char*)cDataResident, 2*nAttrSize);
    fprintf(stderr,")\n==============> Decoded file name: (%s)\n", szTemp);*/
  
  // szFilename must be equal to szTemp now
  if (strcasecmp(szTemp, szFilename) != 0)
    {
      showDebug(1, "readFilenameAttribute(): Cannot check filename is \"%s\" (%s)\n", szFilename, szTemp);	
      RETURN_int(-1);
    }
  else
    {
      showDebug(1, "readFilenameAttribute(): success (%s)\n", szFilename);		
    }
  
  return 0; // success
}

// =======================================================
int CNtfsPart::processRunList(BYTE *cAttribData, CNtfsRunList *runlist, QWORD *qwAllocatedSize, QWORD *qwRealSize)
{			
  //QWORD qwAllocatedSize;
  //QWORD qwRealSize;
  QWORD qwCompressedSize;
  WORD nCompressionEngine;
  WORD nOffsetRunList;
  BYTE *cRunList;
  QWORD qwStartVcn;
  QWORD qwEndVcn;
  QWORD qwLastClusterOffset;
  QWORD qwClusterNumber;
  
  showDebug(1, "\n\n\n================== begin processRunList =============\n");
  
  qwStartVcn = NTFS_GETU64(cAttribData+0x10);	
  qwEndVcn = NTFS_GETU64(cAttribData+0x18);
  *qwAllocatedSize = NTFS_GETU64(cAttribData+0x28);	
  *qwRealSize = NTFS_GETU64(cAttribData+0x30);
  qwCompressedSize = NTFS_GETU64(cAttribData+0x38);	
  nCompressionEngine = NTFS_GETU16(cAttribData+0x22);
  
  showDebug(1, "qwStartVcn = %llu\n", qwStartVcn);
  showDebug(1, "qwEndVcn = %llu\n", qwEndVcn);
  showDebug(1, "qwAllocatedSize = %llu\n", *qwAllocatedSize);
  showDebug(1, "qwRealSize = %llu\n", *qwRealSize);
  showDebug(1, "qwCompressedSize = %llu\n", qwCompressedSize);
  showDebug(1, "nCompressionEngine = %d\n", nCompressionEngine);
  
  //if(attr->compressed)
  //	attr->compsize=NTFS_GETU32(cAttribData+0x40);
  
  nOffsetRunList = NTFS_GETU16(cAttribData+0x20);
  cRunList = cAttribData + nOffsetRunList;
  
  BYTE cType;
  BYTE cSizeOfOffset;
  BYTE cSizeOfLength;
  BYTE *cData;
  QWORD qwLength;
  long long signed int llOffset;
  QWORD i;
  int nRes;
  
  nRes = runlist->init(qwEndVcn-qwStartVcn+1); // number of clusters used by data
  if (nRes == -1)
    {
      g_interface->msgBoxError(i18n("Out of memory"));
      showDebug(1, "processRunList(): Out of memory.\n");
      RETURN_int(-1);
    }
  
  qwClusterNumber = qwStartVcn;	
  qwLastClusterOffset = 0L;
  while (qwClusterNumber <= qwEndVcn)
    {
      cType = NTFS_GETU8(cRunList);
      cSizeOfLength = cType & 0xF;
      cSizeOfOffset = cType & 0xF0;
      cData = cRunList + 1;
      
      // 1: length
      switch(cSizeOfLength)
	{
	case 1: qwLength = NTFS_GETU8(cData); break;
	case 2: qwLength = NTFS_GETU16(cData); break;
	case 3: qwLength = NTFS_GETU24(cData); break;
	case 4: qwLength = NTFS_GETU32(cData); break;
	  // Note: cases 5-8 are probably pointless to code,
	  // since how many runs > 4GB of length are there?
	  // at the most, cases 5 and 6 are probably necessary,
	  // and would also require making length 64-bit
	  // throughout
	default:
				//ntfs_error("Can't decode run type field %x\n",type);
	  RETURN_int(-1);
	}
      cData += cSizeOfLength;
      
      // 2: offset
      switch(cSizeOfOffset)
	{
	  //case 0:	   *ctype=2; break;
	case 0x10: llOffset = NTFS_GETS8(cData);break;
	case 0x20: llOffset = NTFS_GETS16(cData);break;
	case 0x30: llOffset = NTFS_GETS24(cData);break;
	case 0x40: llOffset = NTFS_GETS32(cData);break;
#if 0 // Keep for future, in case ntfs_cluster_t ever becomes 64bit
	case 0x50: llOffset = NTFS_GETS40(cData);break;
	case 0x60: llOffset = NTFS_GETS48(cData);break;
	case 0x70: llOffset = NTFS_GETS56(cData);break;
	case 0x80: llOffset = NTFS_GETS64(cData);break;	
#endif
	default:
	  //ntfs_error("Can't decode run type field %x\n",type);
	  RETURN_int(-1);
	}
      cData += cSizeOfOffset;

      showDebug(10, "DEBUGGING: pos=%llu and len=%llu and bool=%d\n", qwClusterNumber, qwLength, (int)(qwClusterNumber < qwEndVcn));
      
      for (i = 0; i < qwLength; i++)
	{
	  runlist->setCluster(qwClusterNumber, qwLastClusterOffset + llOffset + i);
	  qwClusterNumber++;
	}
      
      qwLastClusterOffset += llOffset;
    }
  
  showDebug(1, "\n================== end processRunList =============\n\n\n");
  
  return 0;
}

// ================================================
CNtfsRunList::CNtfsRunList()
{
  m_qwOffset = NULL; // no memory allocated
  m_qwClustersCount = 0L;
}

// ================================================
CNtfsRunList::~CNtfsRunList()
{
  freeMemory();
}

// ================================================
void CNtfsRunList::freeMemory()
{
  if (m_qwOffset)
    {
      delete []m_qwOffset;
      m_qwClustersCount = 0L;
    }
}

// ================================================
int CNtfsRunList::init(QWORD qwClustersCount)
{
  if (m_qwOffset)
    RETURN_int(-1); // mem alreay allocated
  
  m_qwOffset = new QWORD[qwClustersCount+1];
  if (!m_qwOffset)
    RETURN_int(-1);
  
  m_qwClustersCount = qwClustersCount;
  
  return 0; // success
}

// ================================================
int CNtfsRunList::setCluster(QWORD qwNumber, QWORD qwOffset) // ex: cluser 0 position is 12589
{
  if ((qwOffset) && (qwNumber <= m_qwClustersCount))
    {
      m_qwOffset[qwNumber] = qwOffset;
      return 0;
    }
  else
    RETURN_int(-1);
}

// ================================================
void CNtfsRunList::show()
{
  QWORD i;
  
  for (i=0L; i < m_qwClustersCount; i++)
    fprintf(stderr, "cluster[%llu] = %llu\n", i, m_qwOffset[i]);
  
}

// ================================================
QWORD CNtfsRunList::offset(QWORD qwCluster)
{
  if (m_qwOffset)
    return m_qwOffset[qwCluster];
  else
    return 0;
}

// ================================================
QWORD CNtfsRunList::clustersCount()
{
  return m_qwClustersCount;
}

// ================================================
int CNtfsPart::checkFilenameForRecordIs(BYTE *cRecordData, char *szFilename, bool bShowDebug)
{
  BYTE *cData;
  WORD nOffsetSequenceAttribute;
  DWORD dwAttribType;
  DWORD dwAttribLen;
  bool bAttribResident;
  int nRes;
  
  nOffsetSequenceAttribute = NTFS_GETU16(cRecordData+0x14);
  cData = cRecordData + nOffsetSequenceAttribute;
  
  // check this is a file record
  if (memcmp(cRecordData, "FILE", 4) != 0)
    {
      if (bShowDebug)
	showDebug(1, "checkFilenameForRecordIs(): error im magic string, not a valid file record\n");
      RETURN_int(-1);
    }	
  
  do
    {
      // szData points to the beginning of an attribute
      dwAttribType = NTFS_GETU32(cData);
      dwAttribLen = NTFS_GETU32(cData+4);
      bAttribResident = (NTFS_GETU8(cData+8)==0);
      
      if(dwAttribType == 0x30) // "filename"
	{
	  showDebug(1, "begin of readFilenameAttribute()\n");
	  nRes = readFilenameAttribute(cData, szFilename);
	  showDebug(1, "end of readFilenameAttribute(), return = %d\n", nRes);
	  return nRes;
	}
      cData += dwAttribLen;
      
    } while(dwAttribType != (DWORD)-1); // attribute list ends with type -1
  
  RETURN_int(-1);
}

// ================================================
int CNtfsPart::searchForRecordFilename(BYTE *cFileRecord, char *szFilename, char *szUnicodeName, int nUnicodeNameLen)
{
  QWORD qwCurrentOffset = 0L;
  QWORD qwPartSize;
  char cBuffer[MAX_NTFS_BLOCK_SIZE+1];
  int nRes;
  int i;
  
  qwPartSize = getPartitionSize(m_szDevice);
  
  // go to the beginning of the partition
  nRes = fseek(m_fDeviceFile, 0L, SEEK_SET);
  if (nRes == -1)
    RETURN_int(-1);
  
  while(qwCurrentOffset + m_info.dwClusterSize < qwPartSize)
    {
      nRes = fread(cBuffer, m_info.dwClusterSize, 1, m_fDeviceFile);
      if (nRes != 1)
	{
	  showDebug(1, "searchForRecordFilename(): error in fread(1)\n");
	  RETURN_int(-1);
	}	
      
      qwCurrentOffset += m_info.dwClusterSize;
      
      // if it can be the begin of the file record
      if (memcmp(cBuffer, "FILE", 4) == 0)
	{
	  showDebug(1, "searchForRecordFilename: begin FILE detected\n");	
	  
	  nRes = fseek(m_fDeviceFile, -m_info.dwClusterSize, SEEK_CUR);
	  if (nRes == -1)
	    {
	      showDebug(1, "searchForRecordFilename(): error in fseek()\n");
	      RETURN_int(-1);
	    }	
	  
	  showDebug(1, "searchForRecordFilename: 002 FILE detected\n");	
	  
	  qwCurrentOffset -= m_info.dwClusterSize;
	  
	  nRes = fread(cFileRecord, m_info.dwFileRecordSize, 1, m_fDeviceFile);
	  if (nRes != 1)
	    {
	      showDebug(1, "searchForRecordFilename(): error in fread(2)\n");
	      RETURN_int(-1);
	    }	
	  
	  showDebug(1, "searchForRecordFilename: 003 FILE detected\n");	
	  
	  qwCurrentOffset += m_info.dwFileRecordSize;
	  
	  // the "FILE" we read can be the magic string of the file record, but it can be a simple
	  // data of the user. Then, check we can find what we search, before to decode the record
	  
	  // check the record contains the unicode name of the record we search
	  for (i=0; i < (int)(m_info.dwFileRecordSize - nUnicodeNameLen); i++)
	    {
	      if (memcmp(cFileRecord+i, szUnicodeName, nUnicodeNameLen) == 0) // if record contains the name
		{
		  showDebug(1, "begin of checkFilenameForRecordIs()\n");
		  nRes = checkFilenameForRecordIs(cFileRecord, szFilename, false);
		  showDebug(1, "end of checkFilenameForRecordIs(), return = %d\n", nRes);
		  if (nRes == 0)
		    {
		      showDebug(1, "searchForRecordFilename(): experimental search succeed. Offset = %llu\n", qwCurrentOffset);
		      g_interface->msgBoxOk(i18n("$bitmap record search"), i18n("experimental search succeed"));
		      return 0; // success
		    }
		}
	    }
	  
	  showDebug(1, "searchForRecordFilename: end FILE detected\n");	
	  
	}
    }
  
  RETURN_int(-1);
}

