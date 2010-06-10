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


/*
 * Possible configuration values:
 *
 *   ACC_CONFIG_HEADER          if given, then use this as <config.h>
 *   ACC_CONFIG_INCLUDE         include path to acc_ files
 *   ACC_CONFIG_FRESSTANDING    only use <limits.h> and possibly <stdint.h>
 *                              [use this for embedded targets]
 */


#ifndef __ACC_H_INCLUDED
#define __ACC_H_INCLUDED

#define ACC_VERSION     20030427L

#if !defined(ACC_CONFIG_INCLUDE)
#  define ACC_CONFIG_INCLUDE(file)     file
#endif


#if defined(__CYGWIN32__) && !defined(__CYGWIN__)
#  define __CYGWIN__ __CYGWIN32__
#endif
#if defined(__ICL) && !defined(__INTEL_COMPILER)
#  define __INTEL_COMPILER __ICL
#endif

/* disable pedantic warnings */
#if defined(__INTEL_COMPILER) && defined(__linux__)
#  pragma warning(disable: 193)     /* #193: zero used for undefined preprocessing identifier */
#endif
#if 0 && defined(__WATCOMC__)
#  if (__WATCOMC__ < 1000)
#    pragma warning 203 9           /* W203: Preprocessing symbol '%s' has not been declared */
#  endif
#endif


#include <limits.h>
#include ACC_CONFIG_INCLUDE("acc_init.h")
#include ACC_CONFIG_INCLUDE("acc_os.h")
#include ACC_CONFIG_INCLUDE("acc_cc.h")
#include ACC_CONFIG_INCLUDE("acc_mm.h")
#include ACC_CONFIG_INCLUDE("acc_arch.h")
#include ACC_CONFIG_INCLUDE("acc_defs.h")

#if defined(ACC_CONFIG_HEADER)
#  include ACC_CONFIG_HEADER
#else
#  include ACC_CONFIG_INCLUDE("acc_auto.h")
#endif

#include ACC_CONFIG_INCLUDE("acc_type.h")

#if !defined(ACC_CONFIG_FREESTANDING)
#  include ACC_CONFIG_INCLUDE("acc_incd.h")
#  include ACC_CONFIG_INCLUDE("acc_lib.h")
#endif

#endif /* already included */


/*
vi:ts=4:et
*/
