/***************************************************************************
                            imginfo.h  -  description
                             -------------------
    begin                : Mon May 22 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef IMGINFO_H
#define IMGINFO_H

class CMainHeader;
class CVolumeHeader;
class CMbr;

void imageInfoShowVolume(char *szText, int nMaxTextLen, CVolumeHeader *head, char *szImagefile, DWORD dwCompression, QWORD qwImageSize);
void imageInfoShowRegular(char *szText, int nMaxTextLen, CMainHeader *head, char *szImagefile, DWORD dwCompression);
void imageInfoShowMBR(char *szText, int nMaxTextLen, int i, CMbr *mbr);	  
int showImgInfos(char *szImageFile, COptions *options);

#endif // IMGINFO_H
