/***************************************************************************
                          messages.h  -  description
                             -------------------
    begin                : Mon Oct 30 2000
    copyright            : (C) 2000 by Franck Ladurelle
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

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <string.h>
#include <stdlib.h>

#include "misc.h"
#include "partimage.h"

#define MSG_OK 0
#define MSG_OPENW 1
#define MSG_WRITE 2
/// #define MSG_WRITEMAGIC 3
#define MSG_READMAGIC 4
#define MSG_READ 5
#define MSG_CLOSE 6
#define MSG_OPENR 7
#define MSG_ISSPLIT 8
#define MSG_GETERROR 9
#define MSG_ERROR 10
#define MSG_GETSPACE 11
/// #define MSG_UNWRITE 12
#define MSG_DOESFILEEXIST 13
#define MSG_GETDISKSPACEFILE 14
#define MSG_GETFILESIZE 15
#define MSG_MOUNTIMAGELOCATION 16
#define MSG_UMOUNTIMAGELOCATION 17
#define MSG_NEWOPTIONS 18

#define MSG_SET_CVS 20
#define MSG_SET_VN 21
//#define MSG_SET_C 22 
//#define MSG_SET_CRC 23 
#define MSG_SET_IS 24 
#define MSG_SET_IO 25 
#define MSG_SET_P 26 
#define MSG_SET_IF 27 
#define MSG_SET_OF 28 

#define MSG_GET_CVS 30
#define MSG_GET_VN 31
#define MSG_GET_C 32 
//#define MSG_GET_CRC 33 
#define MSG_GET_IS 34 
#define MSG_GET_IO 35 
#define MSG_GET_P 36 
#define MSG_GET_IF 37 
#define MSG_GET_OF 38 

#define MSG_QUIT 40

#define OFF_SIZE (sizeof(DWORD))
#define MESSAGE_LEN (OFF_SIZE + sizeof(DWORD))

class CMessages
{
private:
  char msg[sizeof(DWORD)+sizeof(DWORD)];

public:
  CMessages() {}

  void setMessage(DWORD _cmd);
  void setMessage(DWORD _cmd, DWORD _size);
  void getMessage(DWORD *_cmd, DWORD *_size);

  void getCmd(DWORD * cmd)
    {
      memcpy(cmd, msg, sizeof(DWORD));
    }
      
  void getSize(DWORD * size)
    {
      memcpy(size, msg+OFF_SIZE, sizeof(DWORD));
    }
  char * toString() { return msg; }
  void fromString(char * _msg) { memcpy(msg, _msg, MESSAGE_LEN); }
};


#endif // _MESSAGES_H_
