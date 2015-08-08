#include "play.h"
#include "position.h"

#include <limits.h>
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
int n_phase1;
int n_phase1_done;

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
	fprintf(stderr, "n_phase1=%d\n", n_phase1);
	fprintf(stderr, "n_phase1_done=%d\n", n_phase1_done);
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
	n->state = PS_UNKNOWN;
	n->n_children = count_children(n);
	n->terminal_distance = INT_MAX;
	if (is_phase1_position(n))
		++n_phase1;
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

struct position *find_node(struct position *p)
{
	uint32_t hash = hash_position(p);
	struct position **head = &htab[hash & (HSIZE - 1)];
	for (struct position *n = *head; n; n = n->next)
		if (equal_position(p, n))
			return n;
	return NULL;
}

struct position *make_node_tmp(struct position *p)
{
	static struct position tmp;
	struct position *ret = find_node(p);
	if (ret)
		return ret;
	tmp = *p;
	tmp.state = PS_UNKNOWN;
	tmp.n_children = count_children(&tmp);
	tmp.terminal_distance = INT_MAX;
	return &tmp;
}

int eval_position(struct position *p)
{
	if (p->state == PS_WIN)
		return 256 - p->terminal_distance;
	if (p->state == PS_LOSE)
		return -255 + p->terminal_distance;
	return 0;
}

int ai_best_count;
struct position ai_best;

void ai_play_handler(struct position *p)
{
	//puts("---- checking");
	//printf("ai_best = %p\n", ai_best);
	p = make_node_tmp(p);
	if (ai_best_count == 0) {
		ai_best_count = 1;
		ai_best = *p;
		return;
	}

	int cmp = eval_position(&ai_best) - eval_position(p);
	if (cmp > 0) {
		ai_best_count = 1;
		ai_best = *p;
	} else if (cmp == 0) {
		if (rand() % (ai_best_count + 1) == 0)
			ai_best = *p;
		++ai_best_count;
	}
	//dump_position_short(p);
}

int ai_perfect_player_cb(struct player_fo *fo, struct position *p)
{
	(void)fo;
	/*struct position *n = find_node(p);
	if (!n)
		puts("unknown node");
	else
		printf("n_children = %d\n", n->n_children);*/
	ai_best_count = 0;
	foreach_child(p, ai_play_handler, false);
	if (ai_best_count == 0) {
		puts("I failed to find any valid move.");
		return -1;
	}
	if (ai_best.state == PS_LOSE)
		puts("I think I'm winning.");
	if (ai_best.state == PS_WIN)
		puts("I think I've lost.");
	if (ai_best.state == PS_UNKNOWN)
		puts("I am confused.");

	memcpy(p->white, ai_best.black, PIECES);
	//dump_position_short(ai_best);
	//printf("ai_best->n_children = %d\n", ai_best->n_children);
	return 0;
}

void gen_terminals_white(struct position *p, int piece)
{
	if (piece == PIECES) {
		/*static int cnt = 0;
		if (++cnt % 10000 == 0)
			dump_stat();*/
		if (!is_terminal(p->white)) {
			++n_terminal;
			make_node(p);
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

static struct position **queue;
static int wr, rd;

static void enqueue_node(struct position *p)
{
	if (wr >= POOLSIZE) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		exit(-1);
	}
	queue[wr++] = p;
	p->state = PS_QUEUED;
}

void make_win_handler(struct position *child)
{
	child = make_node(child);
	if (child->state != PS_UNKNOWN)
		return;
	child->n_children--;
	if (child->n_children == 0)
		enqueue_node(child);
}

void make_lose_handler(struct position *child)
{
	child = make_node(child);
	if (child->state != PS_UNKNOWN)
		return;
	enqueue_node(child);
}

struct player_fo *ai_perfect_init(void)
{
	positions = malloc(POOLSIZE * sizeof positions[0]);
	if (positions == NULL)
		return NULL;
	struct position p = { .state = PS_UNKNOWN };
	gen_terminals_black(&p, 0);
	dump_stat();

	queue = malloc(POOLSIZE * sizeof queue[0]);
	if (queue == NULL)
		return NULL;

	wr = rd = 0;

	int n_terminals = n_positions;
	for (int i = 0; i < n_terminals; ++i)
		enqueue_node(&positions[i]);

	int dist = -1, up_rd = 0;
	while (rd < wr) {
		if (rd == up_rd) {
			++dist;
			fprintf(stderr, "set %d nodes to distance %d\n",
				wr - up_rd, dist);
			up_rd = wr;
		}

		struct position *p = queue[rd++];
		if (p->state != PS_QUEUED)
			continue;

		p->terminal_distance = dist;
		if (p->n_children == 0) { // dealing with lose node
			++n_lose;
			p->state = PS_LOSE;
			foreach_child(p, make_lose_handler, true);
		} else {
			++n_win;
			p->state = PS_WIN;
			foreach_child(p, make_win_handler, true);
		}
		if (is_phase1_position(p))
			++n_phase1_done;
	}
	dump_stat();
	return &ai_perfect_player;
}

void ai_random_handler(struct position *p)
{
	if (rand() % (ai_best_count + 1) == 0)
		ai_best = *p;
	++ai_best_count;
}

int ai_random_player_cb(struct player_fo *fo, struct position *p)
{
	(void)fo;
	ai_best_count = 0;
	foreach_child(p, ai_random_handler, false);
	if (ai_best_count == 0) {
		puts("I failed to find any valid move.");
		return -1;
	}
	memcpy(p->white, ai_best.black, PIECES);
	return 0;
}

struct player_fo ai_random = { .cb = ai_random_player_cb };
