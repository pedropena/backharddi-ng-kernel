/***************************************************************************
                          gui_text.h  -  description
                             -------------------
    begin                : Tue Aug 22 2000
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

#ifndef GUI_TEXT_H
#define GUI_TEXT_H

#include "partimage.h"
#include "misc.h"

#include <newt.h>

#define KEY_OKAY NEWT_KEY_F5
#define KEY_EXIT NEWT_KEY_F6
#define KEY_BACK NEWT_KEY_F7

int fillPartitionList(newtComponent editPartition);

#define MBR_RESTORE_WHOLE 1
#define MBR_RESTORE_BOOT  2
#define MBR_RESTORE_TABLE 3

// =======================================================
class COptionsWindow
{
 public:
  int runForm();
  void destroyForm();
  void addHotKeys();
  void addButtons();
	
 protected:
  newtComponent m_formMain;
  newtComponent m_btnOkay, m_btnExit, m_btnBack;
};

// =======================================================
class CRestoreMbrWindow: public COptionsWindow
{
 public:
  int create();
  void addHardDisk(char *szText, DWORD dwNumber);
  void addMbr(char *szText, DWORD dwNumber);
  //int runForm();
  //void destroyForm();
  void getValues(DWORD *dwCurrentMbrNb, DWORD *dwOriginalMbrNb, int *nRestoreMode);
	
 private:
  //newtComponent m_formMain;
  newtComponent m_labelList1, m_list1, m_labelList2, m_list2; 
  //m_btnRestore, m_btnCancel;
  newtComponent m_labelType, m_radioFull, m_radioBoot, m_radioTable;
};

// =======================================================
class CSaveOptWindow: public COptionsWindow
{
 public:
  int create(char *szImageFile, COptions options);
  //int runForm();
  //void destroyForm();
  int getValues(COptions *options);
	
 private:
  //newtComponent m_formMain;
  newtComponent m_labelCompression, m_labelOptions, m_labelSplit;
  newtComponent m_radioCompNone, m_radioCompLzop, m_radioCompGzip, m_radioCompBzip2;
  newtComponent m_radioSplitSize, m_labelSplitSizeKB, m_radioSplitAuto;
  newtComponent m_checkCheckBeforeSaving, m_checkAskDesc, m_checkOverwrite;
  newtComponent m_editSplitSize, m_checkSplitWait;
  //newtComponent m_btnOkay, m_btnExit, m_btnBack;
  newtComponent m_labelFinish, m_radioFinishWait, m_radioFinishHalt, m_radioFinishReboot, m_radioFinishQuit, m_radioFinishLast;
};

// =======================================================
class CRestoreOptWindow: public COptionsWindow
{
 public:
  int create(char *szDevice, char *szImageFile, COptions options);
  //int runForm();
  //void destroyForm();
  int getValues(COptions *options);
	
private:
  //newtComponent m_formMain;
  newtComponent m_labelOptions;
  newtComponent m_checkEraseWithNull, m_checkSimulateMode;
  //newtComponent m_btnRestore, m_btnExit;
  newtComponent m_labelFinish, m_radioFinishWait, m_radioFinishHalt, m_radioFinishReboot, m_radioFinishQuit;
};

// =======================================================
class CSavingWindow
{
 public:
  int create(const char *szDevice, const char *szImageFile, const char *szFilesystem, QWORD qwPartSize, COptions options);
  int runForm();
  void destroyForm();
  void showImageFileInfo(char *szImageFile, QWORD qwFreeSpace, QWORD qwImageSize, char *szFullyBatchMode);
  void showStats(const time_t timeStart, QWORD qwBlockSize, QWORD qwBlocksDone, QWORD qwBlocksTotal, char *szFullyBatchMode);
  
 private:
  newtComponent m_formMain;
  newtComponent m_labelPartition, m_labelPartitionSize, m_labelImageFile, m_labelImageFileSize, m_labelFS;
  newtComponent m_labelCompression;
  newtComponent m_labelFreeSpace;
  newtComponent m_progressSaving, m_labelPercent;
  newtComponent m_labelStatsTime, m_labelStatsTimeRemaining, m_labelStatsSpeed, m_labelStatsSpace;
};

// =======================================================
class CRestoringWindow
{
 public:
  int create(char *szDevice, char *szImageFile, QWORD qwCurPartSize, DWORD dwCompressionMode, char *szOriginalDevice, char *szFileSystem, tm dateCreate, QWORD qwOrigPartSize, COptions * options);
  int runForm();
  void destroyForm();
  void showImageFileInfo(char *szImageFile, int nCompressionMode, char *szFullyBatchMode);
  void showStats(const time_t timeStart, QWORD qwBlockSize, QWORD qwBlocksDone, QWORD qwBlocksUsed, QWORD qwBlocksTotal, bool bEraseWithNull, char *szFullyBatchMode);
	
private:
  newtComponent m_formMain;
  newtComponent m_labelPartition, m_labelPartitionSize, m_labelImageFile, m_labelFS, m_labelCompression;
  newtComponent m_labelOriginalPartitionSize, m_labelDate, m_labelOldDevice;
  newtComponent m_progressRestoring, m_labelPercent;
  newtComponent m_labelStatsTime, m_labelStatsTimeRemaining, m_labelStatsSpeed, m_labelStatsSpace;
};

// =======================================================
class CExceptionsGUI
{
public:
  CExceptionsGUI() {szNewString[0] = '\0';}
  ~CExceptionsGUI() {}
  unsigned int windowError(char *szTitle, char *szText, char *szButton, char *szCurPath);
  char * getNewString() { return szNewString; }
  unsigned int windowAlreadyExist(char * img, char * path);
  unsigned int windowLocked(char * img, char * path);
  unsigned int windowWrongPath(char * szFilename);

private:
  char szNewString[1024];
};

#endif // GUI_TEXT_H

