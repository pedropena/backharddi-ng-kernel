/***************************************************************************
                          access.cpp  -  allow or not connexion from user
                             -------------------
    begin                : Tue Feb 20 2001
    copyright            : (C) 2000 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/
// $Revision: 1.24 $
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// PAM support is writen based on code from Chris Evans for
// Very Secure FTPd.

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

// shadow.h and crypt.h may not exist on bsd
#ifdef HAVE_PAM
  #include <security/pam_appl.h>
#else
  #ifdef HAVE_SHADOW_H
    #include <shadow.h>
  #endif
  #ifdef HAVE_CRYPT_H
    #include <crypt.h>
  #endif
#endif

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>

#include "common.h"
#include "pathnames.h"
#include "privs.h"

extern CPrivs * g_privs;

#ifndef HAVE_PAM
char * GetSalt(char * szLogin)
{
#ifdef OS_LINUX
  struct spwd * s_pass;
#endif
#ifdef OS_FBSD
  struct passwd * s_pass;
#endif
  char * salt = (char *) malloc(12); 
  long int r;
  BEGIN;

  if (!salt)
    return NULL;

  srandom((unsigned int) time(NULL));
  *(salt+11) = '\0';
  for (unsigned int i = 0; i < 11; i++)
    {
      r = random() % 52;
      *(salt+i) = (r>25 ? 'a'+r-26 : 'A'+r);
    }

#ifdef MUST_LOGIN
    g_privs->Root(); 
  #ifdef OS_LINUX
    s_pass = getspnam(szLogin);
  #endif
  #ifdef OS_FBSD
    s_pass = getpwnam(szLogin);
  #endif
    g_privs->User(); 

  #ifdef OS_LINUX
    if (s_pass && strlen(s_pass->sp_pwdp) > 10)
      memcpy(salt, s_pass->sp_pwdp, 11);
    else if (s_pass)
      memcpy(salt, s_pass->sp_pwdp, strlen(s_pass->sp_pwdp));
  #endif
  #ifdef OS_FBSD
    if (s_pass && strlen(s_pass->pw_passwd) > 10)
      memcpy(salt, s_pass->pw_passwd, 11);
    else if (s_pass)
      memcpy(salt, s_pass->pw_passwd, strlen(s_pass->pw_passwd));
  #endif
#else  // MUST_LOGIN
  *salt = '\0';
#endif // MUST_LOGIN

  showDebug(9, "end of access::getsalt\n");
  return salt;
}
#else // HAVE_PAM
char * GetSalt(char * szLogin)
{
  return NULL;
}
#endif

#ifdef MUST_LOGIN  
#ifndef HAVE_PAM
unsigned int CheckAccess(bool bMustLogin, char * szLogin, char * szPasswd)
{
  if (!bMustLogin)
    RETURN_int(0)

  FILE * f;
#ifdef OS_LINUX
  struct spwd * s_pass;
#endif
#ifdef OS_FBSD
  struct passwd * s_pass;
#endif
  int found;
  char * str = (char *) malloc (1024);
  char * ptr, * ptr2;
  BEGIN;

  f = fopen(PARTIMAGED_USERS, "r");
  if (!f) 
    {
    showDebug(1, "can't open: %d %s\n", errno, strerror(errno));
    RETURN_int(1);
    }

  found = 0;
  while (!feof(f))
    {
      fgets(str, 1023, f);
      while (str[strlen(str)-1] == '\n')
        str[strlen(str)-1] = '\0';
      ptr = strchr(str, '#');
      if (ptr)
        *ptr = '\0'; // remove comments from partimagedusers

      ptr = str;
      while (*ptr && isblank(*ptr)) // remove starting spaces or tabs
        ++ptr;
 
      ptr2 = ptr+strlen(ptr);       // *ptr2 = '\0'
                                    // warning: maybe *ptr=*ptr2='\0'
      while (ptr2 > ptr)            
        {
          --ptr2;
          if ( isblank(*ptr2) )
            *(ptr2) = '\0';         // remove trailing spaces or tabs 
        }
       
      if (!strcmp(ptr, szLogin)) 
        {
          found = 1;
          break;
        }
    }
  free(str);
  fclose(f);

  if (!found)
    RETURN_int(2);

  g_privs->Root();
#ifdef OS_LINUX
  s_pass = getspnam(szLogin);
#endif
#ifdef OS_FBSD
  s_pass = getpwnam(szLogin);
#endif
  g_privs->User();

  if (!s_pass)
    RETURN_int(3);

#ifdef OS_LINUX
  if (!strcmp(s_pass->sp_pwdp, szPasswd))
    RETURN_int(0)
  else
    RETURN_int(10);
#endif
#ifdef OS_FBSD
  if (strncmp(s_pass->pw_passwd, "$1$", 3))
    {
      showDebug(1, "password for %s is not a MD5, I don't kwow what to do.\n",
         szLogin);
      RETURN_int(4);
    }
  if (!strcmp(s_pass->pw_passwd, szPasswd))
    RETURN_int(0)
  else
    RETURN_int(10);
#endif
}
#else // HAVE_PAM
static char * g_szPassword;
int pam_conv_partimaged(int nmsg, const struct pam_message ** p_messages,
   struct pam_response ** p_responses, void * p_adddata)
{
  struct pam_response * response;
  
  response = (struct pam_response *) malloc (sizeof(struct pam_response)*nmsg);
  if (!response)
    {
      showDebug(2, "PAM: cannot allocate memory for response\n");
      return PAM_CONV_ERR;
    }
  showDebug(1, "PAM: conv called\n");
  for (int i=0; i < nmsg; ++i)
    switch (p_messages[i] -> msg_style)
    {
      case PAM_PROMPT_ECHO_OFF:   // send user password to PAM module
        showDebug(1, "PAM ECHOOFF\n");
        response[i].resp_retcode = PAM_SUCCESS;
        response[i].resp = strdup(g_szPassword);
        break;

      case PAM_PROMPT_ECHO_ON:    // we don't want this
        showDebug(2, "PAM: PROMPT_ECHO_ON refused by us.\n"); 
        free(response);
        return PAM_CONV_ERR;

      case PAM_TEXT_INFO:
      case PAM_ERROR_MSG:
        showDebug(2, "PAM message: %s\n", p_messages[i] -> msg); 
        response[i].resp_retcode = PAM_SUCCESS; 
        response[i].resp = NULL;
        break;

      default:
        showDebug(2, "PAM: wrong message code: %d.\n",
           p_messages[i] -> msg_style); 
        free(response);
        return PAM_CONV_ERR;
    }
  *p_responses = response;  // send answers for messages to PAM module
  return PAM_SUCCESS; 
}

unsigned int CheckAccess(bool bMustLogin, char * szLogin, char * szPasswd)
{
  int nRes;
  pam_handle_t * pamh;
  struct pam_conv partimaged_conv = { &pam_conv_partimaged, 0 };

  if (!bMustLogin)
    RETURN_int(0)

  g_szPassword = strdup(szPasswd);

  nRes = pam_start("partimaged", szLogin, &partimaged_conv, &pamh);
  if (nRes != PAM_SUCCESS)
    {
      pam_end(pamh, nRes);
      showDebug(1, "pam_start failed: %s\n", 
         pam_strerror(pamh, nRes));
      return 1;
    }
 
  g_privs->Root(); 
  nRes = pam_authenticate(pamh, 0);
  g_privs->User(); 
  if (nRes != PAM_SUCCESS)
    {
      pam_end(pamh, nRes);
      showDebug(1, "pam_autenticate failed: %s\n", 
         pam_strerror(pamh, nRes));
      return 2;
    }
  
  g_privs->Root(); 
  nRes = pam_acct_mgmt(pamh, 0);
  g_privs->User(); 
  if (nRes != PAM_SUCCESS)
    {
      pam_end(pamh, nRes);
      showDebug(1, "pam_acct_mgmt failed: %s\n", 
         pam_strerror(pamh, nRes));
      return 3;
    }
  
  nRes = pam_end(pamh, nRes);
  return (nRes == PAM_SUCCESS ? 0 : 4);
}
#endif  // HAVE_PAM
#else   // MUST_LOGIN
unsigned int CheckAccess(bool bMustLogin, char * szLogin, char * szPasswd)
{
  RETURN_int(0);
}
#endif

#ifndef HAVE_PAM
static void help(char * szFile)
{
  fprintf(stderr,"error when verifying %s, check logfile for help\n",
     szFile);
  fprintf(stderr, "logfile is %s\n", PARTIMAGED_LOG);
  fprintf(stderr,"if you don't have logfile, use partimaged --debug=1\n");
}

unsigned int CheckAccessFile(char * szFile)
{
  int nRes;
  struct stat status;
  mode_t mode;

  showDebug(2, "verifing %s access rights\n", szFile); 
  nRes = stat(szFile, &status);
  if (nRes)
    {
      help(szFile);
      showDebug(0, "error on stat for %s: %s\n", szFile, strerror(errno));
      return 1;
    }
  mode = status.st_mode;
  if (!S_ISREG(mode))  
    {
      help(szFile);
      showDebug(0, "file %s is not a regular file\n", szFile);
      return 1;
    }
/*
  if (status.st_uid != 0 || status.st_gid != 0)
    {
      help(szFile);
      showDebug(0, "file %s must belong to root.root\n", szFile);
      return 1;
    }
*/

  if (mode != (S_IFREG | S_IRUSR | S_IWUSR))
    {
      help(szFile);
      showDebug(0, "file %s must be chmoded 0600\n", szFile);
      showDebug(0, "enter \"chmod 0600 %s\"\n", szFile);
      showDebug(0, "it's %o, see stat manpage for help\n");
      return 1;
    }
  
  return 0;
}
#else // HAVE_PAM
unsigned int CheckAccessFile(char * szFile)
{
  return 0;
}
#endif
