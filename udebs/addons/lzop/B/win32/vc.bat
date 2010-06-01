rem /* Windows 32-bit - Visual C/C++
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
cl -MD -O2 -GF -W3 -I%LZODIR%\include -Felzop.exe src\*.c lzo.lib setargv.obj /link /map:lzop.map /libpath:%LZODIR%
@call b\unset.bat
