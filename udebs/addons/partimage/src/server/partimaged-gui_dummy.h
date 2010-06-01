/***************************************************************************
                          partimaged-gui_dummy.h  -  description
                             -------------------
    begin                : Tue Mar 20 2001
    copyright            : (C) 2001 by Franck Ladurelle
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

#ifndef PARTIMAGED_GUI_DUMMY_H
#define PARTIMAGED_GUI_DUMMY_H

#include <newt.h>
#include <time.h>

#include "partimage.h"
#include "partimaged.h"
#include "partimaged-gui.h"


// =======================================================
class CPartimagedInterfaceDummy : public CPartimagedInterface
{
public:
  CPartimagedInterfaceDummy();
  virtual ~CPartimagedInterfaceDummy();

  virtual void Status(char * msg);
  virtual void show();
  virtual void SetState(int client, char * state);
  virtual void SetHostname(int client, char * state);
  virtual void SetLocation(int client, char * locate);
  virtual void SetSize(int client, QWORD size);
  virtual void SetSize(int client, char * size);
private:
};

#endif // PARTIMAGED_GUI_DUMMY_H

