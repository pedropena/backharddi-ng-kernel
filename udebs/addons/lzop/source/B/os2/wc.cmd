rem /* OS/2 32-bit - Open Watcom C/C++ 1.0
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
set WCL386=-zq -bt#os2v2 -l#os2v2 -I%LZODIR%\include
wcl386 -mf -5r -ox -w5 -fm -Fe#lzop.exe src\*.c /"libp %LZODIR%" lzo.lib
@call b\unset.bat
