#include "play.h"
#include "position.h"

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

static void make_random_board(struct position *p)
{
	uint8_t perm[2 * PIECES];
	randperm(perm, 2 * PIECES, SIZE * SIZE);
	memcpy(p->white, perm, PIECES);
	memcpy(p->black, perm + PIECES, PIECES);
}

void play(struct player_fo *white, struct player_fo *black)
{
	puts("-------------- NEW GAME -------------");
	struct position p;
	make_random_board(&p);

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

static int user_player_cb(struct player_fo *unused, struct position *p)
{
	(void)unused;

	bool taken[SIZE * SIZE];
	gen_taken(taken, p);
	int v, v_;
	for (;;) {
		puts("Your move? [x y d]");
		int x, y, d, ret;
		ret = scanf("%d %d %d", &x, &y, &d);
		v = x + SIZE * y;
		int ok = 0;
		for (int i = 0; i < PIECES; ++i)
			if (p->white[i] == v)
				ok = 1;
		int x_ = x, y_ = y;
		if (d == 0) x_ += 1;
		if (d == 1) y_ += 1;
		if (d == 2) x_ -= 1;
		if (d == 3) y_ -= 1;
		v_ = x_ + SIZE * y_;
		printf("(%d,%d)->(%d,%d)\n",x,y,x_,y_);
		if (ret == 3 &&
		    x >= 0 && x < SIZE &&
		    y >= 0 && y < SIZE &&
		    ok &&
		    x_ >= 0 && x_ < SIZE &&
		    y_ >= 0 && y_ < SIZE &&
		    !taken[v_])
			break;
		puts("Invalid move!");
		clearerr(stdin);
		if (scanf("%*[^\n]") < 0)
			return -1;
	}
	for (int i = 0; i < PIECES; ++i)
		if (p->white[i] == v)
			p->white[i] = v_;
	sort4(p->white);
	return 0;
}

struct player_fo user_player = { .cb = user_player_cb };
