Simple make drivers for DOS, Windows, OS/2 and other systems
============================================================

This directory contains a bunch of simple make drivers - I've tried
to make them as foolproof as possible.

To build lzop for your system type `b\OS\COMPILER' in the base directory,
e.g. `b\win32\vc.bat' will build the Win32 Visual C++ version.

All scripts expect that the LZO library has been installed and built
in the directory ..\lzo-1.08. You can also set the environment variable
`LZODIR' to point to your LZO directory, e.g. `set LZODIR=c:\src\lzo-1.08'

Please send me your additional/improved versions.


Overview:
---------

b\dos16\bc.bat          Borland C/C++ 5.02                    (1)
b\dos16\mc.bat          Microsoft C 7.0                       (1)
b\dos16\wc.bat          Watcom C/C++                          (1)
b\dos16\vc.bat          Visual C/C++ 1.5                      (1)

b\dos32\dj2.bat         djgpp v2 + gcc
b\dos32\emx.bat         emx + gcc
b\dos32\wc.bat          Watcom C/C++

b\os2\emx.bat           emx + gcc                             (2)
b\os2\wc.cmd            Watcom C/C++                          (2)

b\win16\bc.bat          Borland C/C++ 5.02                    (1)
b\win16\wc.bat          Watcom C/C++                          (1)
b\dos16\vc.bat          Visual C/C++ 1.5                      (1)

b\win32\bc.bat          Borland C/C++
b\win32\cygwin.bat      Cygwin + gcc
b\win32\dm.bat          Digital Mars C/C++
b\win32\ic.bat          Intel C/C++
b\win32\mingw.bat       MinGW + gcc
b\win32\wc.bat          Watcom C/C++
b\win32\vc.bat          Visual C/C++


Notes:
  (1) 16-bit versions should work but are not officially supported
  (2) not tested as of version 1.01


