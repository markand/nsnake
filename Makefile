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

CC=             gcc
CFLAGS=         -Wall -Wextra -pedantic
LDFLAGS=        -lncurses
GID=            games
UID=            games

PREFIX=         /usr/local
BINDIR=         ${PREFIX}/bin
MANDIR=         ${PREFIX}/share/man
VARDIR=         ${PREFIX}/var/db

SRCS=           nsnake.c
OBJS=           ${SRCS:.c=.o}

all: nsnake

.c.o:
	${CC} -c -DVARDIR=\"${VARDIR}\" ${CFLAGS} $<

${OBJS}: nsnake.h

nsnake: ${OBJS}
	${CC} -o $@ ${LDFLAGS} ${OBJS}

install: nsnake
	install -Dm2555 -g ${GID} -o ${UID} nsnake ${DESTDIR}${BINDIR}/nsnake
	install -Dm0644 nsnake.6 ${DESTDIR}${MANDIR}/man6/nsnake.6
	install -dm0770 -g ${GID} -o ${UID} ${DESTDIR}${VARDIR}/nsnake

uninstall:
	rm -f ${DESTDIR}${BINDIR}/nsnake
	rm -f ${DESTDIR}${MANDIR}/man6/nsnake.6

clean:
	rm -f ${OBJS} nsnake

.PHONY: all clean install uninstall
