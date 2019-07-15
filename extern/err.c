/*
 * err.c -- formtted error messages (portable version)
 *
 * Copyright (c) 2011-2018 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "err.h"

/*
 * These functions implements at least the same functions that can be found
 * in the NetBSD err(3) man page without printing the programe name due to
 * a portability issue.
 */

void
err(int val, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verr(val, fmt, ap);
	va_end(ap);
}

void
verr(int val, const char *fmt, va_list ap)
{
	if (fmt) {
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, ": ");
	}

	fprintf(stderr, "%s\n", strerror(errno));
	exit(val);
}

void
errx(int val, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verrx(val, fmt, ap);
	va_end(ap);
}

void
verrx(int val, const char *fmt, va_list ap)
{
	if (fmt)
		vfprintf(stderr, fmt, ap);

	fprintf(stderr, "\n");

	exit(val);
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}

void
vwarn(const char *fmt, va_list ap)
{
	if (fmt) {
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, ": ");
	}

	fprintf(stderr, "%s\n", strerror(errno));
}

void
warnx(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
}

void
vwarnx(const char *fmt, va_list ap)
{
	if (fmt)
		vfprintf(stderr, fmt, ap);

	fprintf(stderr, "\n");
}
