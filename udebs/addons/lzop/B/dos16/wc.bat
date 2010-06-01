rem /* DOS 16-bit - Open Watcom C/C++ 1.0
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
set WCL=-zq -bt#dos -l#dos -I%LZODIR%\include
wcl -ml -ox -w5 -k0x2000 -fm -Fe#lzop.exe src\*.c /"libp %LZODIR%" lzo_l.lib
@call b\unset.bat
