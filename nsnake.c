/*
 * nsnake.c -- a snake game for your terminal
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

#include "nsnake.h"

/*
 * Global options and variables.
 * ------------------------------------------------------------------
 */

static int setcolors = 1;       /* enable colors */
static int warp = 1;            /* enable wall crossing */
static int color = 2;           /* green color by default */
static int noscore = 0;         /* do not score */
static int verbose = 0;         /* be verbose */

static uint8_t grid[HEIGHT][WIDTH] = { { GridEmpty } };
static WINDOW *top = NULL;
static WINDOW *frame = NULL;

static void
nsnake_wset(WINDOW *frame, int pair)
{
	if (setcolors)
		wattron(frame, pair);
}

static void
nsnake_wunset(WINDOW *frame, int pair)
{
	if (setcolors)
		wattroff(frame, pair);
}

static void
nsnake_refresh(void)
{
	refresh();
	wrefresh(top);
	wrefresh(frame);
}

static int
nsnake_init(void)
{
	/* Ncurses init */
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

	nsnake_refresh();

	return 1;
}

static void
nsnake_set_grid(struct snake *sn)
{
	uint16_t i;

	for (i = 0; i < sn->length; ++i)
		grid[sn->pos[i].y][sn->pos[i].x] = GridSnake;

	/*
	 * each snake part must follow the last part, pos[0] is head, then
	 * pos[2] takes pos[1] place, pos[3] takes pos[2] and so on.
	 */
	grid[sn->pos[sn->length-1].y][sn->pos[sn->length-1].x] = GridEmpty;
	memmove(&sn->pos[1], &sn->pos[0], sizeof (sn->pos) - sizeof (sn->pos[0]));
}

static void
nsnake_draw(const struct snake *sn, const struct food *fd)
{
	uint16_t i;

	for (i = 0; i < sn->length; ++i) {
		nsnake_wset(frame, COLOR_PAIR(color));
		mvwaddch(frame, sn->pos[i].y, sn->pos[i].x, '#');
		nsnake_wunset(frame, COLOR_PAIR(color));
	}

	/* Print head */
	nsnake_wset(frame, COLOR_PAIR(color) | A_BOLD);
	mvwaddch(frame, sn->pos[0].y, sn->pos[0].x, '@');
	nsnake_wunset(frame, COLOR_PAIR(color) | A_BOLD);

	/* Erase the snake's tail */
	mvwaddch(frame, sn->pos[sn->length].y, sn->pos[sn->length].x, ' ');

	/* Print food */
	nsnake_wset(frame, COLOR_PAIR(1) | A_BOLD);
	mvwaddch(frame, fd->y, fd->x, (fd->type == FREE) ? '*' : '+');
	nsnake_wunset(frame, COLOR_PAIR(1) | A_BOLD);

	/* Print score */
	wmove(top, 0, 0);
	wprintw(top, "Score : %d", sn->score);
	nsnake_refresh();
}

static int
nsnake_is_dead(const struct snake *sn)
{
	if (grid[sn->pos[0].y][sn->pos[0].x] == GridSnake)
		return 1;

	/* No warp enabled means dead in wall */
	return (!warp && grid[sn->pos[0].y][sn->pos[0].x] == GridWall);
}

static int
nsnake_is_eaten(const struct snake *sn)
{
	return grid[sn->pos[0].y][sn->pos[0].x] == GridFood;
}

static void
nsnake_spawn(struct food *fd)
{
	int num;

	do {
		fd->x = (random() % (WIDTH - 2)) + 1;
		fd->y = (random() % (HEIGHT - 2)) + 1;
	} while (grid[fd->y][fd->x] != GridEmpty);

	/* Free food does not grow the snake */
	num = ((random() % 7) - 1) + 1;
	fd->type = (num == 6) ? FREE : NORM;
}

static void
nsnake_direction(struct snake *sn, int ch)
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

static int
nsnake_append_score(const struct score *sc)
{
	FILE *fp;
	char header[12] = "nsnake-score";
	uint32_t nscore = 1;

	if (!(fp = fopen(NSNAKE_SCOREFILE, "w+b")))
		return 0;

	fwrite(header, sizeof (header), 1, fp);
	fwrite(&nscore, sizeof (nscore), 1, fp);
	fwrite(sc, sizeof (*sc), 1, fp);
	fclose(fp);

	return 1;
}

static int
nsnake_insert_score(const struct score *sc)
{
	FILE *fp;
	uint32_t nscore, i;
	char header[12] = { 0 };
	struct score *buffer;

	if (!(fp = fopen(NSNAKE_SCOREFILE, "r+b")))
		return 0;

	fread(header, sizeof (header), 1, fp);
	if (strncmp(header, "nsnake-score", sizeof (header)) != 0) {
		fclose(fp);
		return 0;
	}

	fread(&nscore, sizeof (nscore), 1, fp);
	if (!(buffer = calloc(nscore + 1, sizeof (*buffer)))) {
		fclose(fp);
		return 0;
	}

	fread(buffer, sizeof (*buffer), nscore, fp);
	for (i = 0; i < nscore; ++i)
		if (sc->score >= buffer[i].score && sc->wc == buffer[i].wc)
			break;

	/* Replace same score */
	if (sc->score == buffer[i].score && strcmp(buffer[i].name, sc->name) == 0)
		memcpy(&buffer[i], sc, sizeof (*sc));
	else {
		memmove(&buffer[i + 1], &buffer[i], (sizeof (*sc)) * (nscore - i));
		memcpy(&buffer[i], sc, sizeof (*sc));

		/* There is now a new entry */
		++ nscore;

		/* Update number of score entries */
		fseek(fp, sizeof (header), SEEK_SET);
		fwrite(&nscore, sizeof (nscore), 1, fp);
	}
		
	/* Finally write */
	fseek(fp, sizeof (header) + sizeof (nscore), SEEK_SET);
	fwrite(buffer, sizeof (*sc), nscore, fp);
	free(buffer);
	fclose (fp);

	return 1;
}

static int
nsnake_register_score(const struct snake *sn)
{
	struct score sc;
	int (*reshandler)(const struct score *);

	memset(&sc, 0, sizeof (sc));

#if defined(_WIN32)
	DWORD length = NAMELEN + 1;
	GetUserNameA(sc.name, &length);
#else
	strncpy(sc.name, getpwuid(getuid())->pw_name, sizeof (sc.name));
#endif
	sc.score = sn->score;
	sc.time = time(NULL);
	sc.wc = warp;

	reshandler = (access(NSNAKE_SCOREFILE, F_OK) == -1) ? &nsnake_append_score : &nsnake_insert_score;

	return reshandler(&sc);
}

void
nsnake_show_scores(void)
{
	FILE *fp;
	uint32_t nscore, i;
	char header[12] = { 0 };
	struct score sc;

	if (!(fp = fopen(NSNAKE_SCOREFILE, "rb")))
		err(1, "Could not read %s", NSNAKE_SCOREFILE);

	if (verbose)
		printf("Wall crossing %s\n", (warp) ? "enabled" : "disabled");

	fread(header, sizeof (header), 1, fp);
	if (strncmp(header, "nsnake-score", sizeof (header)) != 0)
		errx(1, "Not a valid nsnake score file");

	fread(&nscore, sizeof (nscore), 1, fp);
	for (i = 0; i < nscore; ++i) {
		fread(&sc, sizeof (sc), 1, fp);

		if (sc.wc == warp) {
			char date[128] = { 0 };
			struct tm *tm = localtime(&sc.time);

			strftime(date, sizeof (date), "%c", tm);
			printf("%-16s%-10u %s\n", sc.name, sc.score, date);
		}
	}

	fclose(fp);
	exit(0);
}

void
nsnake_wait(unsigned ms)
{
#if defined(_WIN32)
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

static void
nsnake_quit(const struct snake *sn)
{
	uint16_t i;

	if (sn != NULL) {
		for (i = 0; i < sn->length; ++i) {
			mvwaddch(frame, sn->pos[i].y, sn->pos[i].x, ' ');
			nsnake_wait(50);
			nsnake_refresh();
		}
	}

	delwin(top);
	delwin(frame);
	delwin(stdscr);
	endwin();
}

#if defined(HAVE_SIGWINCH)

void
nsnake_resize_handler(int signal)
{
	int x, y;

	if (signal != SIGWINCH)
		return;

	/* XXX: I would like to pause the game until the terminal is resized */
	endwin();

#if defined(HAVE_RESIZETERM)
	resizeterm(LINES, COLS);
#endif
#if defined(HAVE_RESIZE_TERM)
	resize_term(LINES, COLS);
#endif
	refresh(); clear();

	/* Color the top bar */
	wbkgd(top, COLOR_PAIR(0) | A_BOLD);

	getmaxyx(stdscr, y, x);

	if (x < WIDTH || y < HEIGHT) {
		nsnake_quit(NULL);
		errx(1, "Terminal has been resized too small, aborting");
	}

	mvwin(frame, (y / 2) - (HEIGHT / 2), (x / 2) - (WIDTH / 2));
	nsnake_refresh();
}

#endif

static void
nsnake_usage(void)
{
	fprintf(stderr, "usage: nsnake [-cnsvw] [-C color]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int running;
	int x, y, ch;
	int showscore = 0;

	struct snake sn = { 0, 4, 1, 0, {
		{5, 10}, {5, 9}, {5, 8}, {5, 7} }
	};

	struct food fd = { NORM, 0, 0 };

	/* Process options */
	while ((ch = getopt(argc, argv, "cC:nsvw")) != -1) {
		switch (ch) {
		case 'c': setcolors = 0;	break;
		case 'C': color = atoi(optarg); break;
		case 'n': noscore = 1;	  break;
		case 's': showscore = 1;	break;
		case 'v': verbose = 1;	  break;
		case 'w': warp = 0;	 break;
		case '?':
		default :
			nsnake_usage();
			/* NOTREACHED */
		}
	}

	if (showscore)
		nsnake_show_scores();

	argc -= optind;
	argv += optind;

	/* Game init */
	srandom((unsigned)time(NULL));

#if defined(HAVE_SIGWINCH)
	signal(SIGWINCH, nsnake_resize_handler);
#endif

	if (!nsnake_init()) {
		nsnake_quit(NULL);
		errx(1, "Terminal too small, aborting");
	}
		
	if (top == NULL || frame == NULL) {
		endwin();
		errx(1, "ncurses failed to init");
	}

	/* Apply GridWall to the edges */
	for (y = 0; y < HEIGHT; ++y)
		grid[y][0] = grid[y][WIDTH - 1] = GridWall;
	for (x = 0; x < WIDTH; ++x)
		grid[0][x] = grid[HEIGHT - 1][x] = GridWall;

	/* Do not spawn food on snake */
	nsnake_set_grid(&sn);
	nsnake_spawn(&fd);

	/* Apply food on the grid */
	grid[fd.y][fd.x] = GridFood;
	nsnake_draw(&sn, &fd);

	/* First direction is to right */
	sn.pos[0].x += sn.dirx;

	running = 1;
	while (!nsnake_is_dead(&sn) && running) {
		if (nsnake_is_eaten(&sn)) {
			int i;

			if (fd.type == NORM)
				sn.length += 2;

			for (i = 0; i < sn.length; ++i)
				grid[sn.pos[i].y][sn.pos[i].x] = GridSnake;

			/* If the screen is totally filled */
			if (sn.length >= SIZE) {
				/* Emulate new game */
				for (i = 4; i < SIZE; ++i) {
					mvwaddch(frame, sn.pos[i].y, sn.pos[i].x, ' ');
					grid[sn.pos[i].y][sn.pos[i].x] = GridEmpty;
				}

				sn.length = 4;
			}

			if (fd.type == NORM)
				nsnake_set_grid(&sn);

			/* Prevent food spawning on snake's tail */
			nsnake_spawn(&fd);

			sn.score += 1;
			grid[fd.y][fd.x] = GridFood;
		}

		/* Draw and define grid with snake parts */
		nsnake_draw(&sn, &fd);
		nsnake_set_grid(&sn);

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
			nsnake_direction(&sn, ch);

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
			nsnake_wait(118);
		else
			nsnake_wait(100);
	}

	/* The snake is dead. */
	nsnake_quit(&sn);

	/* User has left or is he dead? */
	printf("%sScore: %d\n", (grid[sn.pos[0].y][sn.pos[0].x] != GridEmpty) ?
		"You died...\n" : "", sn.score);

	if (!noscore && !nsnake_register_score(&sn))
		err(1, "Could not write score file %s", NSNAKE_SCOREFILE);

	return 0;
}
