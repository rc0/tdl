#  $Header: /cvs/src/tdl/Attic/Makefile,v 1.9 2002/05/09 23:05:08 richard Exp $
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

#######################################################################
# Select C compiler and compile options
CC=gcc
CFLAGS=-g -Wall
#CFLAGS=-O2 -Wall

#######################################################################
# Readline support (including completion) for interactive mode.
# 
# Comment out the following 2 lines if you don't want to or can't use the
# readline library.
# If your readline library is in a non-standard location, you may need to use
# the 2nd pair of lines and edit accordingly.
INC_READLINE=-DUSE_READLINE
LIB_READLINE=-lreadline -lhistory -ltermcap
#INC_READLINE=-DUSE_READLINE -I/usr/include
#LIB_READLINE=-lreadline -lhistory -ltermcap -L/usr/lib

#######################################################################
# Edit this to change the directory to which the software is installed.
prefix=/usr/local

#######################################################################
# If you're generating a package, you may want to use
# 	make DESTDIR=temporary_dir install
# to get the software installed to a directory where you can create
# a tdl.tar.gz from it
DESTDIR=

#######################################################################
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW HERE

pprefix=$(DESTDIR)$(prefix)
bindir=$(pprefix)/bin
mandir=$(pprefix)/man
man1dir=$(pprefix)/man/man1

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
	[ -d $(pprefix) ] || mkdir -p $(pprefix)
	[ -d $(bindir) ] || mkdir $(bindir)
	[ -d $(mandir) ] || mkdir $(mandir)
	[ -d $(man1dir) ] || mkdir $(man1dir)
	cp tdl $(bindir)/tdl
	chmod 555 $(bindir)/tdl
	(cd $(bindir); ln -sf tdl tdla; ln -sf tdl tdll; ln -sf tdl tdld; ln -sf tdl tdlg)
	cp tdl.1 $(man1dir)/tdl.1
	chmod 444 $(man1dir)/tdl.1

