/***************************************************************************
                        partimaged.h  -  description
                             -------------------
    begin                : Sun Jan 28 2001
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

#ifndef PARTIMAGED_H
#define PARTIMAGED_H

#include <stdio.h>
#include "pathnames.h"

#define BUFFER_LEN BUFFER_SIZE
#define MAX_CLIENTS 15
#define SERVER_LISTEN_PORT 4025

extern bool g_bBeDaemon;

void * partimaged(void * _s);
void * thread(void * param);

#define showDebug(level, format, args...) showDebugMsg(g_fDebug, level, __FILE__, __FUNCTION__, __LINE__, format, ## args)

extern bool g_bSigKill;
extern bool g_bSigInt;

#endif // PARTIMAGED_H
