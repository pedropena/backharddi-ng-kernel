/***************************************************************************
                          partimaged-gui.h  -  description
                             -------------------
    begin                : Wed Feb 21 2001
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

#ifndef PARTIMAGED_GUI_H
#define PARTIMAGED_GUI_H

#include "partimage.h"
#include "partimaged.h"

#include <time.h>

// =======================================================
class CPartimagedInterface
{
public:
  CPartimagedInterface(){}
  virtual ~CPartimagedInterface(){};
  virtual void Status(char * msg)=0;
  virtual void show()=0;
  virtual void SetState(int client, char * state)=0;
  virtual void SetHostname(int client, char * state)=0;
  virtual void SetLocation(int client, char * locate)=0;
  virtual void SetSize(int client, QWORD size)=0;
  virtual void SetSize(int client, char * size)=0;
};

#endif // PARTIMAGED_GUI_H

