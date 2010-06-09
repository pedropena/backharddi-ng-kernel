/***************************************************************************
                          messages.cpp  -  class to handle net messages
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "messages.h"

void CMessages::setMessage(DWORD _cmd)
{
  DWORD _size;
  memcpy(msg, &_cmd, sizeof(DWORD));
  memcpy(msg+OFF_SIZE, &_size, sizeof(DWORD));
}

void CMessages::setMessage(DWORD _cmd, DWORD _size)
{
  memcpy(msg, &_cmd, sizeof(DWORD));
  memcpy(msg+OFF_SIZE, &_size, sizeof(DWORD));
}

void CMessages::getMessage(DWORD *_cmd, DWORD *_size)
{
  memcpy(_cmd, msg, sizeof(DWORD));
  memcpy(_size, msg+OFF_SIZE, sizeof(DWORD));
}
