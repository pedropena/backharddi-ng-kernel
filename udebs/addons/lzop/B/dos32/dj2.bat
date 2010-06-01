rem /* DOS 32-bit - djgpp v2 + gcc
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
gcc @b/dos32/dj2.opt -I%LZODIR%/include -o lzop.exe src/*.c -L%LZODIR% -llzo
stubedit lzop.exe bufsize=64512 minstack=65536
@call b\unset.bat
