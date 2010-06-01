rem /* Windows 32-bit - Digital Mars C/C++ 8.33
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
dmc -mn -o -w- -I%LZODIR%\include -olzop.exe @b\win32\dm.rsp lzo.lib -L/map
@call b\unset.bat
