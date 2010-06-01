rem /* OS/2 32-bit - emx + gcc
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
gcc -Wall -O2 -I%LZODIR%\include -s -o lzop.exe src\*.c -L%LZODIR% -llzo
@call b\unset.bat
