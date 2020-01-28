/*
 * nsnake.c -- a snake game for your terminal
 *
 * Copyright (c) 2011-2020 David Demelier <markand@malikania.fr>
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

#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>

#include <curses.h>

#if !defined(_WIN32)
#	include <unistd.h>
#	include <sys/types.h>
#	include <pwd.h>
#else
#	include <io.h>
#	include <lmcons.h>
#	include <windows.h>
#endif

#include "sysconfig.h"

#if !defined(HAVE_RANDOM)
#	define random rand
#	define srandom srand
#endif

#if !defined(HAVE_GETOPT)
#	include "extern/getopt.c"
#endif

#define HEIGHT          23
#define WIDTH           78
#define SIZE            ((HEIGHT - 2) * (WIDTH - 2))

#define DATABASE        VARDIR "/db/nsnake/scores.txt"
#define DATABASE_WC     VARDIR "/db/nsnake/scores-wc.txt"

#define SCORES_MAX      10

enum grid {
	GRID_EMPTY,
	GRID_WALL,
	GRID_SNAKE,
	GRID_FOOD
};

struct snake {
	int score;              /* user score */
	int length;             /* snake's size */
	int dirx;               /* direction in x could be 0, 1 or -1 */
	int diry;               /* same for y */

	struct {
		int x;          /* each snake part has (x, y) position */
		int y;          /* each part will be displayed */
	} pos[SIZE];
};

struct food {
	enum {
		NORM = 0,       /* both increase the score but only NORM */
		FREE            /* increase the snake's length too */
	} type;

	int x;                  /* Position of the current food, will be used */
	int y;                  /* in grid[][]. */
};

struct score {
	char name[32];          /* highscore's name */
	int score;              /* score */
	long long time;         /* when? */
};

static bool setcolors = true;   /* enable colors */
static bool warp = true;        /* enable wall crossing */
static int color = 2;           /* green color by default */
static bool noscore = false;    /* do not score */
static bool verbose = true;     /* be verbose */

static int grid[HEIGHT][WIDTH] = {{ GRID_EMPTY }};
static WINDOW *top = NULL;
static WINDOW *frame = NULL;

static const char *
name(void)
{
	struct passwd *pw = getpwuid(getuid());

	return pw ? pw->pw_name : "unknown";
}

static noreturn void
die(bool sys, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (sys)
		fprintf(stderr, ": %s\n", strerror(errno));
	else
		fprintf(stderr, "\n");

	exit(1);
}

static void
wset(WINDOW *frame, int pair)
{
	if (setcolors)
		wattron(frame, pair);
}

static void
wunset(WINDOW *frame, int pair)
{
	if (setcolors)
		wattroff(frame, pair);
}

static void
repaint(void)
{
	refresh();
	wrefresh(top);
	wrefresh(frame);
}

static int
init(void)
{
	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);

	if (COLS < (WIDTH + 1) || LINES < (HEIGHT + 1))
		return -1;

	if (color < 0 || color > 8)
		color = 4;
	else
		color += 2;

	if (setcolors && has_colors()) {
		int i;

		use_default_colors();
		start_color();

		init_pair(0, COLOR_WHITE, COLOR_BLACK); /* topbar */
		init_pair(1, COLOR_YELLOW, -1);         /* food */

		for (i = 0; i < COLORS; ++i)
			init_pair(i + 2, i, -1);
	}

	top = newwin(1, 0, 0, 0);
	frame = newwin(HEIGHT, WIDTH, (LINES/2)-(HEIGHT/2), (COLS/2)-(WIDTH/2));
	box(frame, ACS_VLINE, ACS_HLINE);

	if (setcolors) {
		wbkgd(top, COLOR_PAIR(0));
		wattrset(top, COLOR_PAIR(0) | A_BOLD);
	}

	repaint();

	return 1;
}

static void
set_grid(struct snake *sn)
{
	for (int i = 0; i < sn->length; ++i)
		grid[sn->pos[i].y][sn->pos[i].x] = GRID_SNAKE;

	/*
	 * each snake part must follow the last part, pos[0] is head, then
	 * pos[2] takes pos[1] place, pos[3] takes pos[2] and so on.
	 */
	grid[sn->pos[sn->length - 1].y][sn->pos[sn->length - 1].x] = GRID_EMPTY;
	memmove(&sn->pos[1], &sn->pos[0], sizeof (sn->pos) - sizeof (sn->pos[0]));
}

static void
draw(const struct snake *sn, const struct food *fd)
{
	for (int i = 0; i < sn->length; ++i) {
		wset(frame, COLOR_PAIR(color));
		mvwaddch(frame, sn->pos[i].y, sn->pos[i].x, '#');
		wunset(frame, COLOR_PAIR(color));
	}

	/* Print head */
	wset(frame, COLOR_PAIR(color) | A_BOLD);
	mvwaddch(frame, sn->pos[0].y, sn->pos[0].x, '@');
	wunset(frame, COLOR_PAIR(color) | A_BOLD);

	/* Erase the snake's tail */
	mvwaddch(frame, sn->pos[sn->length].y, sn->pos[sn->length].x, ' ');

	/* Print food */
	wset(frame, COLOR_PAIR(1) | A_BOLD);
	mvwaddch(frame, fd->y, fd->x, (fd->type == FREE) ? '*' : '+');
	wunset(frame, COLOR_PAIR(1) | A_BOLD);

	/* Print score */
	wmove(top, 0, 0);
	wprintw(top, "Score: %d", sn->score);
	repaint();
}

static bool
is_dead(const struct snake *sn)
{
	if (grid[sn->pos[0].y][sn->pos[0].x] == GRID_SNAKE)
		return true;

	/* No warp enabled means dead in wall */
	return !warp && grid[sn->pos[0].y][sn->pos[0].x] == GRID_WALL;
}

static bool
is_eaten(const struct snake *sn)
{
	return grid[sn->pos[0].y][sn->pos[0].x] == GRID_FOOD;
}

static void
spawn(struct food *fd)
{
	int num;

	do {
		fd->x = (random() % (WIDTH - 2)) + 1;
		fd->y = (random() % (HEIGHT - 2)) + 1;
	} while (grid[fd->y][fd->x] != GRID_EMPTY);

	/* Free food does not grow the snake */
	num = ((random() % 7) - 1) + 1;
	fd->type = (num == 6) ? FREE : NORM;
}

static void
direction(struct snake *sn, int ch)
{
	switch (ch) {
	case KEY_LEFT: case 'h': case 'H':
		if (sn->dirx != 1) {
			sn->dirx = -1;
			sn->diry = 0;
		}

		break;
	case KEY_UP: case 'k': case 'K':
		if (sn->diry != 1) {
			sn->dirx = 0;
			sn->diry = -1;
		}

		break;
	case KEY_DOWN: case 'j': case 'J':
		if (sn->diry != -1) {
			sn->dirx = 0;
			sn->diry = 1;
		}

		break;
	case KEY_RIGHT: case 'l': case 'L':
		if (sn->dirx != -1) {
			sn->dirx = 1;
			sn->diry = 0;
		}

		break;
	default:
		break;
	}
}

static const char *
scores_path(void)
{
	return warp ? DATABASE_WC : DATABASE;
}

static bool
scores_read(struct score *scores)
{
	FILE *fp;

	if (!(fp = fopen(scores_path(), "r")))
		return false;

	for (int i = 0; i < SCORES_MAX; ++i) {
		int ret = fscanf(fp, "%31[^\\|]|%d|%lld\n", scores->name,
		    &scores->score, &scores->time);

		if (ret == EOF)
			break;
		if (ret == 3)
			scores++;
	}

	fclose(fp);

	return true;
}

static bool
scores_write(const struct score *scores)
{
	FILE *fp;

	if (!(fp = fopen(scores_path(), "w")))
		return false;

	for (int i = 0; i < SCORES_MAX; ++i)
		fprintf(fp, "%s|%d|%lld\n", scores[i].name, scores[i].score,
		    scores[i].time);

	fclose(fp);

	return true;
}

static bool
scores_register(const struct snake *sn)
{
	struct score scores[SCORES_MAX] = {0};
	struct score *s;

	if (!scores_read(scores))
		return false;

	for (s = scores; s != &scores[SCORES_MAX]; ++s)
		if (sn->score >= s->score)
			break;

	/* Not in top list. */
	if (s == &scores[SCORES_MAX])
		return true;

	strncpy(s->name, name(), sizeof (s->name));
	s->score = sn->score;
	s->time = time(NULL);

	return scores_write(scores);
}

static void
scores_show(void)
{
	struct score scores[SCORES_MAX] = {0};

	if (!scores_read(scores))
		die(true, "could not open scores");

	for (struct score *s = scores; s != &scores[SCORES_MAX]; ++s) {
		if (s->name[0]) {
			char date[128] = {0};
			time_t time = s->time;
			struct tm *tm = localtime(&time);

			strftime(date, sizeof (date), "%c", tm);
			printf("%-16s%-10u %s\n", s->name, s->score, date);
		}
	}

	exit(0);
}

static void
wait(unsigned ms)
{
#if defined(_WIN32)
	Sleep(ms);
#else
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = ms * 1000000
	};

	nanosleep(&ts, NULL);
#endif
}

static void
quit(const struct snake *sn)
{
	if (sn != NULL) {
		for (int i = 0; i < sn->length; ++i) {
			mvwaddch(frame, sn->pos[i].y, sn->pos[i].x, ' ');
			wait(50);
			repaint();
		}
	}

	delwin(top);
	delwin(frame);
	delwin(stdscr);
	endwin();
}

#if defined(HAVE_SIGWINCH)

static void
resize_handler(int signal)
{
	int x, y;

	if (signal != SIGWINCH)
		return;

	/* XXX: I would like to pause the game until the terminal is resized */
	endwin();

#if defined(HAVE_RESIZETERM)
	resizeterm(LINES, COLS);
#endif
	repaint(); clear();

	/* Color the top bar */
	wbkgd(top, COLOR_PAIR(0) | A_BOLD);

	getmaxyx(stdscr, y, x);

	if (x < WIDTH || y < HEIGHT) {
		quit(NULL);
		die(false, "Terminal has been resized too small, aborting");
	}

	mvwin(frame, (y / 2) - (HEIGHT / 2), (x / 2) - (WIDTH / 2));
	repaint();
}

#endif

static void
usage(void)
{
	fprintf(stderr, "usage: nsnake [-cnsvw] [-C color]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int x, y, ch;
	bool running, show_scores = false;

	struct snake sn = { 0, 4, 1, 0, {
		{5, 10}, {5, 9}, {5, 8}, {5, 7} }
	};

	struct food fd = { NORM, 0, 0 };

	while ((ch = getopt(argc, argv, "cC:nsvw")) != -1) {
		switch (ch) {
		case 'c':
			setcolors = false;
			break;
		case 'C':
			color = atoi(optarg);
			break;
		case 'n':
			noscore = true;
			break;
		case 's':
			show_scores = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'w':
			warp = false;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (show_scores)
		scores_show();

	argc -= optind;
	argv += optind;

	srandom((unsigned)time(NULL));

#if defined(HAVE_SIGWINCH)
	signal(SIGWINCH, resize_handler);
#endif

	if (!init()) {
		quit(NULL);
		die(false, "Terminal too small, aborting");
	}

	if (top == NULL || frame == NULL) {
		endwin();
		die(false, "ncurses failed to init");
	}

	/* Apply GRID_WALL to the edges */
	for (y = 0; y < HEIGHT; ++y)
		grid[y][0] = grid[y][WIDTH - 1] = GRID_WALL;
	for (x = 0; x < WIDTH; ++x)
		grid[0][x] = grid[HEIGHT - 1][x] = GRID_WALL;

	/* Do not spawn food on snake */
	set_grid(&sn);
	spawn(&fd);

	/* Apply food on the grid */
	grid[fd.y][fd.x] = GRID_FOOD;
	draw(&sn, &fd);

	/* First direction is to right */
	sn.pos[0].x += sn.dirx;

	running = true;
	while (!is_dead(&sn) && running) {
		if (is_eaten(&sn)) {
			int i;

			if (fd.type == NORM)
				sn.length += 2;

			for (i = 0; i < sn.length; ++i)
				grid[sn.pos[i].y][sn.pos[i].x] = GRID_SNAKE;

			/* If the screen is totally filled */
			if (sn.length >= SIZE) {
				/* Emulate new game */
				for (i = 4; i < SIZE; ++i) {
					mvwaddch(frame, sn.pos[i].y, sn.pos[i].x, ' ');
					grid[sn.pos[i].y][sn.pos[i].x] = GRID_EMPTY;
				}

				sn.length = 4;
			}

			if (fd.type == NORM)
				set_grid(&sn);

			/* Prevent food spawning on snake's tail */
			spawn(&fd);

			sn.score += 1;
			grid[fd.y][fd.x] = GRID_FOOD;
		}

		/* Draw and define grid with snake parts */
		draw(&sn, &fd);
		set_grid(&sn);

		/* Go to the next position */
		sn.pos[0].x += sn.dirx;
		sn.pos[0].y += sn.diry;

		ch = getch();
		if (ch == 'p') {
			nodelay(stdscr, FALSE);
			while ((ch = getch()) != 'p' && ch != 'q')
				;

			if (ch == 'q')
				running = 0;

			nodelay(stdscr, TRUE);
		} else if (ch == 'q')
			running = 0;
		else if (ch == 'c')
			color = (color + 1) % 8;
		else if (ch)
			direction(&sn, ch);

		/* If warp enabled, touching wall cross to the opposite */
		if (warp) {
			if (sn.pos[0].x == WIDTH - 1)
				sn.pos[0].x = 1;
			else if (sn.pos[0].x == 0)
				sn.pos[0].x = WIDTH - 2;
			else if (sn.pos[0].y == HEIGHT - 1)
				sn.pos[0].y = 1;
			else if (sn.pos[0].y == 0)
				sn.pos[0].y = HEIGHT - 2;
		}

		if (sn.diry != 0)
			wait(118);
		else
			wait(100);
	}

	/* The snake is dead. */
	quit(&sn);

	/* User has left or is he dead? */
	printf("%sScore: %d\n", is_dead(&sn) ? "You died...\n" : "", sn.score);

	if (!noscore && !scores_register(&sn))
		die(true, "Could not write score file %s", DATABASE);
}
