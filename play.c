#include "play.h"
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

static void make_random_board(struct position *p)
{
	srand(time(0));
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

	for (;;) {
		int ret;

		ret = white->cb(white, &p);
		if (ret != 0)
			return;

		if (is_terminal(p.white)) {
			dump_position(&p);
			puts("White won!");
			return;
		}

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
}


