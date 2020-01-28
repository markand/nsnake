#!/bin/sh
#
# sysconfig.sh -- automatically configure portability checks
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

# Define CC, CFLAGS, LDFLAGS before calling this script.

compile()
{
	$CC $CFLAGS -x "c" - -o /dev/null $LDFLAGS > /dev/null 2>&1 && echo $1
}

# resizeterm(3) (ncurses function)
cat << EOF | compile "#define HAVE_RESIZETERM"
#include <curses.h>

int
main(void)
{
	resizeterm(0, 0);
}
EOF

# random/srandom.
cat << EOF | compile "#define HAVE_RANDOM"
#include <stdlib.h>

int
main(void)
{
	srandom(0);
	random();
}
EOF

# getopt(3) function.
cat << EOF | compile "#define HAVE_GETOPT"
#include <unistd.h>

int
main(void)
{
	getopt(0, 0, 0);
}
EOF
