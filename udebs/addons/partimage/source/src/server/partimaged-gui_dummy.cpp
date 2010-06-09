/***************************************************************************
                          partimaged-gui_dummy.cpp  -  description
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "partimaged-gui_dummy.h"
#include "partimage.h"
#include "partimaged.h"
#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

// =============================================================================
CPartimagedInterfaceDummy::CPartimagedInterfaceDummy()
{
}

// =============================================================================
CPartimagedInterfaceDummy::~CPartimagedInterfaceDummy()
{
}

// =============================================================================
void CPartimagedInterfaceDummy::show()
{
}

// =============================================================================
void CPartimagedInterfaceDummy::SetState(int client, char * state)
{
}

// =============================================================================
void CPartimagedInterfaceDummy::SetHostname(int client, char * state)
{
}

// =============================================================================
void CPartimagedInterfaceDummy::SetLocation(int client, char * locate)
{
}

// =============================================================================
void CPartimagedInterfaceDummy::SetSize(int client, QWORD size)
{
}

// =============================================================================
void CPartimagedInterfaceDummy::SetSize(int client, char * size)
{
}

// =============================================================================
void CPartimagedInterfaceDummy::Status(char * msg)
{
}
