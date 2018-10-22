/*
 * nsnake.h -- a snake game for your terminal
 *
 * Copyright (c) 2011-2018 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef NSNAKE_H
#define NSNAKE_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <curses.h>

#if defined(_WIN32)
#	include <io.h>
#	include <lmcons.h>
#	include <windows.h>
#else
#	include <sys/types.h>
#	include <pwd.h>
#endif

#include "config.h"

#if defined(HAVE_ERR)
#	include <err.h>
#endif

#if !defined(HAVE_RANDOM)
#	define random rand
#	define srandom srand
#endif

#define HEIGHT          23
#define WIDTH           78
#define SIZE            ((HEIGHT - 2) * (WIDTH - 2))

enum Grid {
	GridEmpty,
	GridWall,
	GridSnake,
	GridFood
};

struct snake {
	uint32_t score;         /* user score */
	uint16_t length;        /* snake's size */
	int8_t dirx;            /* direction in x could be 0, 1 or -1 */
	int8_t diry;            /* same for y */

	struct {
		uint8_t x;      /* each snake part has (x, y) position */
		uint8_t y;      /* each part will be displayed */
	} pos[SIZE];
};

struct food {
	enum {
		NORM = 0,       /* both increase the score but only NORM */
		FREE            /* increase the snake's length too */
	} type;

	uint8_t x;              /* Position of the current food, will be used */
	uint8_t y;              /* in grid[][]. */
};

struct score {
#if defined(_WIN32)
#	define NAMELEN UNLEN
#else
#	define NAMELEN 32
#endif
	char name[NAMELEN + 1]; /* highscore's name */
	uint32_t score;         /* score */
	time_t time;            /* when? */
	uint8_t wc;             /* wallcrossing or not */
};

#if !defined(HAVE_ERR)

#include <errno.h>

static void
err(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "nsnake: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ": %s", strerror(errno));
	va_end(ap);

	exit(code);
}

static void
errx(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "nsnake: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(code);
}

#endif /* _HAVE_ERR */

#endif /* !NSNAKE_H */
