/*
   $Header: /cvs/src/tdl/memory.h,v 1.1.2.1 2004/01/07 00:09:05 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001-2004  Richard P. Curnow

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
   */

#ifndef MEMORY_H
#define MEMORY_H
#include <stdlib.h>

#define new_string(s) strcpy((char *) malloc(1+strlen(s)), (s))
#define extend_string(x,s) (strcat(realloc(x, (strlen(x)+strlen(s)+1)), s))
#define new(T) (T *) malloc(sizeof(T))
#define new_array(T, n) (T *) malloc(sizeof(T) * (n))
#define grow_array(T, n, oldX) (T *) ((oldX) ? realloc(oldX, (sizeof(T) * (n))) : malloc(sizeof(T) * (n)))
#endif

