#include "position.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct position *make_node(struct position *p);

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

struct position *ai_best;
void ai_play_handler(struct position *p)
{
	//puts("---- checking");
	//printf("ai_best = %p\n", ai_best);
	p = make_node(p);
	//dump_position(p);
	if (p->n_children == 0) {
		// found winning move
		ai_best = p;
		return;
	}

	if (p->n_children > 0) {
		// found undefined move 
		if (!ai_best || ai_best->n_children != 0)
			ai_best = p;
		return;
	}
	if (!ai_best)
		ai_best = p;
}

void ai_play(struct position *p)
{
	/*struct position *n = find_node(p);
	if (!n)
		puts("unknown node");
	else
		printf("n_children = %d\n", n->n_children);*/
	struct position p_;
	memcpy(p_.white, p->black, PIECES);
	memcpy(p_.black, p->white, PIECES);
	ai_best = NULL;
	foreach_child(&p_, ai_play_handler, false);
	if (!ai_best) {
		puts("I failed.");
		exit(-1);
	}
	if (ai_best->n_children == 0)
		puts("I think I'm winning.");
	if (ai_best->n_children == -1)
		puts("I think I've lost.");
	if (ai_best->n_children > 0)
		puts("I am confused.");

	memcpy(p->black, ai_best->black, PIECES);
	//printf("ai_best->n_children = %d\n", ai_best->n_children);
}

void play(void)
{
	puts("-------------- NEW GAME -------------");
	srand(time(0));
	uint8_t perm[2 * PIECES];
	randperm(perm, 2 * PIECES, SIZE * SIZE);
	struct position p = {};
	memcpy(p.white, perm, PIECES);
	memcpy(p.black, perm + PIECES, PIECES);
	dump_position(&p);
	for (;;) {
		bool taken[SIZE * SIZE];
		gen_taken(taken, &p);
		int v, v_;
		for (;;) {
			puts("Your move? [x y d]");
			int x, y, d, ret;
			ret = scanf("%d %d %d", &x, &y, &d);
			v = x + SIZE * y;
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
			    taken[v] &&
			    x_ >= 0 && x_ < SIZE &&
			    y_ >= 0 && y_ < SIZE &&
			    !taken[v_])
				break;
			puts("Invalid move!");
			clearerr(stdin);
			if (scanf("%*[^\n]") < 0)
				return;
		}
		for (int i = 0; i < PIECES; ++i)
			if (p.white[i] == v)
				p.white[i] = v_;
		sort4(p.white);
		dump_position(&p);
		if (is_terminal(p.white)) {
			puts("You won!");
			return;
		}
		ai_play(&p);
		dump_position(&p);
		if (is_terminal(p.black)) {
			puts("I won!");
			return;
		}
	}
}


