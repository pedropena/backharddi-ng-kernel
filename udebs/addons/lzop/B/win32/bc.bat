rem /* Windows 32-bit - Borland C/C++ 5.5.1
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
bcc32 -O2 -d -w -w-aus -I%LZODIR%\include -L%LZODIR% -ls -elzop.exe src\*.c lzo.lib
@call b\unset.bat
