rem /* Windows 16-bit - Visual C/C++ 1.5
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
set CC=cl -nologo -AL -Mq
%CC% -O -W3 -I%LZODIR%\include -c src\*.c
%CC% -Felzop.exe *.obj lzo_l.lib setargv.obj /link /noe
@call b\unset.bat
