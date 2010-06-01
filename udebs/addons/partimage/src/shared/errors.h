/***************************************************************************
                          errors.h  -  list of exceptions
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _ERRORS_H_
#define _ERRORS_H_

// NB: exceptions have been renamed by category.
// ---------------------------------------------

// ERR_xxx which requires one or more arguments:
// ERR_ERRNO: int errno
// ERR_READ_BITMAP: int block
// ERR_READING: int block, int errno
// ERR_WRITING: int block, int errno
// ERR_ZEROING: int errno
// ERR_OPENING_DEVICE: int errno
// ERR_CHECK_CRC: QWORD oldcrc, QWORD newcrc
// ERR_CHECK_NUM: DWORD expected, DWORD got
// ERR_VOLUMENUMBER: DWORD expected, DWORD got
// ERR_LOCKED: char * partition/filename
// ERR_VERSION_OLDER: char * wrong_release 
// ERR_VERSION_NEWER: char * wrong_release 
// ERR_NOTAPARTIMAGEFILE: char * filename
// ERR_NOTAREGULARFILE: char * filename


// don't use -1

// now useless exceptions
// #define WARN_SPLITNEEDED 
// #define ERR_OTHER 
// #define ERR_PANIC 
// #define ERR_ZVERS 
// #define ERR_OPENING_PART 


//// general errors
// not enough memory to continue
#define ERR_NOMEM -10
// user canceled operation
#define ERR_CANCELED -11
// wrong password
#define ERR_PASSWD -12
#define ERR_ERRNO -13
// file was locked somewhere else
#define ERR_LOCKED -14
// encrypt is not yet supported
#define ERR_ENCRYPT -15
// no mbr were saved into imagefile, we can't restore one
#define ERR_NOMBR -16
// current version is older than release which backuped imagefile
#define ERR_VERSION_OLDER -17
// current version is newer than release which backuped imagefile
#define ERR_VERSION_NEWER -18
#define ERR_PART_TOOSMALL -19
#define ERR_DESTROYSPACEFILE -20
#define ERR_CREATESPACEFILE -21


//// buffer errors
#define ERR_BUFFERFULL -25


//// network errors
#define ERR_HOST -29
#define ERR_SSL_CONNECT -30
#define ERR_SSL_CTX -31
#define ERR_SSL_LOADCERT -32
#define ERR_SSL_LOADKEY -33
#define ERR_SSL_PRIVKEY -34
#define ERR_TOOMANY -35
// connexion refused by partimaged (ssl or login configuration mismatch
// from configuration of partimaged)
#define ERR_REFUSED -36
// client and server versions mismatch
#define ERR_VERSIONSMISMATCH -37


//// errors for image file
// filename doesn't exist(restauring) or already exist(saving)
#define ERR_EXIST -40
#define ERR_ACCESS -41
#define ERR_COMP -42
#define ERR_EOF -43
// the file can't be found
#define ERR_NOTFOUND -44
#define ERR_OPENED -45
#define ERR_PATH -46
#define ERR_FULL -47
// cannot be repared
#define ERR_INCOMPLETEFINISH -48
// file is a symlink or something like this. we can't work on it.
#define ERR_NOTAREGULARFILE -49


//// corruption of imagefile
// file isn't an imagefile or had been damaged
#define ERR_NOTAPARTIMAGEFILE -60
// volume header can't be read. file isn't an imagefile or is damaged
#define ERR_VOLUMEHEADER -61
// file isn't an imagefile because volumemagic isn't found or file damaged
//#define ERR_PARTITIONVOLUME -62
// the volume number is not the expected one
#define ERR_VOLUMENUMBER -63
// an unexpected magic had been read
#define ERR_WRONGMAGIC -64
#define ERR_WRONGMAGICINVALIMGFILE -6
#define ERR_INCOMPLETE -66
// an (should be) uniq identificator is append to each imagefile
// and volumes compared to this identificator. volumeid is raised when a
// volumeid mismatches.
#define ERR_VOLUMEID -67
#define ERR_CHECK_CRC -68
// some blocknumbers are added into imagefile. if they mismatch when restauring
// check_num is raised
// warning: avoid confusion with volumenumber 
#define ERR_CHECK_NUM -69


//// automount errors
// cannot mount device with automount
#define ERR_AUTOMOUNT -80
// cannot unmount device with automount
#define ERR_AUTOUMOUNT -81


//// errors on partition (hardware failure ?)
// can't extract mbr from disk
#define ERR_SAVE_MBR -90
#define ERR_WRONG_FS -91
#define ERR_READ_SUPERBLOCK -92
#define ERR_ZEROING -93
#define ERR_OPENING_DEVICE -94
#define ERR_READ_BITMAP -95
#define ERR_READING -96
#define ERR_WRITING -97
// fs check failed 
#define ERR_FSCHK -98
#define ERR_FAT_MISMATCH -99

#define ERR_CREATESPACEFILENOSPC -100
#define ERR_CREATESPACEFILEDENIED -101

#define ERR_BLOCKSIZE -102

#define ERR_FILETOOLARGE -103


// not really an error -> only a message
#define ERR_COMEBACK -1000
#define ERR_SPLITSIZE -1001

#endif
