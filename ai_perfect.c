#include "play.h"
#include "position.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HSIZE (1 << 27)
#define POOLSIZE (HSIZE - HSIZE / 4)

struct position *htab[HSIZE];
struct position *positions;
int n_terminal;
int n_positions;
int n_lose;
int n_win;
int n_htab;

uint32_t hash_position(struct position *p)
{
	uint32_t hash = 0;
	const uint32_t mult = 521893723u;
	//const uint32_t mult = 37u;
	for (int i = 0; i < PIECES; ++i) {
		hash = mult * hash;
		hash += p->white[i];
		hash = mult * hash;
		hash += p->black[i];
		//hash = 912839121u * hash + p->black[i];
	}
	//hash = mult * hash;
#if 0
	uint32_t *a = (uint32_t*)p->white;
	uint32_t *b = (uint32_t*)p->black;
	uint32_t hash = *a * 521893723u + *b * 912839121u;
	hash ^= hash >> 17;
#endif
	return hash;
}

void dump_stat(void)
{
	fprintf(stderr, "n_term=%d\n", n_terminal);
	fprintf(stderr, "n_win=%d\n", n_win);
	fprintf(stderr, "n_lose=%d\n", n_lose);
	fprintf(stderr, "n_all=%d\n", n_positions);
	fprintf(stderr, "n_htab=%d\n", n_htab);
	fprintf(stderr, "pool=%lld %%\n", 100LL * n_positions / POOLSIZE);
}

static bool equal_position(struct position *p, struct position *n)
{
	/*return (memcmp(p->white, n->white, sizeof p->white) == 0 &&
	    memcmp(p->black, n->black, sizeof p->black) == 0);*/
	return memcmp(p, n, 2 * PIECES) == 0;
}

struct position *make_node(struct position *p)
{
	uint32_t hash = hash_position(p);
	struct position **head = &htab[hash & (HSIZE - 1)];
	for (struct position *n = *head; n; n = n->next)
		if (equal_position(p, n))
			return n;
	if (n_positions == POOLSIZE) {
		fprintf(stderr, "out of memory\n");
		exit(-1);
	}

	if (*head == NULL)
		++n_htab;
	struct position *n = &positions[n_positions++];
	memcpy(n, p, sizeof *p);
	n->next = *head;
	*head = n;
	n->n_children = count_children(n);
	/*if (is_terminal(n->black))
		make_lose_node(n);*/
	/*if (terminal)
		n->n_children = 0;
	else
		n->n_children = count_children(n);*/
	if (n_positions % 1000000 == 0)
		dump_stat();
	return n;
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

int ai_perfect_player_cb(struct player_fo *fo, struct position *p)
{
	(void)fo;
	/*struct position *n = find_node(p);
	if (!n)
		puts("unknown node");
	else
		printf("n_children = %d\n", n->n_children);*/
	ai_best = NULL;
	foreach_child(p, ai_play_handler, false);
	if (!ai_best) {
		puts("I failed.");
		return -1;
	}
	if (ai_best->n_children == 0)
		puts("I think I'm winning.");
	if (ai_best->n_children == -1)
		puts("I think I've lost.");
	if (ai_best->n_children > 0)
		puts("I am confused.");

	memcpy(p->white, ai_best->black, PIECES);
	//printf("ai_best->n_children = %d\n", ai_best->n_children);
	return 0;
}

void make_lose_node(struct position *n);

void make_win_node_helper(struct position *child)
{
	child = make_node(child);
	if (child->n_children <= 0)
		return;
	child->n_children--;
	if (child->n_children == 0)
		make_lose_node(child);
}

void make_win_node(struct position *n)
{
	++n_win;
	n->n_children = -1;
	//puts("-- winner ---"); dump_position(n);
	foreach_child(n, make_win_node_helper, true);
}

void make_lose_node_helper(struct position *child)
{
	child = make_node(child);
	if (child->n_children > 0)
		make_win_node(child);
}

void make_lose_node(struct position *n)
{
	++n_lose;
	n->n_children = 0;
	//puts("-- loser ---"); dump_position(n);
	foreach_child(n, make_lose_node_helper, true);
}

struct position *find_node(struct position *p)
{
	uint32_t hash = hash_position(p);
	struct position **head = &htab[hash & (HSIZE - 1)];
	for (struct position *n = *head; n; n = n->next)
		if (equal_position(p, n))
			return n;
	return NULL;
}

void gen_terminals_white(struct position *p, int piece)
{
	if (piece == PIECES) {
		/*static int cnt = 0;
		if (++cnt % 10000 == 0)
			dump_stat();*/
		if (!is_terminal(p->white)) {
			//puts("term"); dump_position(p);
			++n_terminal;
			struct position *n = make_node(p);
			n->n_children = 0;
			//make_lose_node(n);
		}
		return;
	}
	int start = piece ? p->white[piece - 1] + 1 : 0;
	int last = SIZE * SIZE - PIECES + piece;
	for (int i = start; i <= last; ++i) {
		// check if position is not used by whites
		bool ok = true;
		for (int j = 0; ok && j < PIECES; ++j)
			if (p->black[j] == i)
				ok = false;

		if (ok) {
			p->white[piece] = i;
			gen_terminals_white(p, piece + 1);
		}
	}
}

void gen_terminals_black(struct position *p, int piece)
{
	if (piece == PIECES) {
		if (is_terminal(p->black)) {
			//dump_position(p);
			gen_terminals_white(p, 0);
		}
		return;
	}
	int start = piece ? p->black[piece - 1] + 1 : 0;
	int last = SIZE * SIZE - PIECES + piece;
	for (int i = start; i <= last; ++i) {
		p->black[piece] = i;
		gen_terminals_black(p, piece + 1);
	}
}

struct player_fo ai_perfect_player = { .cb = ai_perfect_player_cb };

struct player_fo *ai_perfect_init(void)
{
	positions = malloc(POOLSIZE * sizeof positions[0]);
	if (positions == NULL)
		return NULL;
	struct position p = {};
	gen_terminals_black(&p, 0);
	dump_stat();
	int n_terminals = n_positions;
	for (int i = 0; i < n_terminals; ++i)
		make_lose_node(&positions[i]);
	return &ai_perfect_player;
}

