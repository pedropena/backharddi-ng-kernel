/* mblock.c -- aligned memory blocks (cache issues)

   This file is part of the lzop file compressor.

   Copyright (C) 1996-2003 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   lzop and the LZO library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzop/
 */


#include "conf.h"


/*************************************************************************
//
**************************************************************************/

static void do_init(memblock_p m, lzo_uint32 size, lzo_uint align)
{
    memset(m,0,sizeof(*m));
    m->size = size;
    m->align = (align > 1) ? align : 1;
    assert((m->align & (m->align - 1)) == 0);
}


lzo_bool mb_init(memblock_p m, lzo_uint32 size, lzo_uint align,
                 lzo_voidp heap, lzo_uint32 heap_size)
{
    do_init(m,size,align);
    if (m->size == 0)
        return 1;

    if (heap == 0)
        return 0;
    m->mem_alloc = (lzo_bytep) heap;
    m->size_alloc = heap_size;
    assert(m->size_alloc >= m->size + m->align - 1);

    m->mem = LZO_PTR_ALIGN_UP(m->mem_alloc,m->align);
    assert(m->mem >= m->mem_alloc);
    assert(m->mem + m->size <= m->mem_alloc + m->size_alloc);
#if 0
    printf("m_init: %p %p %8ld %8ld %8ld\n", m->mem_alloc, m->mem,
           (long) m->size_alloc, (long) m->size, (long) m->align);
#endif
    return 1;
}


lzo_bool mb_alloc(memblock_p m, lzo_uint32 size, lzo_uint align)
{
    do_init(m,size,align);
    if (m->size == 0)
        return 1;

    m->size_alloc = m->size + m->align - 1;
    m->mem_alloc = (lzo_byte *) malloc(m->size_alloc);
    if (m->mem_alloc == NULL)
        return 0;
    m->flags = 1;

    m->mem = LZO_PTR_ALIGN_UP(m->mem_alloc,m->align);
    assert(m->mem >= m->mem_alloc);
    assert(m->mem + m->size <= m->mem_alloc + m->size_alloc);
#if 0
    printf("m_alloc: %p %p %8ld %8ld %8ld\n", m->mem_alloc, m->mem,
           (long) m->size_alloc, (long) m->size, (long) m->align);
#endif
    return 1;
}


void mb_free(memblock_p m)
{
    if (m->flags & 1)
    {
        free(m->mem_alloc);
    }
    memset(m,0,sizeof(*m));
}


/*
vi:ts=4:et
*/

