/*
 * nsnake.c -- a snake game for your terminal
 *
 * Copyright (c) 2011-2021 David Demelier <markand@malikania.fr>
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

#if defined(__OpenBSD__)
#	define _BSD_SOURCE
#endif

#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <curses.h>

#define HEIGHT                  23
#define WIDTH                   78
#define SIZE                    ((HEIGHT - 2) * (WIDTH - 2))

#if !defined(VARDIR)
#define VARDIR                  "/var"
#endif

#define DATABASE                VARDIR "/db/nsnake/scores.txt"
#define DATABASE_WC             VARDIR "/db/nsnake/scores-wc.txt"

/* Maximum number of scores allowed in database. */
#define SCORES_MAX              10

/* Calculated from the title dimensions. */
#define TITLE_WIDTH             62
#define TITLE_HEIGHT            13

/* Frame size where scores list is written. */
#define SCORE_FRAME_WIDTH       60
#define SCORE_FRAME_HEIGHT      12

static struct {
	int score;              /* User score. */
	int length;             /* Snake's length. */
	int dirx;               /* Direction in x could be 0, 1 or -1. */
	int diry;               /* Same for y. */
	int paused;             /* Game is paused */

	struct {
		int x;          /* Snake slices in x. */
		int y;          /* Same for y. */
	} pos[SIZE];
} snake;

static struct {
	enum {
		FOOD_NORM = 0,  /* Increase snake's length */
		FOOD_FREE       /* Don't increase snake's length */
	} type;

	int x;                  /* Position of food in x. */
	int y;                  /* Same for y. */
} food;

struct score {
	char name[16 + 1];      /* User name. */
	int score;              /* Score. */
	long long int time;     /* Timestamp. */
};

static struct {
	int color;              /* Color: 0-8 and -1 to disable. */
	int warp;               /* Enable warp (wall-crossing). */
	int quick;              /* Don't save scores. */
} options = {
	.color  = 4,
	.warp   = 1,
	.quick  = 0,
};

static struct {
	WINDOW *top;
	WINDOW *frame;
	const char *title[10];
} menu_view = {
	.title = {
		"111111111  111111  11  1111111111  11   11111  111111111",
		"11         11  11  11  11      11  11    11    11",
		"11         11  11  11  11      11  11    11    11",
		"11         11  11  11  11      11  11    11    11",
		"111111111  11  11  11  1111111111  1111111     111111111",
		"       11  11  11  11  11      11  11    11    11",
		"       11  11  11  11  11      11  11    11    11",
		"       11  11  11  11  11      11  11    11    11",
		"111111111  11  111111  11      11  11   11111  111111111",
		NULL
	}
};

static struct {
	WINDOW *top;
	WINDOW *frame;
} game_view;

static struct {
	WINDOW *top;
	WINDOW *frame;
} score_view;

static const char *     name(void);
static noreturn void    die(int, const char *, ...);
static void             set(WINDOW *, int);
static void             unset(WINDOW *, int);
static int              is_snake(int, int);
static int              is_wall(int, int);
static int              is_dead(void);
static int              is_eaten(void);
static void             prepare(void);
static void             spawn(void);
static void             rotate(int);
static void             input(void);
static void             update(void);
static void             draw(void);
static void             delay(unsigned int);
static const char *     scores_path(void);
static int              scores_read(struct score []);
static int              scores_write(const struct score []);
static int              scores_register(void);
static void             scores_show(void);
static void             state_menu(void);
static void             state_run(void);
static void             state_score(void);
static void             state_score_exit(void);
static void             init(void);
static void             quit(void);
static noreturn void    usage(void);

void (*state)(void) = &(state_menu);

static const char *
name(void)
{
	struct passwd *pw = getpwuid(getuid());

	return pw ? pw->pw_name : "unknown";
}

static noreturn void
die(int sys, const char *fmt, ...)
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
set(WINDOW *frame, int pair)
{
	if (options.color >= 0)
		wattron(frame, pair);
}

static void
unset(WINDOW *frame, int pair)
{
	if (options.color >= 0)
		wattroff(frame, pair);
}

static int
is_snake(int x, int y)
{
	/*
	 * Check if the coordinate is within the snake part we start at 1
	 * because head should not be considered to avoid `is_dead' to return
	 * 1 on snake's head.
	 */
	for (int i = 1; i < snake.length; ++i)
		if (snake.pos[i].x == x && snake.pos[i].y == y)
			return 1;

	return 0;
}

static int
is_wall(int x, int y)
{
	return x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1;
}

static int
is_dead(void)
{
	/* Head on body. */
	if (is_snake(snake.pos[0].x, snake.pos[0].y))
		return 1;

	/* Head on wall. */
	if (!options.warp)
		return is_wall(snake.pos[0].x, snake.pos[0].y);

	return 0;
}

static int
is_eaten(void)
{
	return snake.pos[0].x == food.x && snake.pos[0].y == food.y;
}

static void
prepare(void)
{
	/* Disable blocking mode. */
	nodelay(stdscr, 1);
	clear();
	werase(game_view.frame);

	/* Prepare snake structure. */
	memset(&snake, 0, sizeof (snake));
	snake.score  = 0,
	snake.length = 4,
	snake.dirx   = 1,
	snake.diry   = 0,
	snake.pos[0].x = 10;
	snake.pos[0].y = 5;
	snake.pos[1].x = 9;
	snake.pos[1].y = 5;
	snake.pos[2].x = 8;
	snake.pos[2].y = 5;
	snake.pos[3].x = 7;
	snake.pos[3].y = 5;

	spawn();
}

static void
spawn(void)
{
	int num;

	do {
		food.x = (random() % (WIDTH - 2)) + 1;
		food.y = (random() % (HEIGHT - 2)) + 1;
	} while (is_snake(food.x, food.y));

	/* Free food does not grow the snake */
	num = ((random() % 7) - 1) + 1;
	food.type = (num == 6) ? FOOD_FREE : FOOD_NORM;
}

static void
rotate(int ch)
{
	switch (ch) {
	case KEY_LEFT:
	case 'h':
	case 'H':
		if (snake.dirx != 1) {
			snake.dirx = -1;
			snake.diry = 0;
		}

		break;
	case KEY_UP:
	case 'k':
	case 'K':
		if (snake.diry != 1) {
			snake.dirx = 0;
			snake.diry = -1;
		}

		break;
	case KEY_DOWN:
	case 'j':
	case 'J':
		if (snake.diry != -1) {
			snake.dirx = 0;
			snake.diry = 1;
		}

		break;
	case KEY_RIGHT:
	case 'l':
	case 'L':
		if (snake.dirx != -1) {
			snake.dirx = 1;
			snake.diry = 0;
		}

		break;
	default:
		break;
	}
}

static void
input(void)
{
	int ch;

	switch ((ch = getch())) {
	case 'p':
		nodelay(stdscr, 0);
		snake.paused = 1;
		break;
	case 'q':
		/* Create game over. */
		snake.pos[0].x = snake.pos[2].x;
		snake.pos[0].y = snake.pos[2].y;
		break;
	case 'c':
		options.color = (options.color + 1) % 8;
		break;
	default:
		if (snake.paused) {
			nodelay(stdscr, 1);
			snake.paused = 0;
		}

		rotate(ch);
		break;
	}
}

static void
update(void)
{
	/* Move each part of the snake to the new position. */
	memmove(&snake.pos[1], &snake.pos[0],
	    sizeof (snake.pos) - sizeof (snake.pos[0]));

	if (is_eaten()) {
		/* Only grow snake for non-free food. */
		if (food.type == FOOD_NORM)
			snake.length += 2;

		/* If the screen is totally filled */
		if (snake.length >= SIZE)
			snake.length = 4;

		spawn();
		snake.score += 1;
	}

	/* Go to the next position */
	snake.pos[0].x += snake.dirx;
	snake.pos[0].y += snake.diry;

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
}

static void
draw(void)
{
	werase(game_view.top);
	werase(game_view.frame);

	for (int i = 0; i < snake.length; ++i) {
		set(game_view.frame, COLOR_PAIR(options.color + 3));
		mvwaddch(game_view.frame, snake.pos[i].y, snake.pos[i].x, '#');
		unset(game_view.frame, COLOR_PAIR(options.color + 3));
	}

	/* Print head */
	set(game_view.frame, COLOR_PAIR(options.color + 3) | A_BOLD);
	mvwaddch(game_view.frame, snake.pos[0].y, snake.pos[0].x, '@');
	unset(game_view.frame, COLOR_PAIR(options.color + 3) | A_BOLD);

	/* Erase the snake's tail */
	mvwaddch(game_view.frame, snake.pos[snake.length].y, snake.pos[snake.length].x, ' ');

	/* Print food */
	set(game_view.frame, COLOR_PAIR(1) | A_BOLD);
	mvwaddch(game_view.frame, food.y, food.x, (food.type == FOOD_FREE) ? '*' : '+');
	unset(game_view.frame, COLOR_PAIR(1) | A_BOLD);

	/* Print paused if necessary. */
	if (snake.paused) {
		mvwprintw(game_view.frame, HEIGHT / 2, (WIDTH / 2) - 3, "PAUSE");
		mvwprintw(game_view.frame, (HEIGHT / 2) + 1, (WIDTH / 2) - 12, "Press any key to resume");
	}

	/* Print score */
	wmove(game_view.top, 0, 0);
	wprintw(game_view.top, "Score: %d", snake.score, snake.dirx, snake.diry);
	box(game_view.frame, ACS_VLINE, ACS_HLINE);
}

static void
delay(unsigned int ms)
{
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = ms * 1000000
	};

	while (nanosleep(&ts, &ts));
}

static const char *
scores_path(void)
{
	return options.warp ? DATABASE_WC : DATABASE;
}

static int
scores_read(struct score scores[SCORES_MAX])
{
	FILE *fp;

	if (!(fp = fopen(scores_path(), "r")))
		return -1;

	for (struct score *s = scores; s != &scores[SCORES_MAX]; ) {
		int ret = fscanf(fp, "%16[^\\|]|%d|%lld\n", s->name,
		    &s->score, &s->time);

		if (ret == EOF)
			break;
		if (ret == 3)
			s++;
	}

	fclose(fp);

	return 0;
}

static int
scores_write(const struct score scores[SCORES_MAX])
{
	FILE *fp;

	if (!(fp = fopen(scores_path(), "w")))
		return 0;

	for (const struct score *s = scores; s != &scores[SCORES_MAX] && s->name[0]; ++s)
		fprintf(fp, "%s|%d|%lld\n", s->name, s->score, s->time);

	fclose(fp);

	return 1;
}

static int
scores_register(void)
{
	struct score scores[SCORES_MAX] = {0};
	struct score *s;

	/* Do not test result, file may not exist yet. */
	scores_read(scores);

	for (s = scores; s != &scores[SCORES_MAX]; ++s)
		if (snake.score >= s->score)
			break;

	/* Not in top list. */
	if (s == &scores[SCORES_MAX])
		return -1;

	/* Move the current score index to the next one. */
	memmove(&s[1], &s[0], sizeof (struct score) * (SCORES_MAX - (&s[1] - scores)));
	strncpy(s->name, name(), sizeof (s->name));
	s->score = snake.score;
	s->time = time(NULL);

	return scores_write(scores);
}

static void
scores_show(void)
{
	struct score scores[SCORES_MAX] = {0};

	if (scores_read(scores) < 0)
		die(1, "could not open scores");

	for (struct score *s = scores; s != &scores[SCORES_MAX]; ++s) {
		if (s->name[0]) {
			char date[128] = {0};
			time_t time = s->time;
			struct tm *tm = localtime(&time);

			strftime(date, sizeof (date), "%c", tm);
			printf("%-16s%-10u %s\n", s->name, s->score, date);
		}
	}
}

static void
state_menu(void)
{
	/* We don't need non-blocking mode here. */
	nodelay(stdscr, 0);
	clear();

	/* Top bar. */
	mvwprintw(menu_view.top, 0, 0, "NSnake " VERSION);
	box(menu_view.frame, ACS_VLINE, ACS_HLINE);

	/* Print title. */
	for (int row = 0; menu_view.title[row]; ++row) {
		wmove(menu_view.frame, row + 2, 3);

		for (const char *p = menu_view.title[row]; *p; ++p) {
			if (*p == '1') {
				if (options.color >= 0) {
					set(menu_view.frame, COLOR_PAIR(2));
					waddch(menu_view.frame, ' ');
					unset(menu_view.frame, COLOR_PAIR(2));
				} else
					waddch(menu_view.frame, '.');
			} else
				waddch(menu_view.frame, ' ');
		}
	}

	/* Print menu actions. */
	const int cx = (COLS / 2) - 14;
	const int cy = (LINES / 2) + (TITLE_HEIGHT / 2) - 3;

	mvprintw(cy + 2, cx, "Hit <Return> to play the game");
	mvprintw(cy + 3, cx, "Hit <n> to %s scoring",
	    options.quick ? "enable" : "disable");
	mvprintw(cy + 4, cx, "Hit <w> to %s wall-crossing",
	    options.warp ? "disable" : "enable");
	mvprintw(cy + 5, cx, "Hit <s> to show scores");
	mvprintw(cy + 6, cx, "Hit <q> to quit");
	refresh();

	wrefresh(menu_view.top);
	wrefresh(menu_view.frame);

	/* Get command. */
	switch (getch()) {
	case '\n':
		state = &(state_run);
		break;
	case 's':
		state = &(state_score);
		break;
	case 'n':
		options.quick = !options.quick;
		break;
	case 'w':
		options.warp = !options.warp;
		break;
	case 'q':
		state = NULL;
		break;
	default:
		break;
	}
}

static void
state_run(void)
{
	prepare();

	while (!is_dead()) {
		input();

		/* Dead state can happen here depending on input. */
		if (is_dead())
			break;
		if (!snake.paused)
			update();

		draw();
		wrefresh(game_view.top);
		wrefresh(game_view.frame);
		delay(snake.diry ? 118 : 100);
	}

	/* Register score only if wanted. */
	if (!options.quick)
		scores_register();

	nodelay(stdscr, 0);

	do {
		/* Show game over in the middle. */
		mvwprintw(game_view.frame, HEIGHT / 2, (WIDTH / 2) - 5,
		    "GAME OVER");
		mvwprintw(game_view.frame, (HEIGHT / 2) + 1, (WIDTH / 2) - 19,
		    "Press <Return> to return to main menu");
		wrefresh(game_view.top);
		wrefresh(game_view.frame);
	} while (getch() != '\n');

	state = (&state_menu);
}

static void
state_score(void)
{
	struct score scores[SCORES_MAX] = {0};
	int y = 1;

	erase();
	werase(score_view.frame);
	box(score_view.frame, ACS_VLINE, ACS_HLINE);

	/* Ignore result, we print an empty list. */
	scores_read(scores);

	/* Top bar. */
	mvwprintw(score_view.top, 0, 0, "Top scores");

	/*
	 * This is the available space for each line:
	 *
	 * nnnnnnnnnnnnnnnn ssssssssssssssssssssssssssss yyyy-mm-dd
	 */
	for (struct score *s = scores; s != &scores[SCORES_MAX]; ++s) {
		if (!s->name[0])
			break;

		char date[10 + 1];
		time_t timestamp = s->time;
		struct tm *tm = localtime(&timestamp);

		strftime(date, sizeof (date), "%Y-%m-%d", tm);
		mvwprintw(score_view.frame, y++, 2, "%-16s %-28d %-10s",
		    s->name, s->score, date);
	}

	mvprintw((LINES / 2) + (SCORE_FRAME_HEIGHT / 2) + 1, (COLS / 2) - 17,
	    "Type any key to return to main menu");

	refresh();
	wrefresh(score_view.top);
	wrefresh(score_view.frame);

	/* Return to menu on any key press. */
	getch();
	state = &(state_menu);
}

static void
state_score_exit(void)
{
	scores_show();
	state = NULL;
}

static void
init(void)
{
	srandom((unsigned int)time(NULL));

	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);

	if (COLS < (WIDTH + 1) || LINES < (HEIGHT + 1))
		die(0, "abort: terminal too small");

	if (options.color > 8)
		options.color = 4;
	else if (options.color >= 0)
		options.color += 2;

	if (options.color >= 0 && has_colors()) {
		use_default_colors();
		start_color();

		init_pair(0, COLOR_WHITE, COLOR_BLACK); /* Bar */
		init_pair(1, COLOR_YELLOW, -1);         /* Food */
		init_pair(2, -1, COLOR_CYAN);           /* Title */

		for (int i = 0; i < COLORS; ++i)
			init_pair(i + 3, i, -1);
	}

	/* Init game view. */
	game_view.top = newwin(1, 0, 0, 0);
	game_view.frame = newwin(HEIGHT, WIDTH, (LINES / 2) - (HEIGHT / 2),
	    (COLS / 2) - (WIDTH / 2));

	if (options.color >= 0) {
		wbkgd(game_view.top, COLOR_PAIR(0));
		wattrset(game_view.top, COLOR_PAIR(0) | A_BOLD);
	}

	/* Init menu view. */
	menu_view.top = newwin(1, 0, 0, 0);
	menu_view.frame = newwin(TITLE_HEIGHT, TITLE_WIDTH,
	    (LINES / 2) - (TITLE_HEIGHT / 2) - 3,
	    (COLS  / 2) - (TITLE_WIDTH / 2));

	/* Init score view. */
	score_view.top = newwin(1, 0, 0, 0);
	score_view.frame = newwin(SCORE_FRAME_HEIGHT, SCORE_FRAME_WIDTH,
	    (LINES / 2) - (SCORE_FRAME_HEIGHT / 2),
	    (COLS  / 2) - (SCORE_FRAME_WIDTH / 2));
}

static void
quit(void)
{
	delwin(score_view.top);
	delwin(score_view.frame);
	delwin(menu_view.top);
	delwin(menu_view.frame);
	delwin(game_view.top);
	delwin(game_view.frame);
	delwin(stdscr);
	endwin();
}

static noreturn void
usage(void)
{
	fprintf(stderr, "usage: nsnake [-cnsw] [-C color]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int ch;

#if defined(__OpenBSD__)
	if (pledge("cpath getpw rpath stdio tty wpath", NULL) < 0)
		die(1, "pledge");
#endif

	while ((ch = getopt(argc, argv, "cC:nsw")) != -1) {
		switch (ch) {
		case 'c':
			options.color = -1;
			break;
		case 'C':
			options.color = atoi(optarg);
			break;
		case 'n':
			options.quick = 1;
			break;
		case 's':
			state = &(state_score_exit);
			break;
		case 'w':
			options.warp = 0;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (state != &state_score_exit)
		init();

	while (state)
		state();

	quit();
}
