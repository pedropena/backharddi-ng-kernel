/***************************************************************************
                          interface_none.h  -  description
                             -------------------
    begin                : Sun 6 Apr 2003
    copyright            : (C) 2003 by Ian Jackson
    email                : ian@davenant.greenend.org.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fnmatch.h>

#include "interface_none.h"
            
CInterfaceNone::CInterfaceNone(char *a_optstring) : CInterface(true) {
  optstring= a_optstring;
  count= 0;
  char *where= optstring;
  while (*where) {
    char *semicolon= strchr(where,';');
    if (semicolon) *semicolon= 0;

    char *equals= strchr(where,'=');
    if (!equals) {
      fprintf(stderr,"partimage: no equals in `%s'\n",where);
      invalid_programmed_response();
    }
    *equals= 0;
    count++;

    if (!semicolon) return;
    where= semicolon+1;
  }
}

void CInterfaceNone::invalid_programmed_response() {
  fprintf(stderr,"partimage: invalid programmed response.\n");
  flusherr();
  exit(8);
}

char *CInterfaceNone::lookup(char *title, char *text, char *deflt) {
  char *globfor= (char*)malloc(strlen(title) + strlen(text) + 5);
  if (!globfor) { perror("malloc for lookup"); exit(16); }
  sprintf(globfor, "%s/%s", title, text);
  char *where= optstring;
  char *value;
  while (count) {
    value= where + strlen(where) + 1;
    if (!fnmatch(where,globfor,0)) { free(globfor); return value; }
    where= value + strlen(value) + 1;
    count--;
  }
  free(globfor);
  if (deflt) return deflt;
  fprintf(stderr,"partimage: %s (%s) no default\n", title, text);
  exit(8);
}
      

void CInterfaceNone::flusherr() {
  if (ferror(stderr) || fflush(stderr)) exit(16);
}

void CInterfaceNone::message_only(char *kind, char *title,
				  char *format, va_list al,
				  char *result) {
  fprintf(stderr,"partimage: %s [%s]\npartimage:  ", title, kind);
  vfprintf(stderr,format,al);
  fputc('\n',stderr);
  if (result) fprintf(stderr,"partimage:  => %s\n",result);    
  flusherr();
}

#define MB_1(KInd,Kind)							\
  void CInterfaceNone::msgBox##KInd(char *title, char *text, ...) {	\
    va_list al;								\
    va_start(al,text);							\
    message_only(#Kind, title, text, al, 0);				\
    va_end(al);								\
  }

MB_1(Ok,OK)
MB_1(Cancel,Cancel)

void CInterfaceNone::msgBoxError(char *title, ...) {
  va_list al;						
  va_start(al,title);					
  message_only("Error", title, "", al, 0);		
  va_end(al);						
}

#define MB_2(One,Other,ONE,OTHER)					  \
  int CInterfaceNone::msgBox##One##Other(char *title, char *text, ...) {  \
    char *result= lookup(title,text,"(unspecified)");			  \
    va_list al;								  \
    va_start(al,text);							  \
    message_only(#One "/" #Other, title, text, al, result);		  \
    va_end(al);								  \
    if (!strcasecmp(result,#One)) return MSGBOX_##ONE;			  \
    if (!strcasecmp(result,#Other)) return MSGBOX_##OTHER;		  \
    invalid_programmed_response();					  \
    return 0;                                                             \
  }

MB_2(Continue,Cancel,CONTINUE,CANCEL)
MB_2(Yes,No,YES,NO)

void CInterfaceNone::showErrorInternal(char *text) {
  fprintf(stderr,"partimage: internal error: %s\n", text);
  flusherr();
}

void CInterfaceNone::askDescription(char * szDescription,
					 DWORD size) {
  askText("Description","Description",szDescription,size);
}

int CInterfaceNone::askText(char *message, char *title,
				 char *resbuf, WORD size) {
  char *result= lookup(title,message,0);
  if (strlen(result) >= size) {
    fprintf(stderr,"partimage: %s (%s) too long (>%lu)\n", title, message,
	    (unsigned long)size);
    exit(12);
  }
  strcpy(resbuf,result);
  return 0;
}

void CInterfaceNone::StatusLine(char * str) {
  fprintf(stderr,"partimage: status: %s\n", str);
  flusherr();
}

#define DONT(rt,what,args)				\
  rt CInterfaceNone::what args {			\
    fprintf(stderr,"partimage: " #what			\
	    " not supported in CInterfaceNone\n");	\
    exit(8);						\
  }

DONT(int, askLogin,    (char * szLogin, char * szPasswd, WORD size))
DONT(WORD, askNewPath, (char * szOriginalFilename, DWORD dwVolume,
			  char * szPath, char *szNewPath, WORD size))
