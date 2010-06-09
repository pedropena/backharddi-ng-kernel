/***************************************************************************
                          zerox.cpp  -  description
                             -------------------
    begin                : Wed May 30 21:31:30 CEST 2001
    copyright            : (C) 2001 by Francois Dupoux
    email                : fdupoux@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  char c;
  FILE *fStream;
  
  // chech arguments
  if (argc != 3)
    {
      printf ("command: zerox <file> <char>\n\n");
      printf ("example: zerox /dev/hda1 65 to fill this device with 'A'\n");
      printf ("This is useful when we don't want to use /dev/zero\n");
      return EXIT_FAILURE;
    }
  
  c = atoi(argv[2]);
  printf("filling file [%s] with [%d]=[0x%.2X]...\n\n",argv[1], c ,c);
  
  // open file
  fStream = fopen(argv[1], "r+b");
  if (!fStream)
    {
      printf ("cannot open file %s\n");
      return EXIT_FAILURE;
    }
  
  while (!feof(fStream))
    putc(c, fStream);	
  
  fclose(fStream);
  
  return EXIT_SUCCESS;
}
