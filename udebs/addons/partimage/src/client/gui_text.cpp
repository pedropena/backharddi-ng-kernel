/***************************************************************************
                          gui_text.cpp  -  description
                             -------------------
    begin                : Tue Aug 22 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/
// $Revision: 1.25 $
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

#include "gui_text.h"
#include "partimage.h"
#include "misc.h"
#include "interface_newt.h"

#include <newt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// =======================================================
void CRestoreMbrWindow::addHardDisk(char *szText, DWORD dwNumber)
{
  BEGIN;
  newtListboxAppendEntry(m_list1, szText, (void*)dwNumber);
  RETURN;
}


// =======================================================
void CRestoreMbrWindow::addMbr(char *szText, DWORD dwNumber)
{
  BEGIN;
  newtListboxAppendEntry(m_list2, szText, (void*)dwNumber);
  RETURN;
}

// =======================================================
int CRestoreMbrWindow::create()
{
  BEGIN;

  char szTemp[2048];
  
  SNPRINTF(szTemp, "%s", i18n("Restore an MBR to the hard disk"));
  newtCenteredWindow(78, 20, szTemp);
  
  m_labelList1 = newtLabel(1, 1, i18n("Disk with the MBR to restore"));
  m_list1 = newtListbox(1, 2, 8, NEWT_FLAG_SCROLL);

  m_labelList2 = newtLabel(35, 1, i18n("Original MBR to use"));
  m_list2 = newtListbox(35, 2, 8, NEWT_FLAG_SCROLL);

  m_labelType = newtLabel(1, 12, i18n("What to restore:"));

  m_radioFull = newtRadiobutton(1, 13, i18n("The whole MBR"), true, NULL);
  m_radioBoot = newtRadiobutton(1, 14, "Only the boot loader", false, m_radioFull);
  m_radioTable = newtRadiobutton(1, 15, "Only the primary partitions table", false, m_radioBoot);

  addButtons();

  m_formMain = newtForm(NULL, NULL, 0);
  newtFormAddComponents(m_formMain, m_labelList1, m_list1, m_labelList2, 
			m_list2, m_labelType, m_radioFull, m_radioBoot, m_radioTable, NULL);
  			//m_btnOkay, m_btnCancel, NULL);
  addHotKeys();

  newtDrawForm(m_formMain);
  RETURN_int(0);	
}

// =======================================================
void CRestoreMbrWindow::getValues(DWORD *dwCurrentMbrNb, DWORD *dwOriginalMbrNb, int *nRestoreMode)
{
  BEGIN;

  *dwCurrentMbrNb = (DWORD) newtListboxGetCurrent(m_list1);
  *dwOriginalMbrNb = (DWORD) newtListboxGetCurrent(m_list2);

  if (newtRadioGetCurrent(m_radioFull) == m_radioFull)
    *nRestoreMode = MBR_RESTORE_WHOLE;
  else if (newtRadioGetCurrent(m_radioFull) == m_radioBoot)
    *nRestoreMode = MBR_RESTORE_BOOT;
  else if (newtRadioGetCurrent(m_radioFull) == m_radioTable)
    *nRestoreMode = MBR_RESTORE_TABLE;

  RETURN;
}

// =======================================================
char *skipPartitionsEntries(char *cPtr)
{
  char *cOldPtr;
  int nMajor, nMinor, nBlocks;
  int nPartNum;
  char szDevice[128];

  cOldPtr = cPtr;
  nPartNum = 1;

  while (*(cPtr) && *(cPtr+1) && *(cPtr+2) && nPartNum)
  {
    cPtr = decodePartitionEntry(cPtr, &nMajor, &nMinor, &nBlocks, &nPartNum, szDevice);
    if (nPartNum)
      cOldPtr = cPtr;
  }

  return cOldPtr;
}


// =======================================================
char *processHardDrive(char *cPtr, newtComponent editPartition)
{
  char *cOldPtr;
  int nMajor, nMinor, nBlocks;
  int nPartNum;
  char szDevice[128];
  char szFullDevice[128];
  char cFormat[1024];
  char szTemp[1024];
  char *szDeviceName;
  int nRes;
  QWORD qwSize;

  cOldPtr = cPtr;
  nPartNum = 1;

  showDebug(9, "decode HD\n");

  while (*(cPtr) && *(cPtr+1) && *(cPtr+2) && nPartNum)
    {
      cPtr = decodePartitionEntry(cPtr, &nMajor, &nMinor, &nBlocks, &nPartNum, szDevice);
      if (nPartNum)
	{
	  cOldPtr = cPtr;

	  memset(cFormat, ' ', 50);
	  memset(cFormat+50, 0, 50);
	  
	  memcpy(cFormat, szDevice, strlen(szDevice)); // Device
	  
	  SNPRINTF(szFullDevice, "/dev/%s", szDevice);
	  checkInodeForDevice(szFullDevice);
	  
	  if (nBlocks > 1) // a standard device
	    {
	      nRes = detectFileSystem(szFullDevice, szTemp);
	      memcpy(cFormat+37, szTemp, strlen(szTemp)); // File system
	      
	      qwSize = getPartitionSize(szFullDevice);
	      formatSize(qwSize, szTemp);	
	      memcpy(cFormat+50, szTemp, strlen(szTemp)); // Size
	    }
	  else if (nBlocks == 1) // an extended partition
	    {
	      SNPRINTF(szTemp, "-extended-");
	      memcpy(cFormat+37, szTemp, strlen(szTemp)); // File system
	    }
	  
          szDeviceName = strdup(szFullDevice); // TOTO: never freed
	  newtListboxAppendEntry(editPartition, cFormat, (void*)szDeviceName);
          showDebug(9, "inserted[2]: %s\n", cFormat);
	  //debugWin("add[%s] and *cPtr=%d and 1=%d and 2=%d and 3=%d and 4=%d", cFormat, *(cPtr), *(cPtr+1), *(cPtr+2), *(cPtr+3), *(cPtr+4));
	}
    }

  return cOldPtr;
}

// =======================================================
int fillPartitionList(newtComponent editPartition)
{
  BEGIN;

  FILE *fPart;
  int nMajor, nMinor, nBlocks;
  char szDevice[128];
  char szFullDevice[128];
  char cBuffer[32768];
  char cFormat[1024];
  char szTemp[1024];
  char *cPtr;
  int nLines;
  unsigned int nSize;
  QWORD qwSize;
  int i;
  int nRes;
  char *szDeviceName;
  int nPartNum;
  
  errno = 0;
  fPart = fopen("/proc/partitions", "rb");
  if (!fPart)
    {	
      g_interface->msgBoxError(i18n("Cannot read \"/proc/partitions\" (%s). Then, you must use the "
				    "command line to run Partition Image. Type \"partimage --help\" for help."), strerror(errno));
      RETURN_int(-1);	
    }
  
  nSize = 0;
  nLines = 0;
  memset(cBuffer, 0, sizeof(cBuffer));
  showDebug(9, "Reading /proc/partitions file\n");
  while (nSize < sizeof(cBuffer) && !feof(fPart))
    {
      nRes = fgetc(fPart);
      if (nRes != -1)
        cBuffer[nSize] = nRes;
      else
        showDebug(9, "error in fgetc\n");
      if (cBuffer[nSize] == '\n')
        {
	  ++nLines;
          showDebug(9, "%d lines read\n", nLines);
        }
      ++nSize;
    }
  if (nSize < sizeof(cBuffer)) 
    cBuffer[nSize] = '\0';
  else
    cBuffer[sizeof(cBuffer)-1] = '\0';

  fclose(fPart);
  showDebug(9, "/proc/partitions read -> \n%s\n", cBuffer);
  
  cPtr = cBuffer;
  
  // skip two first lines
  nLines -= 2;
  for (i=0; i < 2; i++)
    {
      while (*cPtr != '\n')
	cPtr++;
      cPtr++;
    }
  
  while (*(cPtr) && *(cPtr+1)&& *(cPtr+2))
  //for (i=0; i < nLines; i++) // TO BE CHANGED
    {
      cPtr = decodePartitionEntry(cPtr, &nMajor, &nMinor, &nBlocks, &nPartNum, szDevice);
      
      // detect file system of the hard disk
      SNPRINTF(szFullDevice, "/dev/%s", szDevice);
      checkInodeForDevice(szFullDevice);
      nRes = detectFileSystem(szFullDevice, szTemp);

      showDebug(9, "device %s\n", szDevice);     
 
      if (nRes != -1) // if a removable media
	{
          showDebug(9, "removable media found: %s\n", szTemp);
	  // add media
	  memset(cFormat, ' ', 50);
	  memset(cFormat+50, 0, 50);
	  
	  memcpy(cFormat, szDevice, strlen(szDevice)); // Device

	  memcpy(cFormat+37, szTemp, strlen(szTemp)); // File system
	  
	  qwSize = getPartitionSize(szFullDevice);
	  formatSize(qwSize, szTemp);	
	  memcpy(cFormat+50, szTemp, strlen(szTemp)); // Size

          szDeviceName = strdup(szFullDevice); // TODO: never freed
	  newtListboxAppendEntry(editPartition, cFormat, (void*)szDeviceName);
          showDebug(9, "inserted: %s\n", cFormat);

	  // skip partitions
	  cPtr = skipPartitionsEntries(cPtr);
	}
      else // if an hard disk
	{
	  cPtr = processHardDrive(cPtr, editPartition);
	}
    }

  RETURN_int(0);
}

// =======================================================
void COptionsWindow::destroyForm()
{
  BEGIN;
  newtFormDestroy(m_formMain);
  newtPopWindow();
  RETURN;
}

// =======================================================
void COptionsWindow::addHotKeys()
{
  newtFormAddComponents(m_formMain, m_btnOkay, m_btnExit, m_btnBack, NULL);
  newtFormAddHotKey(m_formMain, KEY_EXIT); // Exit
  newtFormAddHotKey(m_formMain, KEY_OKAY); // Okay
  newtFormAddHotKey(m_formMain, KEY_BACK); // Back 
}

// =======================================================
void COptionsWindow::addButtons()
{
  m_btnOkay = newtCompactButton(7, 19, i18n("Continue (F5)"));
  m_btnExit = newtCompactButton(30, 19, i18n("Exit (F6)"));
  m_btnBack = newtCompactButton(52, 19, i18n("Main window (F7)"));
}

// =======================================================
int CSaveOptWindow::create(char *szImageFile, COptions options)
{
  BEGIN;

  char szTemp[2048];
  
  newtCenteredWindow(78, 20, i18n("save partition to image file"));
  
  m_labelCompression = newtLabel(1, 1, i18n("Compression level"));
  m_radioCompNone = newtRadiobutton(1, 2, i18n("None (very fast + very big file)"), options.dwCompression == COMPRESS_NONE, NULL);
  m_radioCompGzip = newtRadiobutton(1, 3, "Gzip (.gz: medium speed + small image file)", options.dwCompression == COMPRESS_GZIP, m_radioCompNone);
  m_radioCompBzip2 = newtRadiobutton(1, 4, "Bzip2 (.bz2: very slow + very small image file)", options.dwCompression == COMPRESS_BZIP2, m_radioCompGzip);

  m_labelOptions = newtLabel(1, 7, i18n("Options"));
  m_checkCheckBeforeSaving = newtCheckbox(1, 8, i18n("Check partition before saving"), (!!options.bCheckBeforeSaving ? 'X' : ' '), " X", NULL);
  m_checkAskDesc = newtCheckbox(1, 9, i18n("Enter description"), (!!options.bAskDesc ? 'X' : ' '), " X", NULL);
  m_checkOverwrite = newtCheckbox(1, 10, i18n("Overwrite without prompt"), (!!options.bOverwrite ? 'X' : ' '), " X", NULL);

  m_labelSplit = newtLabel(1, 12, i18n("Image split mode"));
  m_radioSplitAuto = newtRadiobutton(1, 13, i18n("Automatic split (when no space left)"), !options.qwSplitSize, NULL);
  m_radioSplitSize = newtRadiobutton(1, 14, i18n("Into files whose size is:............"), !!options.qwSplitSize, m_radioSplitAuto);
  SNPRINTF(szTemp, "%llu", (!!options.qwSplitSize) ? (options.qwSplitSize/1024/1024) : 2048);
  m_editSplitSize = newtEntry(43, 14, szTemp, 8, NULL, 0);
  m_labelSplitSizeKB = newtLabel(52, 14, i18n("MiB"));
  m_checkSplitWait = newtCheckbox(1, 15, i18n("Wait after each volume change"), (!!options.bSplitWait ? 'X' : ' '), " X", NULL);

  m_labelFinish = newtLabel(43, 7, i18n("If finished successfully:"));
  m_radioFinishWait = newtRadiobutton(43, 8, i18n("Wait"), options.dwFinish == FINISH_WAIT, NULL);
  m_radioFinishHalt = newtRadiobutton(43, 9, i18n("Halt"), options.dwFinish == FINISH_HALT, m_radioFinishWait);
  m_radioFinishReboot = newtRadiobutton(43, 10, i18n("Reboot"), options.dwFinish == FINISH_REBOOT, m_radioFinishHalt);
  m_radioFinishQuit = newtRadiobutton(43,11,i18n("Quit"), options.dwFinish == FINISH_QUIT, m_radioFinishReboot);
  m_radioFinishLast = newtRadiobutton(43,12,i18n("Last"), options.dwFinish == FINISH_LAST, m_radioFinishQuit);

  addButtons();
  
  m_formMain = newtForm(NULL, NULL, 0);
  newtFormAddComponents(m_formMain, m_labelCompression, m_labelOptions, m_labelSplit, NULL);
  newtFormAddComponents(m_formMain, m_radioCompNone, m_radioCompGzip, m_radioCompBzip2, m_checkCheckBeforeSaving, m_checkAskDesc, m_checkOverwrite, NULL);
  newtFormAddComponents(m_formMain, m_labelFinish, m_radioFinishWait, m_radioFinishHalt, m_radioFinishReboot, m_radioFinishQuit, m_radioFinishLast, NULL);	
  newtFormAddComponents(m_formMain, m_radioSplitAuto, m_radioSplitSize, m_labelSplitSizeKB, m_editSplitSize, m_checkSplitWait, NULL);
  addHotKeys();
  
  newtDrawForm(m_formMain);
  RETURN_int(0);	
}

// =======================================================
int CSaveOptWindow::getValues(COptions *options)
{
  BEGIN;

  // get options
  options->bAskDesc = (newtCheckboxGetValue(m_checkAskDesc) == 'X');
  options->bCheckBeforeSaving = (newtCheckboxGetValue(m_checkCheckBeforeSaving) == 'X');
  options->bOverwrite = (newtCheckboxGetValue(m_checkOverwrite) == 'X');
  options->bSplitWait = (newtCheckboxGetValue(m_checkSplitWait) == 'X');
  
  // get compression level
  if (newtRadioGetCurrent(m_radioCompNone) == m_radioCompNone)
    options->dwCompression = COMPRESS_NONE;
  else if (newtRadioGetCurrent(m_radioCompNone) == m_radioCompGzip)
    options->dwCompression = COMPRESS_GZIP;
  else if (newtRadioGetCurrent(m_radioCompNone) == m_radioCompBzip2)
    options->dwCompression = COMPRESS_BZIP2;
  
  // get finish level
  if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishWait)
    options->dwFinish = FINISH_WAIT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishHalt)
    options->dwFinish = FINISH_HALT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishReboot)
    options->dwFinish = FINISH_REBOOT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishQuit)
    options->dwFinish = FINISH_QUIT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishLast)
    options->dwFinish = FINISH_LAST;
  
  // get split mode
  if (newtRadioGetCurrent(m_radioSplitAuto) == m_radioSplitAuto)
    options->qwSplitSize = 0LL;//options->dwSplitMode = SPLIT_AUTO;
  else if (newtRadioGetCurrent(m_radioSplitAuto) == m_radioSplitSize)
    {	
      //options->dwSplitMode = SPLIT_SIZE;
#ifdef HAVE_ATOLL
      options->qwSplitSize = atoll(newtEntryGetValue(m_editSplitSize)) * 1024 * 1024;
#else
  #ifdef HAVE_STRTOLL
      options->qwSplitSize = strtoll(newtEntryGetValue(m_editSplitSize), 
         NULL, 10) * 1024 * 1024;
  #else
    #error "no function to convert string to long long int"
  #endif // HAVE_STRTOLL
#endif // HAVE_ATOLL
      showDebug(1, "qwSplitSize = %llu\n", options->qwSplitSize);
    }
  /*else if (newtRadioGetCurrent(m_radioSplitNone) == m_radioSplitAuto)
    {
      options->dwSplitMode = SPLIT_AUTO;
      options->dwSplitSize = 0;
    }*/
  
  RETURN_int(0);
}

// =======================================================
int COptionsWindow/*CSaveOptWindow*/::runForm()
{
  BEGIN;
	  
  newtComponent widgetTemp;
  newtExitStruct event;

  /*widgetTemp = */newtFormRun(m_formMain, &event);
  
  widgetTemp = newtFormGetCurrent(m_formMain);
  
  if (((event.reason == event.NEWT_EXIT_HOTKEY) && (event.u.key == KEY_EXIT)) || ((event.reason == event.NEWT_EXIT_COMPONENT) && (widgetTemp == m_btnExit))) 
    RETURN_int(KEY_EXIT);
  
  if (((event.reason == event.NEWT_EXIT_HOTKEY) && (event.u.key == KEY_OKAY)) || ((event.reason == event.NEWT_EXIT_COMPONENT) && (widgetTemp == m_btnOkay))) 
    RETURN_int(KEY_OKAY);
  
  if (((event.reason == event.NEWT_EXIT_HOTKEY) && (event.u.key == KEY_BACK)) || ((event.reason == event.NEWT_EXIT_COMPONENT) && (widgetTemp == m_btnBack))  ) 
    RETURN_int(KEY_BACK);
  
  RETURN_int(-1);
}

// =======================================================
int CSavingWindow::create(const char *szDevice, const char *szImageFile, const char *szFilesystem, QWORD qwPartSize, COptions options)
{
  BEGIN;

  char szTemp[1024];
  char szTemp2[1024];
  
  SNPRINTF(szTemp, i18n("save partition to image file"));
  newtCenteredWindow(78, 20, szTemp);
  
  SNPRINTF(szTemp, i18n("Partition to save:...........%s"), szDevice);
  m_labelPartition = newtLabel(1, 0, szTemp);
  
  SNPRINTF(szTemp, i18n("Size of the Partition:.......%s = %llu bytes"), formatSize(qwPartSize, szTemp2), qwPartSize);
  m_labelPartitionSize = newtLabel(1, 1, szTemp);
  
  SNPRINTF(szTemp, i18n("Image file to create.........%s"), szImageFile);
  m_labelImageFile = newtLabel(1, 2, szTemp);
  m_labelImageFileSize = newtLabel(1, 3, "");
  
  m_labelFreeSpace = newtLabel(1, 4, "");

  SNPRINTF(szTemp, i18n("Detected file system:........%s"), szFilesystem);
  m_labelFS = newtLabel(1, 5, szTemp);
  
  switch (options.dwCompression)
    {
    case COMPRESS_NONE:
      SNPRINTF(szTemp, i18n("Compression level:...........None"));
      break;
    case COMPRESS_GZIP:
      SNPRINTF(szTemp, i18n("Compression level:...........gzip"));
      break;
    case COMPRESS_BZIP2:
      SNPRINTF(szTemp, i18n("Compression level:...........bzip2"));
      break;
    case COMPRESS_LZO:
      SNPRINTF(szTemp, i18n("Compression level:...........lzo"));
      break;
    }
  m_labelCompression = newtLabel(1, 6, szTemp);

  // stats
  m_labelStatsTime = newtLabel(1, 9, "");
  m_labelStatsTimeRemaining = newtLabel(1, 10, "");
  m_labelStatsSpeed = newtLabel(1, 11, "");
  m_labelStatsSpace = newtLabel(1, 12, "");

  m_progressSaving = newtScale(1, 18, 70, 100);
  m_labelPercent = newtLabel(72, 18, "");
  
  m_formMain = newtForm(NULL, NULL, 0);
  newtFormAddComponents(m_formMain, m_labelPartition, m_labelPartitionSize, m_labelImageFile, m_labelImageFileSize, m_labelFreeSpace, m_labelFS, m_labelCompression, NULL);
  newtFormAddComponents(m_formMain, m_labelStatsTime, m_labelStatsTimeRemaining, m_labelStatsSpeed, m_labelStatsSpace, NULL);
  newtFormAddComponents(m_formMain, m_progressSaving, m_labelPercent, NULL);

  newtRefresh();
  newtDrawForm(m_formMain);
  
  RETURN_int(0);	
}

// =======================================================
void CSavingWindow::showStats(const time_t timeStart, QWORD qwBlockSize, QWORD qwBlocksDone, QWORD qwBlocksTotal, char *szFullyBatchMode)
{
  BEGIN;

  char szTemp[1024];
  char szTemp2[1024];
  char szTemp3[1024];

  time_t timeElapsed, timeRemaining;
  QWORD qwBytesPerMin;
  float fMinElapsed;
  QWORD qwPercent;

  // progress bar
  qwPercent = (100 * qwBlocksDone) / qwBlocksTotal;
  SNPRINTF(szTemp, "%d %%", (int)qwPercent);
  newtScaleSet(m_progressSaving, (int)qwPercent);
  newtLabelSetText(m_labelPercent, szTemp);

  // stats
  if (timeStart != 0)
    {
      QWORD qwDone = qwBlockSize * qwBlocksDone;
      QWORD qwTotal = qwBlockSize * qwBlocksTotal;
      
      timeElapsed = time(0) - timeStart;
      timeRemaining = (timeElapsed * (qwTotal - qwDone)) / qwDone;

      SNPRINTF(szTemp, i18n("Time elapsed:................%s"), formatTime((DWORD)timeElapsed, szTemp2));
      newtLabelSetText(m_labelStatsTime, szTemp);
      
      SNPRINTF(szTemp, i18n("Estimated time remaining:....%s"), formatTime((DWORD)timeRemaining, szTemp2));
      newtLabelSetText(m_labelStatsTimeRemaining, szTemp);
      
      fMinElapsed = ((float)timeElapsed) / 60.0;
      qwBytesPerMin = (QWORD) (((float)qwDone) / fMinElapsed);
      SNPRINTF(szTemp, i18n("Speed:.......................%s/min"), formatSize(qwBytesPerMin, szTemp2));
      newtLabelSetText(m_labelStatsSpeed, szTemp);
      
      SNPRINTF(szTemp, i18n("Data copied:.................%s / %s"), formatSize(qwDone, szTemp2), formatSize(qwTotal, szTemp3));
      newtLabelSetText(m_labelStatsSpace, szTemp);
      //option  -B gui=no show stats
      if ((szFullyBatchMode) && (strlen(szFullyBatchMode)>0))
      {
        SNPRINTF(szTemp, "stats: %3d%%", (int)qwPercent);
        fprintf(stderr,"%-s ",szTemp);
        SNPRINTF(szTemp, i18n("%-s %-s"), formatTimeNG((DWORD)timeElapsed, szTemp2), formatTimeNG((DWORD)timeRemaining, szTemp3));
        fprintf(stderr,"%-s ",szTemp);
        SNPRINTF(szTemp, i18n("%s/min"), formatSizeNG(qwBytesPerMin, szTemp2));
        fprintf(stderr,"%-s\n",szTemp);
      }
    }

  newtRefresh();
  RETURN;
}

// =======================================================
int CSavingWindow::runForm()
{
  BEGIN;
  newtRefresh();
  RETURN_int(0);
}

// =======================================================
void CSavingWindow::destroyForm()
{
  BEGIN;
  newtPopWindow();
  RETURN;
}

// =======================================================
void CSavingWindow::showImageFileInfo(char *szImageFile, QWORD qwFreeSpace, QWORD qwImageSize, char *szFullyBatchMode)
{
  BEGIN;

  char szTemp[2048];
  char szTemp2[2048];
  
  SNPRINTF(szTemp, i18n("Current image file:..........%s"), szImageFile);
  newtLabelSetText(m_labelImageFile, szTemp);

  if (qwImageSize)
    {
      SNPRINTF(szTemp, i18n("Image file size:.............%s"), formatSize(qwImageSize, szTemp2));
      newtLabelSetText(m_labelImageFileSize, szTemp);
    }
  
  SNPRINTF (szTemp, i18n("Available space for image:...%s = %llu bytes"), formatSize(qwFreeSpace, szTemp2), qwFreeSpace);
  newtLabelSetText(m_labelFreeSpace, szTemp);

  RETURN;
}

// =======================================================
int CRestoreOptWindow::create(char *szDevice, char *szImageFile, COptions options)
{
  BEGIN;

  char szTemp[1024];
  
  SNPRINTF(szTemp, i18n("restore partition from image file"));
  newtCenteredWindow(78, 20, szTemp);
  
  m_labelOptions = newtLabel(1, 1, i18n("Options"));

  m_checkEraseWithNull = newtCheckbox(1, 3, i18n("Erase free blocks with zero values"), (!!options.bEraseWithNull ? 'X' : ' '), " X", NULL);
  m_checkSimulateMode = newtCheckbox(1, 2, i18n("Simulation of the restoration (nothing is written)"), (!!options.bSimulateMode ? 'X' : ' '), " X", NULL);

  m_labelFinish = newtLabel(1, 5, i18n("If finished successfully:"));
  m_radioFinishWait = newtRadiobutton(1, 6, i18n("Wait"), options.dwFinish == FINISH_WAIT, NULL);
  m_radioFinishHalt = newtRadiobutton(1, 7, i18n("Halt"), options.dwFinish == FINISH_HALT, m_radioFinishWait);
  m_radioFinishReboot = newtRadiobutton(1, 8, i18n("Reboot"), options.dwFinish == FINISH_REBOOT, m_radioFinishHalt);
  m_radioFinishQuit = newtRadiobutton(1, 9, i18n("Quit"), options.dwFinish == FINISH_QUIT, m_radioFinishReboot);
  
  addButtons();

  m_formMain = newtForm(NULL, NULL, 0);
  newtFormAddComponents(m_formMain, m_labelOptions, m_checkSimulateMode, m_checkEraseWithNull, NULL);
  newtFormAddComponents(m_formMain, m_labelFinish, m_radioFinishWait, m_radioFinishHalt, m_radioFinishReboot, m_radioFinishQuit, NULL);	
  //newtFormAddComponents(m_formMain, m_btnRestore, m_btnExit, NULL);
  addHotKeys();

  newtDrawForm(m_formMain);
  
  RETURN_int(0);	
}

// =======================================================
int CRestoreOptWindow::getValues(COptions *options)
{
  BEGIN;

  // get options
  options->bEraseWithNull = (newtCheckboxGetValue(m_checkEraseWithNull) == 'X');
  options->bSimulateMode = (newtCheckboxGetValue(m_checkSimulateMode) == 'X');

  // get finish level
  if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishWait)
    options->dwFinish = FINISH_WAIT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishHalt)
    options->dwFinish = FINISH_HALT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishReboot)
    options->dwFinish = FINISH_REBOOT;
  else if (newtRadioGetCurrent(m_radioFinishWait) == m_radioFinishQuit)
    options->dwFinish = FINISH_QUIT;
  
  RETURN_int(0);
}

// =======================================================
int CRestoringWindow::create(char *szDevice, char *szImageFile, QWORD qwCurPartSize, DWORD dwCompressionMode, char *szOriginalDevice, char *szFileSystem, tm dateCreate, QWORD qwOrigPartSize, COptions * options)
{
  BEGIN;
  
  char szTemp[2048];
  char szTemp2[2048];
 
  if (options->bSimulateMode)
    SNPRINTF(szTemp, i18n("simulate partition restoration from image file"));
  else
    SNPRINTF(szTemp, i18n("restore partition from image file"));
  newtCenteredWindow(78, 20, szTemp);
  
  SNPRINTF(szTemp, i18n("Partition to restore:.............%s"), szDevice);
  m_labelPartition = newtLabel(1, 0, szTemp);
  
  SNPRINTF(szTemp, i18n("Size of partition to restore:.....%s = %llu bytes"), formatSize(qwCurPartSize, szTemp2), qwCurPartSize);
  m_labelPartitionSize = newtLabel(1, 1, szTemp);
  
  SNPRINTF(szTemp, i18n("Image file to use.................%s"), szImageFile);
  m_labelImageFile = newtLabel(1, 2, szTemp);
  
  SNPRINTF(szTemp, i18n("File system:......................%s"), szFileSystem);
  m_labelFS = newtLabel(1, 3, szTemp);

  m_labelCompression = newtLabel(1, 4, "");

  SNPRINTF(szTemp, i18n("Partition was on device:..........%s\n"), szOriginalDevice);
  m_labelOldDevice = newtLabel(1, 5, szTemp);

  SNPRINTF(szTemp, i18n("Image created on:.................%s\n"), asctime(&dateCreate));
  m_labelDate = newtLabel(1, 6, szTemp);

  SNPRINTF(szTemp, i18n("Size of the original partition:...%s = %llu bytes"), formatSize(qwOrigPartSize, szTemp2), qwOrigPartSize);
  m_labelOriginalPartitionSize = newtLabel(1, 7, szTemp);

  // stats
  m_labelStatsTime = newtLabel(1, 9, "");
  m_labelStatsTimeRemaining = newtLabel(1, 10, "");
  m_labelStatsSpeed = newtLabel(1, 11, "");
  m_labelStatsSpace = newtLabel(1, 12, "");

  m_progressRestoring = newtScale(1, 18, 70, 100);
  m_labelPercent = newtLabel(72, 18, "");
  
  m_formMain = newtForm(NULL, NULL, 0);
  newtFormAddComponents(m_formMain, m_labelPartition, m_labelPartitionSize, m_labelImageFile, m_labelFS, m_labelCompression, NULL);
  newtFormAddComponents(m_formMain, m_labelOldDevice, m_labelDate, m_labelOriginalPartitionSize, NULL);
  newtFormAddComponents(m_formMain, m_labelStatsTime, m_labelStatsTimeRemaining, m_labelStatsSpeed, m_labelStatsSpace, NULL);
  newtFormAddComponents(m_formMain, m_progressRestoring, m_labelPercent, NULL);
  
  newtDrawForm(m_formMain);
  newtRefresh();

  RETURN_int(0);
}

// =======================================================
void CRestoringWindow::showStats(const time_t timeStart, QWORD qwBlockSize, QWORD qwBlocksDone, QWORD qwBlocksUsed, QWORD qwBlocksTotal, bool bEraseWithNull, char *szFullyBatchMode)
{
  BEGIN;

  char szTemp[1024];
  char szTemp2[1024];
  char szTemp3[1024];

  time_t timeElapsed, timeRemaining;
  QWORD qwBytesPerMin;
  float fMinElapsed;
  QWORD qwPercent;
  QWORD qwTotal;
  QWORD qwDone;
  
  if (bEraseWithNull == false)
    {
      qwPercent = (100 * qwBlocksDone) / qwBlocksUsed;
      qwTotal = qwBlockSize * qwBlocksUsed;
    }
  else
    {
      qwPercent = (100 * qwBlocksDone) / qwBlocksTotal;
      qwTotal = qwBlockSize * qwBlocksTotal;
    }
  
  SNPRINTF(szTemp, "%d %%", (int)qwPercent);
  
  newtScaleSet(m_progressRestoring, (int)qwPercent);
  newtLabelSetText(m_labelPercent, szTemp);

  if (timeStart != 0)
    {
      qwDone = qwBlockSize * qwBlocksDone;
      
      timeElapsed = time(0) - timeStart;

      if (bEraseWithNull)
	timeRemaining = (timeElapsed * (qwBlocksTotal - qwBlocksDone)) / qwBlocksDone;
      else
	timeRemaining = (timeElapsed * (qwBlocksUsed - qwBlocksDone)) / qwBlocksDone;

      SNPRINTF(szTemp, i18n("Time elapsed:.....................%s"), formatTime((DWORD)timeElapsed, szTemp2));
      newtLabelSetText(m_labelStatsTime, szTemp);
      
      SNPRINTF(szTemp, i18n("Estimated time remaining:.........%s"), formatTime((DWORD)timeRemaining, szTemp2));
      newtLabelSetText(m_labelStatsTimeRemaining, szTemp);
      
      fMinElapsed = ((float)timeElapsed) / 60.0;
      qwBytesPerMin = (QWORD) (((float)qwDone) / fMinElapsed);
      SNPRINTF(szTemp, i18n("Speed:............................%s/min"), formatSize(qwBytesPerMin, szTemp2));
      newtLabelSetText(m_labelStatsSpeed, szTemp);
      
      SNPRINTF(szTemp, i18n("Data copied:......................%s / %s"), formatSize(qwDone, szTemp2), formatSize(qwTotal, szTemp3));
      newtLabelSetText(m_labelStatsSpace, szTemp);
      //option  -B gui=no show stats
      if ((szFullyBatchMode) && (strlen(szFullyBatchMode)>0))
      {
        SNPRINTF(szTemp, "stats: %3d%%", (int)qwPercent);
        fprintf(stderr,"%-s ",szTemp);
        SNPRINTF(szTemp, i18n("%-s %-s"), formatTimeNG((DWORD)timeElapsed, szTemp2), formatTimeNG((DWORD)timeRemaining, szTemp3));
        fprintf(stderr,"%-s ",szTemp);
        SNPRINTF(szTemp, i18n("%s/min"), formatSizeNG(qwBytesPerMin, szTemp2));
        fprintf(stderr,"%-s\n",szTemp);
      }
    }

  newtRefresh();
  RETURN;
}

// =======================================================
int CRestoringWindow::runForm()
{
  BEGIN;
  newtRefresh();
  RETURN_int(0);
}

// =======================================================
void CRestoringWindow::destroyForm()
{
  BEGIN;
  newtPopWindow();
  RETURN;
}

// =======================================================
void CRestoringWindow::showImageFileInfo(char *szImageFile, int nCompressionMode, char *szFullyBatchMode)
{
  BEGIN;
  
  char szTemp[2048];

  SNPRINTF(szTemp, i18n("Current image file:...............%s"), szImageFile);	
  newtLabelSetText(m_labelImageFile, szTemp);
  
  if (nCompressionMode != -1)
    {
      switch (nCompressionMode)
	{
	case COMPRESS_NONE:
	  SNPRINTF(szTemp, i18n("Compression level:................None\n"));
	  break;
	case COMPRESS_GZIP:
	  SNPRINTF(szTemp, i18n("Compression level:................gzip"));
	  break;
	case COMPRESS_BZIP2:
	  SNPRINTF(szTemp, i18n("Compression level:................bzip2"));
	  break;
	case COMPRESS_LZO:
	  SNPRINTF(szTemp, i18n("Compression level:................lzo"));
	  break;
	default:
	  memset(szTemp, 0, sizeof(szTemp));
	}

      newtLabelSetText(m_labelCompression, szTemp);
    }

  RETURN;
}

// ============================================================================
unsigned int CExceptionsGUI::windowError(char *szTitle, char *szText, char *szButton, char *szCurPath)
{
  BEGIN;
  //char szTemp[] = "/tmp/image";

  unsigned int nRes;
  newtComponent btnOther;
  
  newtComponent formMain = newtForm(NULL, NULL, 0);
  if (szButton)
    btnOther = newtButton(30, 15, szButton);
  else
    btnOther = newtButton(30, 15, "(none)");
  newtComponent btnOk = newtButton(10, 15, i18n("Change"));
  newtComponent btnCancel = newtButton(50, 15, i18n("Cancel"));
  newtComponent editFilename = newtEntry(5, 10, szCurPath, 45, NULL, 0);
  newtComponent labelFilename = newtLabel(5, 9, i18n("Enter new filename"));

  newtComponent labelText = newtTextbox(1, 1, 60, 7, 0);
  newtTextboxSetText(labelText, szText);

  newtComponent widgetTemp;

  newtCenteredWindow(67, 20, szTitle);
  if (szButton)
    newtFormAddComponents(formMain, editFilename, btnOk, btnOther, btnCancel,
       labelFilename, labelText, NULL);
  else
    newtFormAddComponents(formMain, editFilename, btnOk, btnCancel,
       labelFilename, labelText, NULL);

  widgetTemp = newtRunForm(formMain);

  if (widgetTemp == btnCancel)
    nRes = ERR_QUIT;
  else if (szButton && widgetTemp == btnOther)
    nRes = ERR_CONT;
  else
    {
      strncpy(szNewString, newtEntryGetValue(editFilename), 1023);
      *(szNewString+1023) = '\0';

      while (szNewString[strlen(szNewString)-1] == '/')
        szNewString[strlen(szNewString)-1] = '\0';

      nRes = ERR_RETRY;
    }
  newtFormDestroy(formMain);
  newtPopWindow();
  RETURN_WORD(nRes);
}

// ============================================================================
unsigned int CExceptionsGUI::windowAlreadyExist(char * img, char * path)
{
  BEGIN;

  char szMess[2048];
  int nCont;
  
  SNPRINTF(szMess, i18n("The following imagefile already exist.\nDo you"
     " want to overwrite or change image filename ?\nFile: %s\nTo change the"
     " filename, please enter a full path\n(location + filename) "
     "without volume number at the end, \nand press \"Change\""), img);

  nCont = windowError(i18n("Overwrite image?"), szMess, i18n("Overwrite"), path);
  RETURN_WORD(nCont);
}

// ============================================================================
unsigned int CExceptionsGUI::windowLocked(char * img, char * path)
{
  BEGIN;

  char szMess[2048];
  int nCont;
  
  SNPRINTF(szMess, i18n("The following imagefile is locked.\nDo you"
     " want to wait and retry or change image filename ?\nFile: %s\nTo change"
     " the filename, please enter a full path\n(location + filename) "
     "without volume number at the end, \nand press \"Change\""), img);

  nCont = windowError(i18n("imagefile locked"), szMess, i18n("Retry"), path);
  
  RETURN_WORD(nCont);
}

// ============================================================================
unsigned int CExceptionsGUI::windowWrongPath(char * szFilename)
{
  BEGIN;

  int nCont;
  char szMess[2048];

  SNPRINTF(szMess, i18n("unable to access to %s\n\nPlease, enter"
     "another directory path (without filename):"), szFilename); 

  nCont = windowError(i18n("wrong path"), szMess, NULL, NULL);
  
  RETURN_WORD(nCont);
}
