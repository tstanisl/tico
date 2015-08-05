#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIECES 4
#define SIZE 5

#define SWAP(type,a,b) \
	do { type __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

struct position {
	uint8_t white[PIECES];
	uint8_t black[PIECES];
	int n_children;
	struct position *next;
};

#define HSIZE (1 << 25)
#define POOLSIZE (4 * HSIZE)

struct position *htab[HSIZE];
struct position *positions;
int n_positions;
int n_lose;
int n_win;

uint32_t hash_position(struct position *p)
{
	uint32_t *a = (uint32_t*)p->white;
	uint32_t *b = (uint32_t*)p->black;
	return *a * 521893723u + *b * 912839121u;
}

void sort4(uint8_t A[PIECES])
{
#define SORT2(a,b) if (a > b) SWAP(uint8_t,a,b)
	SORT2(A[0], A[2]);
	SORT2(A[1], A[3]);
	SORT2(A[0], A[1]);
	SORT2(A[2], A[3]);
	SORT2(A[1], A[2]);
#undef SORT2
}

bool is_terminal(uint8_t A[PIECES])
{
	// assume A is sorted
	// column check
	if (A[1] - A[0] == SIZE &&
	    A[2] - A[1] == SIZE &&
	    A[3] - A[2] == SIZE)
		return true; 

	// row check
	uint8_t x = A[0] % SIZE;
	if (A[1] - A[0] == 1 &&
	    A[2] - A[1] == 1 &&
	    A[3] - A[2] == 1 &&
	    x <= SIZE - PIECES)
		return true; 

	// slash check
	uint8_t y = A[0] / SIZE;
	if (A[1] - A[0] == SIZE - 1 &&
	    A[2] - A[1] == SIZE - 1 &&
	    A[3] - A[2] == SIZE - 1 &&
	    x >= PIECES - 1 && y <= SIZE - PIECES)
		return true; 

	// backslash check
	if (A[1] - A[0] == SIZE + 1 &&
	    A[2] - A[1] == SIZE + 1 &&
	    A[3] - A[2] == SIZE + 1 &&
	    x <= SIZE - PIECES && y <= SIZE - PIECES)
		return true; 

	// square check
	uint8_t w = A[1] - A[0];
	if (w < SIZE - x &&
	    A[2] == A[0] + w * SIZE && 
	    A[3] == A[1] + w * SIZE)
		return true; 

	return false;
}

int count_children(struct position *p)
{
	bool taken[SIZE * SIZE];
	memset(taken, 0, sizeof taken);
	for (int i = 0; i < PIECES; ++i) {
		taken[p->white[i]] = true;
		taken[p->black[i]] = true;
	}

	int count = 0;
	for (int i = 0; i < PIECES; ++i) {
		int v = p->white[i];
		int x = v % SIZE;
		int y = v / SIZE;
		if (x > 0 && !taken[v - 1])
			++count;
		if (x + 1 < SIZE && !taken[v + 1])
			++count;
		if (y > 0 && !taken[v - SIZE])
			++count;
		if (y + 1 < SIZE && !taken[v + SIZE])
			++count;
	}
	return count;
}

typedef void (*foreach_child_cb)(struct position *);
void foreach_child(struct position *p, foreach_child_cb cb)
{
	bool taken[SIZE * SIZE];
	memset(taken, 0, sizeof taken);
	for (int i = 0; i < PIECES; ++i) {
		taken[p->white[i]] = true;
		taken[p->black[i]] = true;
	}

	struct position child;
	memcpy(child.black, p->white, sizeof p->white);
	for (int i = 0; i < PIECES; ++i) {
		int v = p->black[i];
		int x = v % SIZE;
		int y = v / SIZE;
		if (x > 0 && !taken[v - 1]) {
			memcpy(child.white, p->black, sizeof p->black);
			child.white[i] = v - 1;
			sort4(child.white);
			cb(&child);
		}
		if (x + 1 < SIZE && !taken[v + 1]) {
			memcpy(child.white, p->black, sizeof p->black);
			child.white[i] = v + 1;
			sort4(child.white);
			cb(&child);
		}

		if (y > 0 && !taken[v - SIZE]) {
			memcpy(child.white, p->black, sizeof p->black);
			child.white[i] = v - SIZE;
			sort4(child.white);
			cb(&child);
		}

		if (y + 1 < SIZE && !taken[v + SIZE]) {
			memcpy(child.white, p->black, sizeof p->black);
			child.white[i] = v + SIZE;
			sort4(child.white);
			cb(&child);
		}
	}
}

void make_lose_node(struct position *n);
struct position *make_node(struct position *p);

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
	foreach_child(n, make_win_node_helper);
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
	foreach_child(n, make_lose_node_helper);
}

struct position *make_node(struct position *p)
{
	uint32_t hash = hash_position(p);
	struct position **head = &htab[hash & (HSIZE - 1)];
	for (struct position *n = *head; n; n = n->next)
		if (memcmp(p->white, n->white, sizeof p->white) == 0 &&
		    memcmp(p->black, n->black, sizeof p->black) == 0)
			return n;
	if (n_positions == POOLSIZE) {
		fprintf(stderr, "out of memory\n");
		exit(-1);
	}

	struct position *n = &positions[n_positions++];
	memcpy(n, p, sizeof *p);
	n->next = *head;
	*head = n;
	n->n_children = count_children(n);
	/*if (terminal)
		n->n_children = 0;
	else
		n->n_children = count_children(n);*/
	return n;
}

void dump_position(struct position *p)
{
	static int cnt = 0;
	if (++cnt % 10000 == 0)
		fprintf(stderr, "count=%d\n", cnt);
	char dump[SIZE * SIZE];
	memset(dump, '.', sizeof dump);

	for (int i = 0; i < PIECES; ++i)
		dump[p->white[i]] = '@';

	for (int i = 0; i < PIECES; ++i)
		dump[p->black[i]] = 'O';

	for (int i = 0; i < PIECES; ++i)
		printf("%d ", p->white[i]);
	putchar('|');
	for (int i = 0; i < PIECES; ++i)
		printf("%d ", p->black[i]);
	//putchar('\n');
	printf("| %d\n", count_children(p));

	for (int i = 0; i < SIZE * SIZE; ++i) {
		putchar(dump[i]);
		if ((i + 1) % SIZE == 0)
			putchar('\n');
	}
	putchar('\n');
}

void gen_terminals_white(struct position *p, int piece)
{
	if (piece == PIECES) {
		static int cnt = 0;
		if (++cnt % 10000 == 0) {
			fprintf(stderr, "count=%d\n", cnt);
			fprintf(stderr, "n_win=%d\n", n_win);
			fprintf(stderr, "n_lose=%d\n", n_lose);
			fprintf(stderr, "n_all=%d\n", n_positions);
			fprintf(stderr, "pool=%lld p/c\n",
				100LL * n_positions / POOLSIZE);
		}
		if (!is_terminal(p->white)) {
			//dump_position(p);
			struct position *n = make_node(p);
			make_lose_node(n);
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
		if (is_terminal(p->black))
			gen_terminals_white(p, 0);
		return;
	}
	int start = piece ? p->black[piece - 1] + 1 : 0;
	int last = SIZE * SIZE - PIECES + piece;
	for (int i = start; i <= last; ++i) {
		p->black[piece] = i;
		gen_terminals_black(p, piece + 1);
	}
}

int main()
{
	positions = malloc(POOLSIZE * sizeof positions[0]);
	struct position p = {};
	gen_terminals_black(&p, 0);
	return 0;
}

