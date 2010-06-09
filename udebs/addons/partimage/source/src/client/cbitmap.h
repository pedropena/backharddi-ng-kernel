/***************************************************************************
                          misc.h  -  description
                             -------------------
    begin                : Sun Jul 16 2000
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

#ifndef BITMAP_H
#define BITMAP_H

#include <stdarg.h>
#include <pthread.h>

#include "partimage.h"

//extern DWORD g_dwBit[]; 
#define BIT(x) (1 << (x))
#define GET_BIT(n, bit) (!!((n) & BIT(bit)))
#define SET_BIT(n,bit,val) n = (val) ? (n | BIT(bit)) : (n & ~BIT(bit))

// =======================================================
class CBlocksUseBitmap
{
 public:
  CBlocksUseBitmap();
  ~CBlocksUseBitmap();
  
 private:
  BYTE *m_cBitmap;
  DWORD m_dwBitmapSize;
  DWORD m_dwMaxUsedBlocksCopiedPerOper;
  DWORD m_dwMaxFreeBlocksCopiedPerOper;

 public:
  int setBit(DWORD dwBit, bool bState);
  inline int isBitSet(DWORD dwBit);
  
  int init(DWORD dwBitmapSize);
  int freeBitmap();
  
  void setMaxUsedBlocksCopiedPerOper(DWORD dwMaxUsedBlocksCopiedPerOper)
    {m_dwMaxUsedBlocksCopiedPerOper = dwMaxUsedBlocksCopiedPerOper;}
  void setMaxFreeBlocksCopiedPerOper(DWORD dwMaxFreeBlocksCopiedPerOper)
    {m_dwMaxFreeBlocksCopiedPerOper = dwMaxFreeBlocksCopiedPerOper;}
  int readBitmap(DWORD dwPos, bool *bUsed, DWORD *dwBitsCount);

  int fill(BYTE cValue);
  BYTE *data();
  DWORD dataSize();
  //DWORD crc(QWORD qwClustersCount);
};

// =======================================================
inline int CBlocksUseBitmap::isBitSet(DWORD dwBit)
{
  if (m_cBitmap == NULL)
    return -1;
  
  DWORD dwLocalBit;
  DWORD dwByte;
  
  dwLocalBit = dwBit % 8;
  dwByte = (dwBit - dwLocalBit) / 8;
  
  return GET_BIT(m_cBitmap[dwByte], dwLocalBit);
  //return (!!(m_cBitmap[dwByte] & g_dwBit[dwLocalBit]));
}



#endif // BITMAP_H

