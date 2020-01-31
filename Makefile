#
# Makefile -- NSnake makefile
#
# Copyright (c) 2011-2020 David Demelier <markand@malikania.fr>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

.POSIX:

# Build options.
CC=             cc
CFLAGS=         -std=c11 -pedantic -O3 -D_XOPEN_SOURCE=700
LDFLAGS=        -lncurses
GID=            games
UID=            games

# Installation options.
PREFIX=         /usr/local
BINDIR=         ${PREFIX}/bin
MANDIR=         ${PREFIX}/share/man
VARDIR=         ${PREFIX}/var

VERSION=        3.0.0
SRCS=           nsnake.c
OBJS=           ${SRCS:.c=.o}

.SUFFIXES:
.SUFFIXES: .c .o

all: nsnake

.c.o:
	${CC} -c -DVARDIR=\"${VARDIR}\" -DVERSION=\"${VERSION}\" ${CFLAGS} $<

nsnake: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS} ${LIBS}

install: nsnake
	mkdir -p ${DESTDIR}${BINDIR}
	cp nsnake ${DESTDIR}${BINDIR}
	chown ${GID}:${UID} ${DESTDIR}${BINDIR}/nsnake
	chmod 2555 ${DESTDIR}${BINDIR}/nsnake
	mkdir -p ${DESTDIR}${MANDIR}/man6
	cp nsnake.6 ${DESTDIR}${MANDIR}/man6
	mkdir -p ${DESTDIR}${VARDIR}/db/nsnake
	chmod 770 ${DESTDIR}${VARDIR}/db/nsnake
	chown ${GID}:${UID} ${DESTDIR}${VARDIR}/db/nsnake

uninstall:
	rm -f ${DESTDIR}${BINDIR}/nsnake
	rm -f ${DESTDIR}${MANDIR}/man6/nsnake.6

clean:
	rm -f ${OBJS} nsnake nsnake-${VERSION}.tar.xz

dist: clean
	mkdir nsnake-${VERSION}
	cp CHANGES.md INSTALL.md LICENSE.md README.md nsnake-${VERSION}
	cp Makefile nsnake.6 nsnake.c nsnake-${VERSION}
	tar -cJf nsnake-${VERSION}.tar.xz nsnake-${VERSION}
	rm -rf nsnake-${VERSION}

.PHONY: all clean dist install uninstall
