rem /* DOS 16-bit - Borland C/C++ 5.02
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
bcc -ml -O1 -w -Isrc -I%LZODIR%\include -elzop.exe src\*.c lzo_l.lib
@call b\unset.bat
