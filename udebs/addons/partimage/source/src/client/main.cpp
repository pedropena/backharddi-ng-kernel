/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : lun mai 22 18:04:54 CEST 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/
// $Revision: 1.70 $
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

#ifdef HAVE_GETOPT_H
  #include <getopt.h>
#endif

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/mount.h>
#include <errno.h>

#include "fs_ext2.h"
#include "fs_fat.h"
#include "fs_reiser.h"
#include "fs_ntfs.h"
#include "fs_hpfs.h"
#include "fs_jfs.h"
#include "fs_xfs.h"
#include "fs_hfs.h"
#include "fs_ufs.h"

#include "partimage.h"
#include "common.h"
#include "gui_text.h"
#include "net.h"
#include "misc.h"
#include "imginfo.h"
#include "buffer.h"
#include "exceptions.h"
#include "interface_newt.h"
#include "interface_none.h"
#include "mbr_backup.h"

extern char *optarg;
extern int optind;
extern int opterr;

// time
time_t g_timeBegin=0; // Beginning of main()
time_t g_timeStart=0; // Beginning of the copy, after confirmation
time_t g_timeEnd=0;   // End of the operation

#ifdef HAVE_GETOPT_H
static struct option const long_options[] =
{
  {"automnt", required_argument, NULL, 'a'},
  {"batch", no_argument, NULL, 'b'},
  {"fully-batch", required_argument, NULL, 'B'},
  {"nocheck", no_argument, NULL, 'c'},
  {"nodesc", no_argument, NULL, 'd'},
  {"erase", no_argument, NULL, 'e'},
  {"finish", required_argument, NULL, 'f'},
  {"debug", required_argument, NULL, 'g'},
  {"help", no_argument, NULL, 'h'},
  {"compilinfo", no_argument, NULL, 'i'},
  {"nombr", no_argument, NULL, 'M'},
  {"allowmnt", no_argument, NULL, 'm'},
  {"nossl", no_argument, NULL, 'n'},
  {"overwrite", no_argument, NULL, 'o'},
  {"password", required_argument, NULL, 'P'},
  {"port", required_argument, NULL, 'p'},
  {"simulate", no_argument, NULL, 'S'},
  {"server", required_argument, NULL, 's'},
  {"username", required_argument, NULL, 'U'},
  {"volume", required_argument, NULL, 'V'},
  {"version", no_argument, NULL, 'v'},
  {"waitvol", no_argument, NULL, 'w'},
  {"runshell", no_argument, NULL, 'X'},
  {"nosync", no_argument, NULL, 'y'},
  {"compress", required_argument, NULL, 'z'},
  {NULL, 0, NULL, 0}
};
#endif

static char optstring[]="z:oV:ecmdhf:s:p:bwg:vynSMa:iU:P:XB:";
FILE * g_fDebug; // debug file
FILE * g_fLocalDebug; // debug file
CInterface * g_interface;
bool g_bSigInt = false;
bool g_bSigKill = false;
QWORD g_qwCopiedBytesCount=0;
WORD g_wEndian=ENDIAN_UNKNOWN;

// =======================================================
static void catch_sigint(int signo)
{
  if (signo == SIGTERM)
    {
      g_bSigKill = true;
      delete g_interface;
      exit(0);
    }
  else if (signo == SIGINT)
    {
      g_bSigInt = true;
    }
  /*else if (signo == SIGSEGV)
    {
      delete g_interface;
      fprintf(stderr, i18n("Segmentation fault. Please report the bug and send the /var/log/partimage-debug.log file to authors\n"));
      exit(0);      
    }*/
}

// =======================================================
int main(int argc, char *argv[])
{
  int nRes;
  int nOptCh;
  COptions options;
  int nChoice;
  char szDevice[MAX_DEVICENAMELEN];
  char szImageFile[MAXPATHLEN];
  char szTemp[2048];
  char szTemp2[1024];
  char szTemp3[1024]; 
  char szAux[MAXPATHLEN+1];
  char szAux2[MAXPATHLEN+1];
  char szFileSystem[1024];

  // struct sched_param Param;

  // initialize time
  g_timeBegin = time(0);
  setEndianess(false);

  // initialize options with defaults values
  memset(&options, 0, sizeof(COptions));
  options.bUseSSL = OPTIONS_DEFAULT_SSL;
  options.bBackupMBR = OPTIONS_DEFAULT_BACKUP_MBR;
  options.bSimulateMode = OPTIONS_DEFAULT_SIMULATE_MODE;
  options.bOverwrite = OPTIONS_DEFAULT_OVERWRITE;
  options.qwSplitSize = OPTIONS_DEFAULT_SPLIT_SIZE;
  options.bEraseWithNull = OPTIONS_DEFAULT_ERASE_EMPTY;
  options.dwCompression = OPTIONS_DEFAULT_COMPRESS;
  options.bCheckBeforeSaving = OPTIONS_DEFAULT_CHECK;
  options.bFailIfMounted = OPTIONS_DEFAULT_FAIL_IF_MOUNTED;
  options.bAskDesc = OPTIONS_DEFAULT_ASK_DESC;
  options.dwFinish = OPTIONS_DEFAULT_FINISH;
  options.dwServerPort = OPTIONS_DEFAULT_SERVERPORT;
  options.bBatchMode = OPTIONS_DEFAULT_AUTOSTART;
  options.bSplitWait = OPTIONS_DEFAULT_SPLIT_WAIT;
  options.bRunShell = OPTIONS_DEFAULT_RUN_SHELL;
  options.bSync = OPTIONS_DEFAULT_SYNC;
  options.dwDebugLevel = DEFAULT_DEBUG_LEVEL; // defined by configure.in
  strncpy(options.szServerName, OPTIONS_DEFAULT_SERVERNAME, MAX_HOSTNAMESIZE);
  strncpy(options.szAutoMount, OPTIONS_DEFAULT_AUTOMOUNT, MAXPATHLEN);
  strncpy(options.szUserName, OPTIONS_DEFAULT_USERNAME, MAX_USERNAMELEN);
  strncpy(options.szPassWord, OPTIONS_DEFAULT_PASSWORD, MAX_PASSWORDLEN);
  *(options.szFullyBatchMode) = 0;
  
  // initialize language for i18n
  setlocale(LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  // Very important: do not remove (else, uncompatible image will be produced)
  if (checkStructSizes() == -1)
     return EXIT_FAILURE;

  // read command line
#ifdef HAVE_GETOPT_H
  while ((nOptCh = getopt_long (argc, argv, optstring, long_options, NULL)) != EOF)
#else
    while ((nOptCh = getopt (argc, argv, optstring)) != -1)
#endif
      {
	switch(nOptCh)
	  {
	  case 's': // partimaged server's ip addr
	    strncpy(options.szServerName, optarg, MAX_HOSTNAMESIZE);
            *(options.szServerName+MAX_HOSTNAMESIZE-1) = '\0';
	    break;
	  case 'a': // auto mount options
	    strncpy(options.szAutoMount, optarg, MAXPATHLEN);
            *(options.szAutoMount+MAXPATHLEN-1) = '\0';
	    break;
	  case 'U': // username for serverauth
            strncpy(options.szUserName, optarg, MAX_USERNAMELEN);
            *(options.szUserName+MAX_USERNAMELEN-1) = '\0';
            break;
	  case 'B': // fully batchmode: don't do any newt or anything
	    strncpy(options.szFullyBatchMode, optarg, 2047);
	    options.bBatchMode = true;
	    break;
	  case 'P': // password for serverauth
            strncpy(options.szPassWord, optarg, MAX_PASSWORDLEN);
            *(options.szPassWord+MAX_PASSWORDLEN-1) = '\0';
            break;
	  case 'b': // batchmode: don't wait for user action (return)
	    options.bBatchMode = true;
	    break;
	  case 'w': // wait after each new colume
	    options.bSplitWait = true;
	    break;
          case 'X': // run shell after all volume
            options.bRunShell = true;
            break; 
	  case 'M': // do not create a backup of the MBR
	    options.bBackupMBR = false;
	    break;
	  case 'y': // no sync at the end
	    options.bSync = false;
	    break;
	  case 'p': // partimaged server's listening port
	    options.dwServerPort = atol(optarg);
	    if ((options.dwServerPort < 1) || (options.dwServerPort > 65535))
	      {	
		fprintf(stderr, i18n("server's port must be between 1 and "
				     "65535\n"));
		return EXIT_FAILURE;
	      }
	    break;
	  case 'z': // compression level
	    options.dwCompression = atol(optarg);
	    if ((options.dwCompression < 0) || (options.dwCompression > 2))
	      {	
		fprintf(stderr, i18n("Compression mode must be 0 (none), "
				     "1(gzip), 2(bzip2)\n"));
		return EXIT_FAILURE;
	      }
	    break;
	  
	  case 'f': // what to do when finished successfully ?
	    options.dwFinish = atol(optarg);
	    if ((options.dwFinish < 0) || (options.dwFinish > 3))
	      {
		fprintf(stderr, i18n("Finish mode must be 0 (wait), "
				     "1(halt), 2(reboot) or 3(quit)\n"));
		return EXIT_FAILURE;
	      }
	    break;
	  
	  case 'g': // change the debug level
	    options.dwDebugLevel = atol(optarg);
	    if ((options.dwDebugLevel < 0) || (options.dwDebugLevel > 10))
	      {
		fprintf(stderr, i18n("The debug level must be 0(none), "
				     "1(user), 2(developer) or 3(debugging)\n"));
		return EXIT_FAILURE;
	      }
	    break;
	  
	  case 'V': // split image into multiple files
	  
	    options.qwSplitSize = atol(optarg) * 1024 * 1024;
	    fprintf (stderr, i18n("Volume size: %llu bytes (%ld MiB)\n"),
		     options.qwSplitSize, atol(optarg));
	    break;
	  
	  case 'o': // overwrite existring image
	    options.bOverwrite = true;
	    break;
	  
	  case 'e': // erase empty blocks with zero bytes
	    options.bEraseWithNull = true;
	    break;
	  
	  case 'm': // don't fail if mounted
	    options.bFailIfMounted = false;
	    break;
	  
	  case 'c': // don't check partition before saving
	    options.bCheckBeforeSaving = false;
	    break;
	  
	  case 'd': // don't ask any description
	    options.bAskDesc = false;
	    break;
	  
	  case 'h': // help
	    usage();
	    return EXIT_SUCCESS;
	    break;
	  
	  case 'v': // version
	    printf(i18n("Partition Image version %s (distributed under the"
			" GNU GPL2).\n"), PACKAGE_VERSION);
	    return EXIT_SUCCESS;
	    break;

	  case 'i': // compilation options
	    formatCompilOptions(szTemp, sizeof(szTemp));
	    printf("%s\n", szTemp);
	    return EXIT_SUCCESS;
	    break;

	  case 'n': // no ssl
	    options.bUseSSL = false;
	    break;
	  case 'S': // simulate
	    options.bSimulateMode = true;
	    break;
	  }
      }

  // debug file
  g_fDebug = NULL;
  if (options.dwDebugLevel)
    {
      SNPRINTF(szAux2, "%s_latest", PARTIMAGE_LOG);
#ifdef APPEND_PID // append pid at the end of PARTIMAGE_LOG filename
      SNPRINTF(szAux, "%s_%d", PARTIMAGE_LOG, getpid());
#else
      SNPRINTF(szAux, "%s", PARTIMAGE_LOG);
#endif 
      unlink(szAux2);
      symlink(szAux, szAux2);
      g_fDebug = openFileDescriptorSecure(szAux, "a", O_WRONLY | O_CREAT | O_NOFOLLOW | O_TRUNC, S_IRUSR | S_IWUSR);

      g_nDebugThreadMain = getpid();
      g_dwDebugLevel = options.dwDebugLevel;
      pthread_mutex_init(&g_mutexDebug, &g_mutexDebugAttr);
    }

  if (!g_fDebug)
    {	
      g_fDebug = fopen("/dev/null", "w");
      if (!g_fDebug)
        {
          fprintf(stderr, i18n("Cannot open debug file.\n"));
	  g_fDebug = stderr;
        }
      //return EXIT_FAILURE;
    }
  else
    {
      showDebug(1, "%s: Partition Image version %s (DebugLevel %lu used, "
		"MainThread=%d)\n",PARTIMAGE_LOG, PACKAGE_VERSION, options.dwDebugLevel, 
		getpid());
      showDebug(1, "========================================================="
		"==============================\n\n\n");

      formatCompilOptions(szTemp, sizeof(szTemp));
      showDebug(1, "%s\n\n", szTemp);

      /*      extractFilepathFromFullPath(PARTIMAGE_LOG, szAux); // filepath without filename
	      #ifdef APPEND_PID 
	      snprintf(szAux2, MAXPATHLEN, "%s/LATEST_PILOG_IS_%d", szAux, getpid());
	      #else
	      snprintf(szAux2, MAXPATHLEN, "%s/LATEST_PILOG_IS_nopid", szAux);
	      #endif
	      touchFile(szAux2);
      */
    }

  //if (fchmod(fileno(g_fDebug), S_IRUSR|S_IWUSR))
  //  showDebug(1, "can't change debugfile mode to 600: %s\n", strerror(errno));


  /*
    fprintf(g_fDebug, "\ntrying to change priority to -20...");
    nRes = nice(-20);
    if (nRes) 
    fprintf(g_fDebug, " failed: %s\n", strerror(errno));
    else
    fprintf(g_fDebug, " ok\n");
    
    fprintf(g_fDebug, "trying to change scheduling to increase speed\n");
    fprintf(g_fDebug, "  call to sched_get_priority_max...");
    fflush(g_fDebug);
    Param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (Param.sched_priority < 0)
    { 
    fprintf(g_fDebug, " failed\n");
    fprintf(g_fDebug, "%s\n", strerror(errno));
    fprintf(g_fDebug, "scheduler not changed\n");
    fflush(g_fDebug);
    }
    else
    {
    fprintf(g_fDebug, " ok priority_max = %d\n", Param.sched_priority);
    fprintf(g_fDebug, "  call to sched_setscheduler...");
    fflush(g_fDebug);
    nRes = sched_setscheduler(0, SCHED_FIFO, &Param);
    if (nRes) 
    {
    fprintf(g_fDebug," failed\n");
    fprintf(g_fDebug,"%s\n", strerror(errno));
    fprintf(g_fDebug, "scheduler not changed\n");
    fflush(g_fDebug);
    }
    else
    {
    fprintf(g_fDebug," ok policy set to SCHED_FIFO\n");
    fprintf(g_fDebug, "scheduler changed\n");
    fflush(g_fDebug);
    }
    }
    fprintf(g_fDebug, "\n"); 
    fflush(g_fDebug);
  */
  
  // init CRC table
  initCrcTable(g_dwCrcTable);

  if (!options.szFullyBatchMode || !strlen(options.szFullyBatchMode)) {
  // signal
  /*struct sigaction saOld, saNew;
    saNew.sa_handler = catch_sigint;
    sigemptyset(&saNew.sa_mask);
    saNew.sa_flags = 0;
    sigaction(SIGKILL, &saNew, &saOld);
    sigaction(SIGINT, &saNew, &saOld);*/
  signal(SIGTERM, catch_sigint);
  signal(SIGINT, catch_sigint);
//  signal(SIGSEGV, catch_sigint); // segmentation fault
  }
   
  // endianess
  switch(g_wEndian)
    {
    case ENDIAN_UNKNOWN:
      showDebug(1, "ENDIANESS=ENDIAN_UNKNOWN\n"); break;
      
    case ENDIAN_LITTLE:
      showDebug(1, "ENDIANESS=ENDIAN_LITTLE\n"); break;
      
    case ENDIAN_BIG:
      showDebug(1, "ENDIANESS=ENDIAN_BIG\n"); break;

    case ENDIAN_PDP:
      showDebug(1, "ENDIANESS=ENDIAN_PDP\n"); break;
    }

  // init interface
  showDebug(8, "initialize interface\n");
  if (!options.szFullyBatchMode || !strlen(options.szFullyBatchMode))
    g_interface = new PARTIMAGE_INTERFACE(options.bBatchMode);
  else
    g_interface = new CInterfaceNone(options.szFullyBatchMode);
  
  showDebug(8, "interface ok\n");

  // ----------- check the user is logged as root
  if (geteuid() != 0) // 0 is the UID of the root
    {
      if (g_interface -> ErrorLogAsRoot() == MSGBOX_CANCEL)
	{
	  delete g_interface; 
	  return EXIT_FAILURE;
	}
    }		
  showDebug(8, "ok for uid\n");

  // check options
  /*nRes = checkOptions(options);
    if (nRes == -1)
    {
    delete g_interface; 
    return EXIT_FAILURE;
    }
    showDebug(8, "ok for options\n");
  */
  
  nChoice = -1;

  memset(szDevice, 0, MAX_DEVICENAMELEN);
  memset(szImageFile, 0, MAXPATHLEN);

  // run operations if the command line is full
  if (argc - optind == 3) // commands with 2 parameters
    {
      showDebug(8, "full cmdline with 2 param\n"); 
      strncpy(szDevice, argv[optind+1], MAX_DEVICENAMELEN);
      strncpy(szImageFile, argv[optind+2], MAXPATHLEN);
      
      if (strcmp(argv[optind], "save")==0) // save
	nChoice = OPERATION_SAVE;
      else if (strcmp(argv[optind], "restore")==0) // restore
	nChoice = OPERATION_RESTORE;
    }

  if (argc - optind == 2) // commands with 1 parameter
    {
      showDebug(8, "full cmdline with 1 param\n"); 
      strncpy(szImageFile, argv[optind+1], MAXPATHLEN);
      
      if (strcmp(argv[optind], "restmbr")==0) // restore an MBR
	nChoice = OPERATION_RESTMBR;
      else if (strcmp(argv[optind], "imginfo")==0)
	// show informations about the imagefile
	nChoice = OPERATION_IMGINFO;
    }
  
  // check options
  nRes = checkOptions(options, szDevice, szImageFile);
  if (nRes == -1)
    {
      delete g_interface; 
      return EXIT_FAILURE;
    }
  showDebug(8, "ok for options\n");

  if ((isDevfsEnabled()) && (!isDevfsMounted()))
  {
    g_interface->msgBoxCancel(i18n("Warning"),
       i18n("You have devfs enabled but not mounted. You have to do it to "
            "continue. You can also boot with non-devfs kernel if you have "
	    "one."));
    delete g_interface;
    return EXIT_FAILURE;
  }
  
 beginMainWin:

  // if no command line or invalid action
  if (nChoice == -1)
    {
      showDebug(8, "weird cmdline\n"); 
      strcpy(szDevice, "/dev/");
      *szImageFile = 0;		
      
      // draw main window
      showDebug(8, "weird cmdline\n"); 
      nChoice = g_interface -> guiInitMainWindow(szDevice, szImageFile, 
						 options.szServerName, &(options.dwServerPort), &options.bUseSSL);
    }
  
  showDebug(8, "go on\n");
  // run action
  switch(nChoice)
    {
    case OPERATION_SAVE:
      showDebug(1, "action=SAVE\n");
      detectFileSystem(szDevice, szFileSystem);
//#ifndef DEVEL_SUPPORT
      if (isFileSystemSupported(szFileSystem) == true)
//#endif // DEVEL_SUPPORT
	{
	  try { savePartition(szDevice, szImageFile, &options); }
	  catch (CExceptions *excep)
	    {
	      if (excep->GetExcept() == ERR_COMEBACK)
		{
		  nChoice = -1;
		  goto beginMainWin;
		}

	      if (!options.bBatchMode)
		g_interface -> Error(excep, szImageFile, szDevice);

	      showDebug(1, "\nFINAL ERROR\n\n");
	  
	      nRes = -1;
	    }
	}
//#ifndef DEVEL_SUPPORT
      else
	{
	  showDebug(1, "The file system of [%s] is [%s], and is not supported\n", szDevice, szFileSystem);
	  if (!options.bBatchMode)
	    g_interface->msgBoxError("The file system of [%s] is [%s], and is not supported", szDevice, szFileSystem);
	  nRes = -1;
	}
//#endif // DEVEL_SUPPORT
      break;
      
    case OPERATION_RESTORE:
      showDebug(1, "action=RESTORE\n");
      try { restorePartition(szDevice, szImageFile, &options); }
      catch (CExceptions *excep)
	{
	  if (excep->GetExcept() == ERR_COMEBACK)
	    {
	      nChoice = -1;
	      goto beginMainWin;
	    }

	  if (!options.bBatchMode && !excep->getCaught())
	    g_interface -> Error(excep, szImageFile);

	  showDebug(1, "\nFINAL ERROR\n\n");

	  nRes = -1;
	}
      break;

    case OPERATION_RESTMBR:
      showDebug(1, "action=RESTMBR\n");
      try { restoreMbr(szImageFile, &options); }
      catch (CExceptions *excep)
	{
	  if (excep->GetExcept() == ERR_COMEBACK)
	    {
	      nChoice = -1;
	      goto beginMainWin;
	    }

          showDebug(1, "restoreMBR caught exception: %d\n", excep->GetExcept());

	  if (!options.bBatchMode && !excep->getCaught())
	    g_interface -> Error(excep, szImageFile);

	  showDebug(1, "\nFINAL ERROR\n\n");

	  nRes = -1;
	}
      break;
      
    case OPERATION_IMGINFO:
      showDebug(1, "action=IMGINFO\n");
      try { showImgInfos(szImageFile, &options); }
      catch (CExceptions *excep)
	{
          showDebug(1, "showimageinfo caught exception: %d\n", excep->GetExcept());

	  if (!options.bBatchMode && !excep->getCaught())
	    g_interface -> Error(excep, szImageFile);

	  showDebug(1, "\nFINAL ERROR\n\n");

	  nRes = -1;
	}
      break;
      
    default: // exit
      if (options.bSync)
        {
          g_interface -> StatusLine(i18n("commiting buffer cache to disk."));
	  sync();
        }
      closeDebugFiles();
      delete g_interface; 
      return EXIT_SUCCESS;
      break;
    }
 
  showDebug(8, "sync\n"); 
  // update the disk IO
  if (options.bSync)
    {
      g_interface -> StatusLine(i18n("commiting buffer cache to disk."));
      sync();
    }
  
  // ---- statistics about time/CPU infos ----
  rusage rusageSelf, rusageChild;
  time_t timeCopy;
  DWORD dwTimeTotal, dwTimeUser, dwTimeSys;
  
  g_timeEnd = time(0);
  timeCopy = g_timeEnd - g_timeStart; 
  
  if ((getrusage(RUSAGE_SELF, &rusageSelf) != -1) &&
      (getrusage(RUSAGE_CHILDREN, &rusageChild) != -1))
    {
      dwTimeUser = rusageSelf.ru_utime.tv_sec + rusageChild.ru_utime.tv_sec +
	g_dwTimeThreadUser;
      dwTimeSys = rusageSelf.ru_stime.tv_sec + rusageChild.ru_stime.tv_sec +
	g_dwTimeThreadSys;
      dwTimeTotal = dwTimeUser + dwTimeSys;
      
      showDebug(1, "\n\n============= TIME and CPU infos ================\n");
      showDebug(1, "Total time:...........%s\n", formatTime(timeCopy, szTemp));
      showDebug(1, "User time:............%s (main=%s, child=%s)\n", 
		formatTime(dwTimeUser, szTemp), formatTime(rusageSelf.ru_utime.tv_sec, 
							   szTemp2), formatTime(rusageChild.ru_utime.tv_sec, szTemp3));
      showDebug(1, "System time:..........%s (main=%s, child=%s)\n", 
		formatTime(dwTimeSys, szTemp), formatTime(rusageSelf.ru_stime.tv_sec,
							  szTemp2), formatTime(rusageChild.ru_stime.tv_sec, szTemp3));
      showDebug(1, "CPU used:.............%d %%\n", (timeCopy != 0) ? ((dwTimeTotal * 100) / timeCopy) : 0);
      showDebug(1, "Beginning:............%15ld = %s", g_timeBegin, ctime(&g_timeBegin));
      showDebug(1, "Start copy:...........%15ld = %s", g_timeStart, ctime(&g_timeStart));
      showDebug(1, "End:..................%15ld = %s", g_timeEnd, ctime(&g_timeEnd));
      showDebug(1, "\n============= TIME and CPU infos ================\n\n\n");
    }
  
  // show result message
  if (nRes == -1) // FAILED
    {
      showDebug(1, "End of operation: FAILED\n");
      closeDebugFiles();
      delete g_interface; 
      return EXIT_FAILURE;
    }
  else // SUCCESS
    {	
      showDebug(1, "End of operation: SUCCESS\n");
//      closeDebugFiles();
//      pthread_mutex_destroy(&g_mutexDebug);
      
      if (options.dwFinish == FINISH_WAIT)
	{
	  showDebug(1, "option FINISH_WAIT\n");
	  if (nChoice == OPERATION_RESTMBR)
	    {
	      SNPRINTF(szTemp, i18n("The MBR was successfully restored"));
	    }
	  else
	    {
	      if (options.bSimulateMode)
		formatOperationSucessMsg(szTemp, sizeof(szTemp), i18n("Simulation"));
	      else
		formatOperationSucessMsg(szTemp, sizeof(szTemp), i18n("Operation"));
	    }
   
	  if (nChoice != OPERATION_IMGINFO)
	    g_interface->msgBoxOk(i18n("Success"), szTemp);
	  delete g_interface; 
	  return EXIT_SUCCESS;
	}
      else if (options.dwFinish == FINISH_HALT)
	{
	  showDebug(1, "option FINISH_HALT\n");
	  delete g_interface; 
	  nRes = system("/sbin/shutdown -h now");
          nRes = system("/sbin/poweroff");
// if we reach this point, it's because shutdown failed
	  fprintf(stderr, i18n("Error: Cannot halt the computer"));
	  return EXIT_SUCCESS;
	}
      else if (options.dwFinish == FINISH_REBOOT)
	{
	  showDebug(1, "option FINISH_REBOOT\n");
	  delete g_interface; 
	  nRes = system("/sbin/shutdown -r now");
          nRes = system("/sbin/reboot");
// if we reach this point, it's because shutdown failed
	  fprintf(stderr, i18n("Error: Cannot reboot the computer"));
	  return EXIT_SUCCESS;
	}
      else if (options.dwFinish == FINISH_QUIT)
	{
	  showDebug(1, "option FINISH_QUIT\n");
	  delete g_interface; 
	  return EXIT_SUCCESS;
	}
      else if (options.dwFinish == FINISH_LAST)
	{
	  showDebug(1, "option FINISH_LAST\n");
	  int nLockFile;
	  FILE * nCountFile;
	  char szValue[11];
	  int nValue, nRetries;
	  ssize_t nRes;

	  nRetries = 5;
          do {
	    nLockFile = open(FINISH_LAST_COUNTFILE_LOCK, O_CREAT|O_EXCL,
               O_RDONLY);
	    if (nLockFile == -1)
	      {
                --nRetries;
		sleep(2);
	      }
	  } while (nLockFile == -1 && nRetries);
	  if (!nRetries)
	    {
    	      delete g_interface; 
	      fprintf(stderr, i18n("Error: Cannot count remaining partimages"));
	      return EXIT_SUCCESS;
	    }
          nCountFile = fopen(FINISH_LAST_COUNTFILE, "r");
	  if (nCountFile == NULL)
	    {
    	      delete g_interface; 
	      fprintf(stderr, i18n("Error: Cannot count remaining partimages"));
	      return EXIT_SUCCESS;
	    }
	  fscanf(nCountFile, "%d\n", &nValue);
	  showDebug(1, "RESTANT: %d\n", nValue);
	  if (nValue <= 1)
	    { // we are the last running partimage -> shutdown
	      delete g_interface; 
	      fclose(nCountFile);
	      close(nLockFile);
	      unlink(FINISH_LAST_COUNTFILE_LOCK);
	      unlink(FINISH_LAST_COUNTFILE);
//	      nRes = system("/sbin/shutdown -r now");
//              nRes = system("/sbin/reboot");
// if we reach this point, it's because shutdown failed
	      fprintf(stderr, i18n("DEBUG: reboot the computer"));
	      return EXIT_SUCCESS;
	    }
	  else
	    {
              --nValue;
	      fclose(nCountFile);
              nCountFile = fopen(FINISH_LAST_COUNTFILE, "w");
	      if (nCountFile == NULL)
	        {
    	          delete g_interface; 
	          fprintf(stderr, i18n("Error: Cannot count remaining "
                     "partimages"));
	          return EXIT_SUCCESS;
	        }
	      fprintf(nCountFile, "%d\n", nValue);
	      fclose(nCountFile);
	      close(nLockFile);
	      unlink(FINISH_LAST_COUNTFILE_LOCK);
    	      delete g_interface; 
	      return EXIT_SUCCESS;
	    }
        }
    }
}

// =======================================================
void closeDebugFiles()
{
  if (g_dwDebugLevel && (g_fDebug != stderr))
    fclose(g_fDebug);

  // local debug
}

// =======================================================
void usage()
{
  printf("===============================================================================\n");
  printf(i18n("Partition Image (http://www.partimage.org/) version %s [%s]\n"
	      "---- distributed under the GPL 2 license (GNU General Public License) ----\n\n"
	      "Supported file systems: Ext2/3, Reiser3, FAT16/32, HPFS, JFS, XFS, \n"
	      "                        UFS(beta), HFS(beta), NTFS(experimental)\n\n"
	      "usage: partimage [options] <action> <device> <image_file>\n"
	      "       partimage <imginfo/restmbr> <image_file>\n\n"
	      "ex: partimage -z1 -o -d save /dev/hda12 /mnt/backup/redhat-6.2.partimg.gz\n"
	      "ex: partimage restore /dev/hda13 /mnt/backup/suse-6.4.partimg\n"
	      "ex: partimage restmbr /mnt/backup/debian-potato-2.2.partimg.bz2\n"
	      "ex: partimage -z1 -om save /dev/hda9 /mnt/backup/win95-osr2.partimg.gz\n"
	      "ex: partimage imginfo /mnt/backup/debian-potato-2.2.partimg.bz2\n"
	      "ex: partimage -a/dev/hda6#/mnt/partimg#vfat -V1440 save /dev/hda12 /mnt/partimg/redhat-6.2.partimg.gz\n\n"
	      "Arguments:\n"
	      "* <action>:\n"
	      "  - save: save the partition datas in an image file\n"
	      "  - restore: restore the partition from an image file\n"
	      "  - restmbr: restore a MBR of the image file to an hard disk\n"
	      "  - imginfo: show informations about the image file\n"
	      "* <device>: partition to save/restore (example: /dev/hda1)\n"
	      "* <image_file>: file where data will be read/written. Can be very big.\n"
	      "                For restore, <image_file> can have the value 'stdin'. This allows\n"
              "                for providing image files through a pipe.\n\n"
	      "Options:\n"
	      "* -z,  --compress      (image file compression level):\n"
	      "  -z0, --compress=0    don't compress: very fast but very big image file\n"
	      "  -z1, --compress=1    compress using gzip: fast and small image file (default)\n"
	      "  -z2, --compress=2    (compress using bzip2: very slow and very small image file):\n"
	      "* -c,  --nocheck       don't check the partition before saving\n"
	      "* -o,  --overwrite     overwrite the existing image file without confirmation\n"
	      "* -d,  --nodesc        don't ask any description for the image file\n"
	      "* -V,  --volume        (split image into multiple volumes files)\n"
	      "  -VX, --volume=X      create volumes with a size of X MB\n"
	      "* -w,  --waitvol       wait for a confirmation after each volume change\n"
	      "* -e,  --erase         erase empty blocks on restore with zero bytes\n"
	      "* -m,  --allowmnt      don't fail if the partition is mounted. Dangerous !\n"
	      "* -M,  --nombr         don't create a backup of the MBR (Mast Boot Record) in the image file\n"
	      "* -h,  --help          show help\n"
	      "* -v,  --version       show version\n"
	      "* -i,  --compilinfo    show compilation options used\n"
	      "* -f,  --finish        (action to do if finished successfully):\n"
	      "  -f0, --finish=0      wait: don't make anything\n"
	      "  -f1, --finish=1      halt (power off) the computer\n"
	      "  -f2, --finish=2      reboot (restart the computer):\n"
	      "  -f3, --finish=3      quit\n"
	      "* -b,  --batch         batch mode: the GUI won't wait for an user action\n"
              "* -BX, --fully-batch=X batch mode without GUI, X is a challenge response string\n"
	      "* -y,  --nosync        don't synchronize the disks at the end of the operation (dangerous)\n"
	      "* -sX, --server=X      give partimaged server's ip address\n"
	      "* -pX, --port=X        give partimaged server's listening port\n"
	      "* -g,  --debug=X       set the debug level to X (default: 1):\n"
	      "* -n,  --nossl         disable SSL in network mode\n"
	      "* -S,  --simulate      simulation of restoration mode\n"
	      "* -aX, --automnt=X     automatic mount with X options. Read the doc for more details\n"
	      "* -UX  --username=X    username to authenticate to server\n"
	      "* -PX  --password=X    password for authentication of user to server\n"),
	 PACKAGE_VERSION, isVersionStable(PACKAGE_VERSION) ? i18n("stable") : i18n("unstable")); 
  printf("===============================================================================\n");
}

// =======================================================
int checkStructSizes()
{
   // Disable header check for AMD64, because it fails 
#if defined(__x86_64__)
    return 0;
#endif

   // ---- check types sizes

   if (sizeof(unsigned long int) != 4)
   {
      fprintf (stderr, "Error: sizeof(DWORD) != 4 (%d)\n", sizeof(unsigned long int));
      goto errcheck;
   }

   if (sizeof(unsigned long long int) != 8)
   {
      fprintf (stderr, "Error: sizeof(QWORD) != 8 (%d)\n", sizeof(unsigned long long int));
      goto errcheck;
   }

   // ---- check struct sizes

   // check main header size is 16384
   if (sizeof(CMainHeader) != 16384)
   {
      fprintf (stderr, "Error: main header size != 16384 (%d)\n",
	       sizeof(CMainHeader));
      goto errcheck;
   }
  
   // check ext2 header size is 16384
   if (sizeof(CInfoExt2Header) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: ext2 header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoExt2Header));
      goto errcheck;
   }
  
   // check reiserfs header size is 16384
   if (sizeof(CInfoReiserHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: reiserfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoReiserHeader));
      goto errcheck;
   }
  
   // check fat header size is 16384
   if (sizeof(CInfoFatHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: fat header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoFatHeader));
      goto errcheck;
   }
  
   // check ntfs header size is 16384
   if (sizeof(CInfoNtfsHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: ntfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoNtfsHeader));
      goto errcheck;
   }
  
   // check hpfs header size is 16384
   if (sizeof(CInfoHpfsHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: hpfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoHpfsHeader));
      goto errcheck;
   }
  
   // check jfs header size is 16384
   if (sizeof(CInfoJfsHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: jfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoJfsHeader));
      goto errcheck;
   }
  
   // check xfs header size is 16384
   if (sizeof(CInfoXfsHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: xfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoXfsHeader));
      goto errcheck;
   }
 
   // check hfs header size is 16384
   if (sizeof(CInfoHfsHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: hfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoHfsHeader));
      goto errcheck;
   }
 
   // check ufs header size is 16384
   if (sizeof(CInfoUfsHeader) != INFOS_STRUCT_SIZE)
   {	
      fprintf (stderr, "Error: jfs header size != %d (%d)\n", INFOS_STRUCT_SIZE,
	       sizeof(CInfoUfsHeader));
      goto errcheck;
   }
 
   // check tail size is 16384
   if (sizeof(CMainTail) != 16384)
   {	
      fprintf (stderr, "Error: main footer size != 16384 (%d)\n", sizeof(CMainTail));
      goto errcheck;
   }
  
   // check volume header size is 512
   if (sizeof(CVolumeHeader) != 512)
   {	
      fprintf (stderr, "Error: volume hedaer size != 512 (%d)\n", sizeof(CVolumeHeader));
      goto errcheck;
   }
 
   // check MBR size is 2048
   if (sizeof(CMbr) != 2048)
   {	
      fprintf (stderr, "Error: MBR size != 2048 (%d)\n", sizeof(CMbr));
      goto errcheck;
   }
 
   // check volume header size is 512
   if (sizeof(CLocalHeader) != 16384)
   {	
      fprintf (stderr, "Error: local header size != 16384 (%d)\n", sizeof(CLocalHeader));
      goto errcheck;
   }

   return 0; // success

  errcheck:
   printf ("This version has been compiled with an uncompatible version of gcc.\n");
   //printf("DetailSize: sizeof(DWORD)=%d\n", sizeof(DWORD));
   //printf("DetailSize: sizeof(QWORD)=%d\n", sizeof(QWORD));
   //printf("DetailSize: sizeof(struct tm)=%d\n", sizeof(struct tm));
   RETURN_int(-1); // error
}

// =======================================================
void formatOperationSucessMsg(char *szMsg, int nMaxLen, char *szOperation)
{
  char szTemp1[2048];
  char szTemp2[2048];
  char szTemp3[2048];

  time_t timeElapsed;
  QWORD qwBytesPerMin;
  float fMinElapsed;
  
  timeElapsed = time(0) - g_timeStart;
  fMinElapsed = ((float)timeElapsed) / 60.0;
  qwBytesPerMin = (QWORD) (((float)g_qwCopiedBytesCount) / fMinElapsed);
  
  snprintf(szMsg, nMaxLen, i18n("%s  successfully finished:\n\n"
				"Time elapsed:...%s\n"
				"Speed:..........%s/min\n"
				"Data copied:....%s"),
	   szOperation,
	   formatTime((DWORD)timeElapsed, szTemp1),
	   formatSize(qwBytesPerMin, szTemp2),
	   formatSize(g_qwCopiedBytesCount, szTemp3));
}

