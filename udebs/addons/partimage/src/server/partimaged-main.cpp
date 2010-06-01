/***************************************************************************
                          partimaged-main.cpp  -  main part of Partimaged
                             -------------------
    begin                : Sun Jan 28 2001
    copyright            : (C) 2001 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/
// $Name:  $
// $Revision: 1.27 $
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

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "net.h"
#include "netserver.h"
#include "messages.h"
#include "image_disk.h"
#include "common.h"
#include "pathnames.h"
#include "buffer.h"
#include "partimaged-gui_newt.h"
#include "partimaged-gui_dummy.h"
#include "partimaged.h"
#include "access.h"
#include "privs.h"

static char Revision[] = "CVS $Revision: 1.27 $";

CPrivs * g_privs=NULL;
FILE * g_fDebug;
FILE * g_fLocalDebug;
extern DWORD g_dwDebugLevel;
bool g_bBeDaemon;
CNetServer * g_Server;
CPartimagedInterface * g_Window;
bool g_bMustLogin;

extern char * optarg;
extern int optind;
extern int opterr;


#ifdef HAVE_GETOPT_H
static struct option const long_options[] =
{
  {"compilinfo", no_argument, NULL, 'i'},
  {"port", required_argument, NULL, 'p'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"daemon", no_argument, NULL, 'D'},
  {"dest", required_argument, NULL, 'd'},
  {"chroot", required_argument, NULL, 'r'},
  {"debug", required_argument, NULL, 'g'},
  {"nologin", no_argument, NULL, 'L'},
  {NULL, 0, NULL, 0}
};
#endif //HAVE_GETOPT_H

static char optstring[]="ip:hvDd:r:g:L"; 

bool g_bSigKill = false;
bool g_bSigInt = false;

// =======================================================
static void catch_sigint(int signo)
{
  if (signo == SIGTERM)
    {
      g_bSigKill = true;
      delete g_Server;
      delete g_Window;
      g_Server = NULL;
      g_Window = NULL;
      exit(0);
    }
  else if (signo == SIGINT)
    {
      g_bSigInt = true;
      delete g_Server;
      delete g_Window;
      g_Server = NULL;
      g_Window = NULL;
      exit(0);
    }
  else if (signo == SIGSEGV)
    {
      delete g_Server;
      delete g_Window;
      g_Server = NULL;
      g_Window = NULL;
      fprintf(stderr, i18n("Segmentation fault. Please report the bug and send the [%s] file to authors\n"), PARTIMAGED_LOG);
      exit(0);      
    }
  showDebug(1, "Signal: %d\n", signo);
}

// =======================================================
void Usage()
{
  printf ("======================================================="
	  "========================\n");
  printf (i18n("Partition Image Daemon (http://www.partimage.org/) version %s\n"
	       "---- distributed under the GPL 2 license (GNU General Public License) ----\n\n"
	       "usage: partimaged [options]\n\n"
	       "Options:\n"
	       "* -h,  --help            show help\n"
	       "* -v,  --version         show version\n"
	       "* -d,  --dest            destination directory\n"
	       "* -pX, --port=X          server's port listening (defaults to %d)\n"
	       "* -D,  --daemon          start in daemon mode, not foreground\n"
	       "* -i,  --compilinfo      show compilation options used\n"
	       "* -r dir, --chroot dir   use chroot to improve security\n"
	       "* -g, --debug=X          set the debug level to X (default: 1)\n"
               "* -L, --nologin          disable login from clients\n"),
	  PACKAGE_VERSION, SERVER_LISTEN_PORT);
  printf ("======================================================="
	  "========================\n");
  exit(0);
}

// =======================================================
void Initialize()
{
  char szTemp[2048];

  // open log files as root
  if (g_dwDebugLevel)
    {
      g_fDebug = openFileDescriptorSecure(PARTIMAGED_LOG, "a", O_WRONLY | 
         O_CREAT | O_NOFOLLOW | O_TRUNC, S_IRUSR | S_IWUSR);
      if (!g_fDebug)
        {
          fprintf(stderr, i18n("Impossible to open logfile %s: %s\n"), 
             PARTIMAGED_LOG, strerror(errno));
          fprintf(stderr, i18n("Fix this and retry.\n"));
        }
    }
  else
    g_fDebug = NULL;

  if (!g_fDebug)
    {
      g_fDebug = fopen("/dev/null", "w");
      if (!g_fDebug)
	exit(1);
    }

  g_fLocalDebug = fopen("/dev/null", "w");
  if (!g_fLocalDebug)
    g_fDebug = stderr;

  // find PARTIMAGED_USER euid and register it to be used later.
  g_privs = new CPrivs(PARTIMAGED_USER);
  if (!g_privs)
    {
      fprintf(stderr, i18n("impossible to find user \"%s\"\n"),PARTIMAGED_USER);
      fprintf(stderr, i18n("add it and retry\n"));
    }
  if (!g_privs->AsSwitched())
    showDebug(4, i18n("Warning: we couldn't changed euid because we weren't "
       "launched as root\nexpect some access denied or troubles like this.\n"));

  // add usefull informations into logfile
  showDebug(1, "partimaged %s(%s)\n", PACKAGE_VERSION, Revision);
  showDebug(1,"-----------------------------------------\n");

  formatCompilOptions(szTemp, sizeof(szTemp));
  showDebug(1, "%s\n\n", szTemp);
  showDebug(1,"-----------------------------------------\n");

  // we only exit here because we must write informations in logfile to help
  // debugging
  if (!g_privs)
    exit(0);  
}
  
// =======================================================
int main(int argc, char *argv[])
{
  unsigned int client;
  int nOptCh, nRes, nServerPort = SERVER_LISTEN_PORT;
  char * szRootDir = NULL;
  char * szPeerName;
  char szTemp[2048];

  g_dwDebugLevel = DEFAULT_DEBUG_LEVEL;
  g_nDebugThreadMain = getpid();
  g_bBeDaemon = false;
  g_bMustLogin = true;

  pthread_t threads[MAX_CLIENTS];

#ifdef HAVE_GETOPT_H
  while ((nOptCh = getopt_long(argc, argv, optstring, long_options,NULL))
     != EOF)
#else
  while ((nOptCh = getopt(argc, argv, optstring)) != -1)
#endif
    {
      switch (nOptCh)
        {
          case 'p':  // listening port  
            nServerPort = atoi(optarg);
            break;
          case 'h':  // help
            Usage();
          case 'v':  // version
#ifdef HAVE_SSL
            printf(i18n("Partition Image Daemon version %s+SSL\n"), PACKAGE_VERSION);
#else
            printf(i18n("Partition Image Daemon version %s\n"), PACKAGE_VERSION);
#endif
            printf(i18n("(distributed under the GNU GPL 2)\n"));
            exit(0);
          case 'D':  // daemonize
            g_bBeDaemon = true;
            break;
	  case 'd':
	    if (chdir(optarg) != 0)
	    {
		    printf("Directory %s: %s\n", optarg, strerror(errno));
		    exit(-1);
	    }
	    break;
          case 'r':  // change chroot
            if ((optarg) && (optarg[0]))
              szRootDir = strdup(optarg); 
            break;
          case 'g':  // debug level
            g_dwDebugLevel = atol(optarg);
            if (g_dwDebugLevel < 0 || g_dwDebugLevel > 10)
              g_dwDebugLevel = DEFAULT_DEBUG_LEVEL;
            break;
	  case 'i':  // compilation options
	    formatCompilOptions(szTemp, sizeof(szTemp));
	    printf("%s\n", szTemp);
	    return EXIT_SUCCESS;
	    break;

          case 'L':  // no login
            g_bMustLogin = false;
          default:
            break;
        }
    }
  if (szRootDir)
    {
//      showDebug(2, "about to chroot to %s\n", szRootDir);
//      g_privs->Root();
      if (chdir(szRootDir) < 0)
        {
 //         g_privs->User();
          fprintf(g_fDebug, "failed to chdir to %s: %s\n", szRootDir,
             strerror(errno));
          delete g_Window;
	  g_Window = NULL;
          exit(1);
        }
      if (chroot(szRootDir) < 0)
        {
//          g_privs->User();
          fprintf(g_fDebug, "failed to chroot: %s\n", strerror(errno));
          delete g_Window;
	  g_Window = NULL;
          exit(1);
        }
      if (chdir("/") < 0)
        {
  //        g_privs->User();
          fprintf(g_fDebug, "failed to chdir to /: %s\n", strerror(errno));
          delete g_Window;
	  g_Window = NULL;
          exit(1);
        }
//      g_privs->User();
//      showDebug(1, "root changed to %s\n", szRootDir);
      free(szRootDir);
      szRootDir = NULL;
    }

  // Warn: no showDebug must have been called before this point
  Initialize();

#ifdef MUST_LOGIN
  if (g_bMustLogin)
    { 
      if (CheckAccessFile(PARTIMAGED_USERS))
        exit(1);   
    }
#endif
#ifdef HAVE_SSL
  if ( CheckAccessFile(KEYF) || CheckAccessFile(CERTF) )
    {
      exit(1);   
    }
#endif    

  // register signals
  signal(SIGTERM, catch_sigint);
  signal(SIGINT, catch_sigint);
  signal(SIGSEGV, catch_sigint); // segmentation fault
//  signal(SIGHUP, catch_sigint);
//  signal(SIGQUIT, catch_sigint);
//  signal(SIGCHLD, catch_sigint);
//  signal(SIGILL, catch_sigint);
//  signal(SIGKILL, catch_sigint);

  if (g_bBeDaemon)
    {
      // fork - so i'm not the owner of the process group any more
      int pgrp;
      int i = fork();
      if (i < 0)
        {
          showDebug(1, "can't fork: %s\n", strerror(errno));
          exit(1);
        }
      if (i > 0)
        exit(0); // no need for the parent any more 

      pgrp = setsid();
      if (pgrp < 0)
        {
          showDebug(1, "can't daemonize: %s\n", strerror(errno));
          exit(1);
        }
/* FIXME: this will break 'socket' call
  close(fileno(stdin));
  close(fileno(stdout));
  close(fileno(stderr));
*/ 

#ifdef HAVE_SETPGID
  setpgid(0, getpid());
#else
# ifdef SETPGRP_VOID
  setpgrp();
# else
  setpgrp(0, getpid());
# endif
#endif
    }
      
  
  try { g_Server = new CNetServer(nServerPort); }
  catch ( CExceptions * excep )
    {
      showDebug(1, "fatal error: get exception %d\n", excep->GetExcept());
      fclose(g_fDebug);
      exit(1);
    }

//  g_Window = new CPartimagedInterfaceDummy(); 
  g_Window = new CPartimagedInterfaceNewt(); 

  g_Window->Status(i18n("Waiting for client ..."));

  while (1)
    {  
      showDebug(1, "infernal loop\n");
      try { client = g_Server->AcceptClient(); }
      catch ( CExceptions * excep )
        {
          showDebug(1, "*** excep catched\n");
          switch (excep -> GetExcept())
          {
            case ERR_ERRNO:
              showDebug(1, "accept failed with %s\n", 
                 strerror(excep->get_dwArg1()));
              continue; // jump to next client

            case ERR_TOOMANY:
              showDebug(3, "too many clients -> one refused\n"); 
              if (!g_bBeDaemon)
                g_Window->Status(i18n("too much client connected")); 
              else
                fprintf(stderr, "partimaged: too much clients connected\n");
              continue; // jump to next client

            case ERR_REFUSED:
              showDebug(1, "refused: banner or version\n");
// client is undefined->we can't show it
//              g_Window->SetState(client, "error !");
//              g_Window->SetLocation(client, "(wrong banner)");
              continue; // jump to next client

            default:
              showDebug(1, "other exception: %d\n", excep->GetExcept());
              continue; // jump to next client
          }
          showDebug(1, "ARG: reached unexpected point\n");
          continue; // we shouldn't reach this point but never knows
        }
      showDebug(1, "client %d arrived\n", client);
      g_Window->SetState(client, "connected");
      szPeerName = g_Server -> GetPeer(client);
      g_Window->SetHostname(client, szPeerName);
      free(szPeerName);
        
      nRes = g_Server->ValidatePass(client);
      if (nRes)
        {
          g_Window->SetState(client, "error !");
          g_Window->SetLocation(client, "(wrong password)");
        }
      else
        pthread_create(&threads[client], NULL,
           partimaged, &client);

    } // infernal loop

  showDebug(1, "end of partimaged-main\n");
  while(1);
  delete g_Server;
  g_Server = NULL;
  delete g_Window;
  g_Window = NULL;
  fclose(g_fDebug);
}

