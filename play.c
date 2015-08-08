#include "play.h"
#include "position.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void randperm(uint8_t P[], int N, int M)
{
	int perm[M];
	for (int i = 0; i < M; ++i)
		perm[i] = i;
	for (int i = 0; i < N; ++i) {
		int j = rand() % (M - i);
		P[i] = perm[j];
		perm[j] = perm[M - i - 1];
	}
}

void make_random_board(struct position *p)
{
	do {
		uint8_t perm[2 * PIECES];
		randperm(perm, 2 * PIECES, SIZE * SIZE);
		memcpy(p->white, perm, PIECES);
		sort4(p->white);
		memcpy(p->black, perm + PIECES, PIECES);
		sort4(p->black);
	} while (is_terminal(p->white) || is_terminal(p->black));
}

void make_empty_board(struct position *p)
{
	for (int i = 0; i < PIECES; ++i)
		p->white[i] = p->black[i] = EMPTY;
}

void play(struct player_fo *white, struct player_fo *black)
{
	puts("-------------- NEW GAME -------------");
	struct position p;
	//make_random_board(&p);
	make_empty_board(&p);
	p.state = PS_UNKNOWN;
	p.terminal_distance = INT_MAX;

	for (int rounds = 0; rounds < 32; ++rounds) {
		int ret;

		dump_position(&p);
		ret = white->cb(white, &p);
		if (ret != 0)
			return;

		if (is_terminal(p.white)) {
			dump_position(&p);
			puts("White won!");
			return;
		}

		dump_position(&p);
		swap_position(&p);

		ret = black->cb(black, &p);
		if (ret != 0)
			return;

		swap_position(&p);

		if (is_terminal(p.black)) {
			dump_position(&p);
			puts("Black won!");
			return;
		}
	}
	puts("Too many rounds. Ended in draw.");
}

static int user_player_phase2(struct position *p, int *pv, int *pv_)
{
	bool taken[SIZE * SIZE];
	gen_taken(taken, p);

	puts("Your move? [x y d]");
	int x, y, d, ret;
	ret = scanf("%d %d %d", &x, &y, &d);
	int v = x + SIZE * y;
	int ok = 0;
	for (int i = 0; i < PIECES; ++i)
		if (p->white[i] == v)
			ok = 1;
	int x_ = x, y_ = y;
	if (d == 0) x_ += 1;
	if (d == 1) y_ += 1;
	if (d == 2) x_ -= 1;
	if (d == 3) y_ -= 1;
	int v_ = x_ + SIZE * y_;
	if (ret == 3 &&
	    x >= 0 && x < SIZE &&
	    y >= 0 && y < SIZE &&
	    ok &&
	    x_ >= 0 && x_ < SIZE &&
	    y_ >= 0 && y_ < SIZE &&
	    !taken[v_]) {
		*pv = v;
		*pv_ = v_;
		return 0;
	}
	puts("Invalid move!");
	clearerr(stdin);
	if (scanf("%*[^\n]") < 0)
		exit(-1);

	return -1;
}

static int user_player_phase1(struct position *p, int *pv)
{
	bool taken[SIZE * SIZE];
	gen_taken(taken, p);

	puts("Your move? [x y]");
	int x, y, ret;
	ret = scanf("%d %d", &x, &y);
	int v = x + SIZE * y;
	if (ret == 2 &&
	    x >= 0 && x < SIZE &&
	    y >= 0 && y < SIZE &&
	    !taken[v]) {
		*pv = v;
		return 0;
	}
	puts("Invalid move!");
	clearerr(stdin);
	if (scanf("%*[^\n]") < 0)
		exit(-1);

	return -1;
}

static int user_player_cb(struct player_fo *unused, struct position *p)
{
	(void)unused;

	int ret = 0;
	int v, v_;
	do {
		if (is_phase1_position(p)) {
			ret = user_player_phase1(p, &v_);
			v = EMPTY;
		} else {
			ret = user_player_phase2(p, &v, &v_);
		}
	} while (ret != 0);
	for (int i = 0; i < PIECES; ++i)
		if (p->white[i] == v) {
			p->white[i] = v_;
			break;
		}
	sort4(p->white);
	return 0;
}

struct player_fo user_player = { .cb = user_player_cb };
