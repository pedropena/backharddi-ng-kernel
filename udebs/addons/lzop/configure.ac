#
#  configure.ac -- autoconf configuration for the lzop package
#
#  This file is part of the lzop file compressor.
#
#  Copyright (C) 1996-2003 Markus Franz Xaver Johannes Oberhumer
#  All Rights Reserved.
#
#  lzop and the LZO library are free software; you can redistribute them
#  and/or modify them under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of
#  the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; see the file COPYING.
#  If not, write to the Free Software Foundation, Inc.,
#  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  Markus F.X.J. Oberhumer
#  <markus@oberhumer.com>
#  http://www.oberhumer.com/opensource/lzop/
#


# /***********************************************************************
# // Init
# ************************************************************************/

AC_PREREQ(2.57)
AC_INIT(lzop,1.01,lzop-bugs@oberhumer.com)
AC_CONFIG_SRCDIR(src/lzop.c)
AC_CONFIG_AUX_DIR(acconfig)
AC_PREFIX_DEFAULT(/usr/local)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_MAINTAINER_MODE

if test -r .Conf.settings1; then
  . ./.Conf.settings1
fi

AC_ARG_ENABLE(ansi, AC_HELP_STRING([--disable-ansi], [disable ansi console]))
if test "X$enable_ansi" != Xno; then
  AC_DEFINE(LZOP_ENABLE_ANSI,1,[Define to 1 if should be enabled.])
fi

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CPP
AC_C_CONST
AC_C_BIGENDIAN([AC_DEFINE(LZOP_BYTE_ORDER,4321,[Define to your byte order.])],[AC_DEFINE(LZOP_BYTE_ORDER,1234)])
AC_SYS_LARGEFILE

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h:config.hin)


# /***********************************************************************
# // Checks for LZO library
# ************************************************************************/

AC_CHECK_HEADERS(lzoconf.h lzo1x.h,,AC_MSG_ERROR([LZO header file not found. Please check your installation or set the environment variable \`CPPFLAGS'.]))

AC_CHECK_LIB(lzo,__lzo_init2,,AC_MSG_ERROR([LZO library not found. Please check your installation or set the environment variable \`LDFLAGS'.]))

AC_CHECK_LIB(lzo,lzo1x_decompress_asm_fast,AC_DEFINE(LZO_USE_ASM_1,1,[Use asm.]))
AC_CHECK_LIB(lzo,_lzo1x_decompress_asm_fast,AC_DEFINE(LZO_USE_ASM_2,1,[Use asm.]))


# /***********************************************************************
# // Checks for header files
# ************************************************************************/

AC_HEADER_TIME
AC_CHECK_HEADERS([assert.h ctype.h dirent.h errno.h fcntl.h limits.h malloc.h memory.h signal.h stdarg.h stddef.h stdint.h stdio.h stdlib.h string.h strings.h time.h unistd.h utime.h sys/stat.h sys/time.h sys/types.h])
AC_CHECK_HEADERS([conio.h direct.h dos.h io.h share.h sys/utime.h])
dnl AC_CHECK_HEADERS([linux/kd.h linux/kdev_t.h linux/major.h])
dnl AC_CHECK_HEADERS([curses.h ncurses.h])


# /***********************************************************************
# // Checks for typedefs and structures
# ************************************************************************/

AC_TYPE_OFF_T
AC_CHECK_TYPE(ptrdiff_t,long)
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_STRUCT_ST_MTIM_NSEC

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(__int64)
AC_CHECK_SIZEOF(unsigned __int64)
AC_CHECK_SIZEOF(intmax_t)
AC_CHECK_SIZEOF(uintmax_t)
AC_CHECK_SIZEOF(intptr_t)
AC_CHECK_SIZEOF(uintptr_t)

AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)

AC_CHECK_SIZEOF(dev_t)
AC_CHECK_SIZEOF(fpos_t)
AC_CHECK_SIZEOF(mode_t)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(ptrdiff_t)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(ssize_t)
AC_CHECK_SIZEOF(time_t)

AC_CHECK_SIZEOF(char *)
AC_CHECK_SIZEOF(void *)


# /***********************************************************************
# // Checks for library functions
# ************************************************************************/

AC_CHECK_FUNCS(access alloca atoi atol chmod chown ctime difftime fstat gettimeofday gmtime localtime lstat memcmp memcpy memmove memset mktime snprintf strchr strdup strerror strftime strrchr umask utime vsnprintf)
AC_CHECK_FUNCS(fchmod stricmp strnicmp)


# /***********************************************************************
# // Write output files
# ************************************************************************/

if test -r .Conf.settings2; then
  . ./.Conf.settings2
fi

CPPFLAGS="$CPPFLAGS -DLZOP_HAVE_CONFIG_H"

AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile])
AC_OUTPUT

cat <<EOF

   $PACKAGE_NAME $PACKAGE_VERSION configured.

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


After installing $PACKAGE_NAME, please read the accompanied documentation.

EOF

# vi:ts=4:et
