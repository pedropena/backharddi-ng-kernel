/***************************************************************************
                          cbitmap.cpp  -  description
                             -------------------
    begin                : Sun Jul 16 2000
    copyright            : (C) 2000 by François Dupoux
    email                : fdupoux@partimage.org
 ***************************************************************************/
// $Revision: 1.4 $
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

#include <string.h>

#include "misc.h"
#include "cbitmap.h"
#include "interface_newt.h"

// =======================================================
CBlocksUseBitmap::CBlocksUseBitmap()
{
  m_cBitmap = NULL;
  m_dwMaxUsedBlocksCopiedPerOper = 1;
  m_dwMaxFreeBlocksCopiedPerOper = 1;
}

// =======================================================
CBlocksUseBitmap::~CBlocksUseBitmap()
{
  freeBitmap();
}

// =======================================================
int CBlocksUseBitmap::setBit(DWORD dwBit, bool bState)
{
  if (m_cBitmap == NULL)
    RETURN_int(-1);
  
  DWORD dwLocalBit;
  DWORD dwByte;
  
  dwLocalBit = dwBit % 8;
  dwByte = (dwBit - dwLocalBit) / 8;
  
  //printf ("dwBit=%ld ==> (byte,bit)=(%ld,%ld)\n", dwByte, dwLocalBit);
  
  // if we must set the bit
  if (bState == true)
    {
      SET_BIT(m_cBitmap[dwByte], dwLocalBit, 1);
      //m_cBitmap[dwByte] |= g_dwBit[dwLocalBit];	
    }
  else // if we must unset the bit
    {
      SET_BIT(m_cBitmap[dwByte], dwLocalBit, 0);
      //m_cBitmap[dwByte] &= ~g_dwBit[dwLocalBit];
    }
  
  return 0;
}

// =======================================================
int CBlocksUseBitmap::init(DWORD dwBitmapSize)
{
  BEGIN;

  // if memory already allocated
  if (m_cBitmap)
    RETURN_int(-1);
  
  m_dwBitmapSize = dwBitmapSize;

  m_cBitmap = new BYTE[dwBitmapSize+1];
  if (m_cBitmap == NULL)
    {	
      g_interface->msgBoxError(i18n("Out of memory"));
      RETURN_int(-1);
    }
  
  memset(m_cBitmap, 0, dwBitmapSize);
  RETURN_int(0);
}

// =======================================================
int CBlocksUseBitmap::freeBitmap()
{
  BEGIN;

  if (m_cBitmap) // if memory was allocated
    {
      delete m_cBitmap;
      m_cBitmap = NULL;
    }
  RETURN_int(0);
}

// =======================================================
int CBlocksUseBitmap::fill(BYTE cValue)
{
  BEGIN;

  DWORD i;
  DWORD dwMemSize;
  
  if (!m_cBitmap) // if memory was not allocated
    RETURN_int(-1);
  
  dwMemSize = m_dwBitmapSize / 8;
  
  for (i=0; i < dwMemSize; i++)
    m_cBitmap[i] = cValue;
  
  RETURN_int(0);
}

// =======================================================
BYTE* CBlocksUseBitmap::data()
{
  return m_cBitmap;
}


// =======================================================
DWORD CBlocksUseBitmap::dataSize()
{
  return m_dwBitmapSize;
}

// =======================================================
int CBlocksUseBitmap::readBitmap(DWORD dwPos, bool *bUsed, DWORD *dwBitsCount)
{
  if (m_cBitmap == NULL)
    RETURN_int(-1);
  
  // is the next bit used or free ?
  *bUsed = isBitSet(dwPos++);
  *dwBitsCount = 1; // currently, 1 bit has been read

  // copy all the bits which are in the same state than the first one
  // TODO: use the real bits count in the bitmap insted of (8*m_dwBitmapSize)
  
  //showDebug(0, "m_dwMaxUsedBlocksCopiedPerOper=%lu\n",m_dwMaxUsedBlocksCopiedPerOper);

  //showDebug(0, "TTT: used=%d, state(+1)=%d,state(+2)=%d,state(+3)=%d\n",(int)*bUsed,(int)isBitSet(dwPos),isBitSet(dwPos+1),isBitSet(dwPos+2),isBitSet(dwPos+3));

  // If we have used bits
  if (*bUsed)
    while ((dwPos < (8*m_dwBitmapSize)) && (*dwBitsCount < m_dwMaxUsedBlocksCopiedPerOper) && (isBitSet(dwPos) == true))
    {
      (*dwBitsCount)++;
      dwPos++;
    }
  else // if free
    while ((dwPos < (8*m_dwBitmapSize)) && (*dwBitsCount < m_dwMaxFreeBlocksCopiedPerOper) && (isBitSet(dwPos) == false))
    {
      (*dwBitsCount)++;
      dwPos++;
    }

  return 0; // success
}
