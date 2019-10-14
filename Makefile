#
# Makefile -- NSnake makefile
#
# Copyright (c) 2011-2019 David Demelier <markand@malikania.fr>
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

include config.mk

VERSION=        2.2.0
SRCS=           nsnake.c
OBJS=           ${SRCS:.c=.o}

.SUFFIXES:
.SUFFIXES: .c .o

all: nsnake

.c.o:
	${CC} -c -DVARDIR=\"${VARDIR}\" ${PORTCFLAGS} ${CFLAGS} $<

${OBJS}: config.mk sysconfig.h

sysconfig.h: sysconfig.sh
	CC="${CC}" CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" ./sysconfig.sh > $@

nsnake: ${OBJS}
	${CC} -o $@ ${LDFLAGS} ${OBJS} ${LIBS}

install: nsnake
	install -Dm2555 -g ${GID} -o ${UID} nsnake ${DESTDIR}${BINDIR}/nsnake
	install -Dm0644 nsnake.6 ${DESTDIR}${MANDIR}/man6/nsnake.6
	install -dm0770 -g ${GID} -o ${UID} ${DESTDIR}${VARDIR}/db/nsnake

uninstall:
	rm -f ${DESTDIR}${BINDIR}/nsnake
	rm -f ${DESTDIR}${MANDIR}/man6/nsnake.6

clean:
	rm -f ${OBJS} nsnake sysconfig.h nsnake-${VERSION}.tar.xz

dist: clean
	mkdir nsnake-${VERSION}
	cp -R extern nsnake-${VERSION}
	cp CHANGES.md INSTALL.md LICENSE.md README.md nsnake-${VERSION}
	cp Makefile config.mk nsnake.6 nsnake.c sysconfig.sh nsnake-${VERSION}
	tar -cJf nsnake-${VERSION}.tar.xz nsnake-${VERSION}
	rm -rf nsnake-${VERSION}

.PHONY: all clean dist install uninstall
