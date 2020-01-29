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

#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <curses.h>

#define HEIGHT          23
#define WIDTH           78
#define SIZE            ((HEIGHT - 2) * (WIDTH - 2))

#ifndef VARDIR
#define VARDIR          "/var"
#endif

#define DATABASE        VARDIR "/db/nsnake/scores.txt"
#define DATABASE_WC     VARDIR "/db/nsnake/scores-wc.txt"

#define SCORES_MAX      10

enum grid {
	GRID_EMPTY,
	GRID_WALL,
	GRID_SNAKE,
	GRID_FOOD
};

static struct {
	int score;              /* user score */
	int length;             /* snake's size */
	int dirx;               /* direction in x could be 0, 1 or -1 */
	int diry;               /* same for y */

	struct {
		int x;          /* each snake part has (x, y) position */
		int y;          /* each part will be displayed */
	} pos[SIZE];
} snake = {
	.score  = 0,
	.length = 4,
	.dirx   = 1,
	.diry   = 0,
	.pos    = {
		{ 5, 10 },
		{ 5, 9  },
		{ 5, 8  },
		{ 5, 7  }
	}
};

static struct {
	enum {
		NORM = 0,       /* both increase the score but only NORM */
		FREE            /* increase the snake's length too */
	} type;

	int x;                  /* Position of the current food, will be used */
	int y;                  /* in grid[][]. */
} food;

struct score {
	char name[32];          /* highscore's name */
	int score;              /* score */
	long long time;         /* when? */
};

static struct {
	int color;              /* Color: 0-8 and -1 to disable) */
	bool warp;              /* Enable warp (wall-crossing) */
	bool quick;             /* Don't save scores */
	bool verbose;           /* Be more verbose */
} options = {
	.color  = 2,
	.warp   = true,
	.quick  = false,
	.verbose = false
};

static struct {
	int grid[HEIGHT][WIDTH];
	WINDOW *top;
	WINDOW *frame;
} view = {
	.grid = {{ GRID_EMPTY }}
};

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
	if (options.color >= 0)
		wattron(frame, pair);
}

static void
wunset(WINDOW *frame, int pair)
{
	if (options.color >= 0)
		wattroff(frame, pair);
}

static void
repaint(void)
{
	refresh();
	wrefresh(view.top);
	wrefresh(view.frame);
}

static void
init(void)
{
	srandom((unsigned int)time(NULL));

	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);

	if (COLS < (WIDTH + 1) || LINES < (HEIGHT + 1))
		die(false, "terminal too small");

	if (options.color < 0 || options.color > 8)
		options.color = 4;
	else
		options.color += 2;

	if (options.color >= 0 && has_colors()) {
		use_default_colors();
		start_color();

		init_pair(0, COLOR_WHITE, COLOR_BLACK); /* topbar */
		init_pair(1, COLOR_YELLOW, -1);         /* food */

		for (int i = 0; i < COLORS; ++i)
			init_pair(i + 2, i, -1);
	}

	view.top = newwin(1, 0, 0, 0);
	view.frame = newwin(HEIGHT, WIDTH, (LINES / 2) - (HEIGHT / 2),
	    (COLS / 2) - (WIDTH / 2));
	box(view.frame, ACS_VLINE, ACS_HLINE);

	if (options.color >= 0) {
		wbkgd(view.top, COLOR_PAIR(0));
		wattrset(view.top, COLOR_PAIR(0) | A_BOLD);
	}

	repaint();
}

static void
update_grid(void)
{
	for (int i = 0; i < snake.length; ++i)
		view.grid[snake.pos[i].y][snake.pos[i].x] = GRID_SNAKE;

	/*
	 * each snake part must follow the last part, pos[0] is head, then
	 * pos[2] takes pos[1] place, pos[3] takes pos[2] and so on.
	 */
	view.grid[snake.pos[snake.length - 1].y][snake.pos[snake.length - 1].x] = GRID_EMPTY;
	memmove(&snake.pos[1], &snake.pos[0], sizeof (snake.pos) - sizeof (snake.pos[0]));
}

static void
draw(void)
{
	for (int i = 0; i < snake.length; ++i) {
		wset(view.frame, COLOR_PAIR(options.color));
		mvwaddch(view.frame, snake.pos[i].y, snake.pos[i].x, '#');
		wunset(view.frame, COLOR_PAIR(options.color));
	}

	/* Print head */
	wset(view.frame, COLOR_PAIR(options.color) | A_BOLD);
	mvwaddch(view.frame, snake.pos[0].y, snake.pos[0].x, '@');
	wunset(view.frame, COLOR_PAIR(options.color) | A_BOLD);

	/* Erase the snake's tail */
	mvwaddch(view.frame, snake.pos[snake.length].y, snake.pos[snake.length].x, ' ');

	/* Print food */
	wset(view.frame, COLOR_PAIR(1) | A_BOLD);
	mvwaddch(view.frame, food.y, food.x, (food.type == FREE) ? '*' : '+');
	wunset(view.frame, COLOR_PAIR(1) | A_BOLD);

	/* Print score */
	wmove(view.top, 0, 0);
	wprintw(view.top, "Score: %d", snake.score);
	repaint();
}

static bool
is_dead(void)
{
	if (view.grid[snake.pos[0].y][snake.pos[0].x] == GRID_SNAKE)
		return true;

	/* No warp enabled means dead in wall */
	return !options.warp && view.grid[snake.pos[0].y][snake.pos[0].x] == GRID_WALL;
}

static bool
is_eaten(void)
{
	return view.grid[snake.pos[0].y][snake.pos[0].x] == GRID_FOOD;
}

static void
spawn(void)
{
	int num;

	do {
		food.x = (random() % (WIDTH - 2)) + 1;
		food.y = (random() % (HEIGHT - 2)) + 1;
	} while (view.grid[food.y][food.x] != GRID_EMPTY);

	/* Free food does not grow the snake */
	num = ((random() % 7) - 1) + 1;
	food.type = (num == 6) ? FREE : NORM;

	/* Put on grid. */
	view.grid[food.y][food.x] = GRID_FOOD;
}

static void
rotate(int ch)
{
	switch (ch) {
	case KEY_LEFT: case 'h': case 'H':
		if (snake.dirx != 1) {
			snake.dirx = -1;
			snake.diry = 0;
		}

		break;
	case KEY_UP: case 'k': case 'K':
		if (snake.diry != 1) {
			snake.dirx = 0;
			snake.diry = -1;
		}

		break;
	case KEY_DOWN: case 'j': case 'J':
		if (snake.diry != -1) {
			snake.dirx = 0;
			snake.diry = 1;
		}

		break;
	case KEY_RIGHT: case 'l': case 'L':
		if (snake.dirx != -1) {
			snake.dirx = 1;
			snake.diry = 0;
		}

		break;
	default:
		break;
	}
}

static const char *
scores_path(void)
{
	return options.warp ? DATABASE_WC : DATABASE;
}

static bool
scores_read(struct score scores[SCORES_MAX])
{
	FILE *fp;

	if (!(fp = fopen(scores_path(), "r")))
		return false;

	for (struct score *s = scores; s != &scores[SCORES_MAX]; ) {
		int ret = fscanf(fp, "%31[^\\|]|%d|%lld\n", s->name,
		    &s->score, &s->time);

		if (ret == EOF)
			break;
		if (ret == 3)
			s++;
	}

	fclose(fp);

	return true;
}

static bool
scores_write(const struct score scores[SCORES_MAX])
{
	FILE *fp;

	if (!(fp = fopen(scores_path(), "w")))
		return false;

	for (const struct score *s = scores; s != &scores[SCORES_MAX] && s->name[0]; ++s)
		fprintf(fp, "%s|%d|%lld\n", s->name, s->score, s->time);

	fclose(fp);

	return true;
}

static bool
scores_register(void)
{
	struct score scores[SCORES_MAX] = {0};
	struct score *s;

	if (!scores_read(scores))
		return false;

	for (s = scores; s != &scores[SCORES_MAX]; ++s)
		if (snake.score >= s->score)
			break;

	/* Not in top list. */
	if (s == &scores[SCORES_MAX])
		return true;

	strncpy(s->name, name(), sizeof (s->name));
	s->score = snake.score;
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
wait(unsigned int ms)
{
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = ms * 1000000
	};

	while (nanosleep(&ts, &ts));
}

static void
quit(void)
{
	/* Snake animation being dead. */
	for (int i = 0; i < snake.length; ++i) {
		mvwaddch(view.frame, snake.pos[i].y, snake.pos[i].x, ' ');
		wait(50);
		repaint();
	}

	delwin(view.top);
	delwin(view.frame);
	delwin(stdscr);
	endwin();
}

static noreturn void
usage(void)
{
	fprintf(stderr, "usage: nsnake [-cnsvw] [-C color]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int x, y, ch;
	bool running = true, show_scores = false;

	while ((ch = getopt(argc, argv, "cC:nsvw")) != -1) {
		switch (ch) {
		case 'c':
			options.color = -1;
			break;
		case 'C':
			options.color = atoi(optarg);
			break;
		case 'n':
			options.quick= true;
			break;
		case 's':
			show_scores = true;
			break;
		case 'v':
			options.verbose = true;
			break;
		case 'w':
			options.warp = false;
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

	init();

	/* Apply GRID_WALL to the edges */
	for (y = 0; y < HEIGHT; ++y)
		view.grid[y][0] = view.grid[y][WIDTH - 1] = GRID_WALL;
	for (x = 0; x < WIDTH; ++x)
		view.grid[0][x] = view.grid[HEIGHT - 1][x] = GRID_WALL;

	/* Apply initial grid values */
	update_grid();
	spawn();
	draw();

	/* Move snake head position to avoid losing immediately. */
	snake.pos[0].x += snake.dirx;

	while (!is_dead() && running) {
		int ch;

		if (is_eaten()) {
			if (food.type == NORM)
				snake.length += 2;

			for (int i = 0; i < snake.length; ++i)
				view.grid[snake.pos[i].y][snake.pos[i].x] = GRID_SNAKE;

			/* If the screen is totally filled */
			if (snake.length >= SIZE) {
				/* Emulate new game */
				for (int i = 4; i < SIZE; ++i) {
					mvwaddch(view.frame, snake.pos[i].y, snake.pos[i].x, ' ');
					view.grid[snake.pos[i].y][snake.pos[i].x] = GRID_EMPTY;
				}

				snake.length = 4;
			}

			if (food.type == NORM)
				update_grid();

			spawn();
			snake.score += 1;
		}

		/* Draw and define grid with snake.ke parts */
		draw();
		update_grid();

		/* Go to the next position */
		snake.pos[0].x += snake.dirx;
		snake.pos[0].y += snake.diry;

		switch ((ch = getch())) {
		case 'p':
			nodelay(stdscr, FALSE);
			while ((ch = getch()) != 'p' && ch != 'q')
				;

			if (ch == 'q')
				running = 0;

			nodelay(stdscr, TRUE);
			break;
		case 'q':
			running = 0;
			break;
		case 'c':
			options.color = (options.color + 1) % 8;
			break;
		default:
			rotate(ch);
			break;
		}

		/* If warp enabled, touching wall cross to the opposite */
		if (options.warp) {
			if (snake.pos[0].x == WIDTH - 1)
				snake.pos[0].x = 1;
			else if (snake.pos[0].x == 0)
				snake.pos[0].x = WIDTH - 2;
			else if (snake.pos[0].y == HEIGHT - 1)
				snake.pos[0].y = 1;
			else if (snake.pos[0].y == 0)
				snake.pos[0].y = HEIGHT - 2;
		}

		if (snake.diry != 0)
			wait(118);
		else
			wait(100);
	}

	/* The snake.ke is dead. */
	quit();

	/* User has left or is he dead? */
	printf("%sScore: %d\n", is_dead() ? "You died...\n" : "", snake.score);

	if (!options.quick && !scores_register())
		die(true, "Could not write score file %s", DATABASE);
}
