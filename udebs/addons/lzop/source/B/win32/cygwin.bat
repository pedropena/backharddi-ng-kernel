rem /* Windows 32-bit - Cygwin + gcc
rem  * a very simple make driver
rem  * Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
rem  */

@call b\prepare.bat
gcc -mcygwin -x c -O2 -Wall -I%LZODIR%/include -s -o lzop.exe src/*.[cC] -L%LZODIR% -llzo -Wl,-Map,lzop.map
@call b\unset.bat
