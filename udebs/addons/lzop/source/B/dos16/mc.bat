rem /* DOS 16-bit - Microsoft C 7.0
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
set CC=cl -nologo -AL
%CC% -O -W3 -I%LZODIR%\include -c src\*.c
%CC% -F 2000 -Felzop.exe *.obj lzo_l.lib setargv.obj /link /noe
@call b\unset.bat
