#
# Copyright 2006 Inventive Technology GmbH
# 
# This file is part of tapecat.
#
# tapecat is free software; you can redistribute it and/or modify it under the terms of the GNU 
# General Public License as published by the Free Software Foundation; version 2 of the License.
#
# tapecat is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with tapecat; if not,
# write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
# MA 02110-1301, USA 
#
CC=gcc
CFLAGS=-Wall -ansi
BINDIR=/usr/local/bin
MANDIR=/usr/local/man

all: tapecat
	
tapecat: tapecat.c cmdline.o debug.o

cmdline.o: cmdline.c tapecat.h debug.h

debug.o: debug.c debug.h

distclean: clean

clean:
	-rm *.o
	-rm tapecat
	
install: tapecat
	@if [ -d ${BINDIR} ] && [ -d ${MANDIR} ];\
	then\
	  cp tapecat ${BINDIR}/tapecat &&\
	  chmod 755 ${BINDIR}/tapecat &&\
	  cp tapecat.1 ${MANDIR}/man1/tapecat.1 &&\
	  chmod 644 ${MANDIR}/man1/tapecat.1 &&\
	  echo "bindir is ${BINDIR}" &&\
	  echo "mandir is ${MANDIR}" && \
	  echo "Install succeeded!";\
	else\
	  echo "bindir is ${BINDIR}" &&\
	  echo "mandir is ${MANDIR}" &&\
	  echo "Install directories not found!";\
	fi

uninstall:
	@if [ -e ${BINDIR}/tapecat ];\
	then\
	  rm ${BINDIR}/tapecat;\
	fi
	@if [ -e ${MANDIR}/man1/tapecat.1 ];\
	then\
	  rm ${MANDIR}/man1/tapecat.1;\
	fi
	@echo "bindir is ${BINDIR}"
	@echo "mandir is ${MANDIR}"
	 echo "Uninstall succeeded!"
