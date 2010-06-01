/* ACC -- Automatic Compiler Configuration

   Copyright (C) 1996-2003 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   This software is a copyrighted work licensed under the terms of
   the GNU General Public License. Please consult the file "ACC_LICENSE"
   for details.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/
 */


/***********************************************************************
// try to detect specific compilers
************************************************************************/

#if defined(__ZTC__) && defined(__I86__) && (UINT_MAX == 0xffffL)
#  if !defined(__DOS__) && !defined(__OS2__)
#    define __DOS__ 1
#  endif
#elif defined(__VERSION) && (UINT_MAX == 0xffffL) && defined(MB_LEN_MAX)
#  if (__VERSION == 520) && (MB_LEN_MAX == 1)
#    if !defined(__AZTEC_C__)
#      define __AZTEC_C__ __VERSION
#    endif
#    if !defined(__DOS__)
#      define __DOS__ 1
#    endif
#  endif
#endif


/***********************************************************************
// fix incorrect and missing stuff
************************************************************************/

/* Microsoft C does not correctly define ptrdiff_t for
 * the 16-bit huge memory model.
 */
#if defined(_MSC_VER) && defined(M_I86HM) && (UINT_MAX == 0xffffL)
#  if (_MSC_VER < 900)
#    define ptrdiff_t long
#    define _PTRDIFF_T_DEFINED
#  endif
#endif


/* Fix old compiler versions. */
#if defined(__PACIFIC__) && defined(DOS)
#  if !defined(__far)
#    define __far far
#  endif
#  if !defined(__near)
#    define __near near
#  endif
#elif defined(_MSC_VER) && defined(MSDOS)
#  if(_MSC_VER < 700)
#    if !defined(__cdecl)
#      define __cdecl _cdecl
#    endif
#    if !defined(__far)
#      define __far _far
#    endif
#    if !defined(__huge)
#      define __huge _huge
#    endif
#    if !defined(__near)
#      define __near _near
#    endif
#  endif
#elif defined(__TURBOC__) && defined(__MSDOS__)
#  if(__TURBOC__ < 0x0410)
#    if !defined(__cdecl)
#      define __cdecl cdecl
#    endif
#    if !defined(__far)
#      define __far far
#    endif
#    if !defined(__huge)
#      define __huge huge
#    endif
#    if !defined(__near)
#      define __near near
#    endif
#  endif
#endif


#if defined(__MSDOS__) && defined(__TURBOC__) && (__TURBOC__ < 0x0200)
#  define ACC_BROKEN_SIZEOF 1
#  if (__TURBOC < 0x0150)
#    define ACC_BROKEN_INTEGRAL_PROMOTION 1
#  endif
#elif defined(MSDOS) && defined(_MSC_VER) && (_MSC_VER < 700)
#  define ACC_BROKEN_INTEGRAL_PROMOTION 1
#  define ACC_BROKEN_SIZEOF 1
#elif defined(__TOS__) && (defined(__PUREC__) || defined(__TURBOC__))
#  define ACC_BROKEN_SIZEOF 1
#endif


/***********************************************************************
// stdc macros
************************************************************************/

#if defined(__cplusplus)
#  if !defined(__STDC_LIMIT_MACROS)
#    define __STDC_LIMIT_MACROS 1
#  endif
#  if !defined(__STDC_CONSTANT_MACROS)
#    define __STDC_CONSTANT_MACROS 1
#  endif
#endif


/***********************************************************************
// preprocessor macros
************************************************************************/

#define ACC_STRINGIZE(x)        #x
#define ACC_MACRO_EXPAND(x)     ACC_STRINGIZE(x)


/*
vi:ts=4:et
*/
