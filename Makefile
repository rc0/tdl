#  $Header: /cvs/src/tdl/Attic/Makefile,v 1.8 2002/05/06 23:14:44 richard Exp $
#  
#  tdl - A console program for managing to-do lists
#  Copyright (C) 2001  Richard P. Curnow
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

CC=gcc
CFLAGS=-g -Wall
#CFLAGS=-O2 -Wall

# Comment out the following 2 lines if you don't want to or can't use the
# readline library.
INC_READLINE=-DUSE_READLINE
LIB_READLINE=-lreadline -lhistory -ltermcap

prefix=/usr/local

bindir=$(prefix)/bin
mandir=$(prefix)/man
man1dir=$(prefix)/man/man1

OBJ = main.o io.o add.o done.o remove.o move.o list.o \
      report.o purge.o util.o dates.o impexp.o inter.o

all : tdl

tdl : $(OBJ)
	$(CC) $(CFLAGS) -o tdl $(OBJ) $(LIB_READLINE)


%.o : %.c
	$(CC) $(CFLAGS) -c $<

inter.o : inter.c
	$(CC) $(CFLAGS) $(INC_READLINE) -c $<

%.s : %.c
	$(CC) $(CFLAGS) -S $<

clean:
	rm -f tdl *.o core

install:
	[ -d $(prefix) ] || mkdir -p $(prefix)
	[ -d $(bindir) ] || mkdir $(bindir)
	[ -d $(mandir) ] || mkdir $(mandir)
	[ -d $(man1dir) ] || mkdir $(man1dir)
	cp tdl $(bindir)/tdl
	chmod 555 $(bindir)/tdl
	(cd $(bindir); ln -sf tdl tdla; ln -sf tdl tdll; ln -sf tdl tdld; ln -sf tdl tdlg)
	cp tdl.1 $(man1dir)/tdl.1
	chmod 444 $(man1dir)/tdl.1

