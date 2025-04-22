#
# Makefile -- NSnake makefile
#
# Copyright (c) 2011-2025 David Demelier <markand@malikania.fr>
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

# Build options.
GID :=          games
UID :=          root

# Installation options.
PREFIX :=       /usr/local
BINDIR :=       $(PREFIX)/bin
MANDIR :=       $(PREFIX)/share/man
VARDIR :=       $(PREFIX)/var

VERSION :=      3.0.1
SRCS :=         nsnake.c
OBJS :=         $(SRCS:.c=.o)

OS :=           $(shell uname -s)

# Overrides for some OSes.
ifeq ($(OS),FreeBSD)
INCS :=         -I/usr/local/include
LIBS :=         -L/usr/local/lib -lncurses
else ifeq ($(OS),DragonFly)
# Has pkg-config for ncurses, but seems broken.
INCS :=         -I/usr/local/include -I/usr/local/include/ncurses
LIBS :=         -L/usr/local/lib -lncurses
else ifeq ($(OS),OpenBSD)
# OpenBSD has no games UID
LIBS :=         -lncurses
MANDIR :=       $(PREFIX)/man
else
# Other linux, expect pkg-config files.
INCS :=         $(shell pkg-config --cflags ncurses)
LIBS :=         $(shell pkg-config --libs ncurses)
endif

override CPPFLAGS += -DVARDIR=\"$(VARDIR)\"
override CPPFLAGS += -DVERSION=\"$(VERSION)\"

override CFLAGS += $(INCS)

override LDLIBS += $(LIBS)

all: nsnake

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	cp nsnake $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man6
	cp nsnake.6 $(DESTDIR)$(MANDIR)/man6
	mkdir -p $(DESTDIR)$(VARDIR)/db/nsnake
	chown $(UID):$(GID) $(DESTDIR)$(VARDIR)/db/nsnake
	chmod 770 $(DESTDIR)$(VARDIR)/db/nsnake

clean:
	rm -f nsnake $(OBJS)

.PHONY: all clean install
