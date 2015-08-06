#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PIECES 4
#define SIZE 4

#define SWAP(type,a,b) \
	do { type __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

struct position {
	uint8_t white[PIECES];
	uint8_t black[PIECES];
	int n_children;
	struct position *next;
};

#define HSIZE (1 << 27)
#define POOLSIZE (HSIZE)

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

void gen_taken(bool taken[SIZE * SIZE], struct position *p)
{
	memset(taken, 0, SIZE * SIZE * sizeof taken[0]);
	for (int i = 0; i < PIECES; ++i) {
		taken[p->white[i]] = true;
		taken[p->black[i]] = true;
	}
}

typedef void (*foreach_child_cb)(struct position *);
void foreach_child(struct position *p, foreach_child_cb cb)
{
	bool taken[SIZE * SIZE];
	gen_taken(taken, p);

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
	//puts("-- winner ---"); dump_position(n);
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
	//puts("-- loser ---"); dump_position(n);
	foreach_child(n, make_lose_node_helper);
}

static bool equal_position(struct position *p, struct position *n)
{
	/*return (memcmp(p->white, n->white, sizeof p->white) == 0 &&
	    memcmp(p->black, n->black, sizeof p->black) == 0);*/
	return memcmp(p, n, 2 * PIECES) == 0;
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
	if (is_terminal(n->black))
		make_lose_node(n);
	/*if (terminal)
		n->n_children = 0;
	else
		n->n_children = count_children(n);*/
	return n;
}

void dump_position(struct position *p)
{
	char dump[SIZE * SIZE];
	memset(dump, '.', sizeof dump);

	for (int i = 0; i < PIECES; ++i)
		dump[p->white[i]] = '@';

	for (int i = 0; i < PIECES; ++i)
		dump[p->black[i]] = 'O';

	for (int i = 0; i < PIECES; ++i)
		printf("%d ", p->white[i]);
	putchar(' ');
	for (int i = 0; i < PIECES; ++i)
		printf("%d ", p->black[i]);
	if (p->n_children == 0)
		putchar('L');
	else if (p->n_children == -1)
		putchar('W');
	else
		putchar('U');
	putchar('\n');
	//printf("| %d\n", count_children(p));

	for (int i = 0; i < SIZE * SIZE; ++i) {
		putchar(dump[i]);
		if ((i + 1) % SIZE == 0)
			putchar('\n');
	}
	putchar('\n');
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

void gen_terminals_white(struct position *p, int piece)
{
	if (piece == PIECES) {
		static int cnt = 0;
		if (++cnt % 10000 == 0)
			dump_stat();
		if (!is_terminal(p->white)) {
			//dump_position(p);
			++n_terminal;
			struct position *n = make_node(p);
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
	puts("---- checking");
	//printf("ai_best = %p\n", ai_best);
	p = make_node(p);
	dump_position(p);
	if (p->n_children == -1) {
		// found winning move
		ai_best = p;
		return;
	}

	if (p->n_children > 0) {
		// found undefined move 
		if (!ai_best || ai_best->n_children != -1)
			ai_best = p;
		return;
	}
	ai_best = p;
}

void ai_play(struct position *p)
{
	struct position *n = find_node(p);
	if (!n)
		puts("unknown node");
	else
		printf("n_children = %d\n", n->n_children);
	ai_best = NULL;
	foreach_child(p, ai_play_handler);
	if (!ai_best) {
		puts("I failed.");
		exit(-1);
	}
	if (ai_best->n_children == -1)
		puts("I think I'm winning.");
	if (ai_best->n_children == 0)
		puts("I think I've lost.");
	if (ai_best->n_children > 0)
		puts("I am confused.");

	memcpy(p->black, ai_best->white, PIECES);
	printf("ai_best->n_children = %d\n", ai_best->n_children);
}

void play()
{
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

int main()
{
	positions = malloc(POOLSIZE * sizeof positions[0]);
	struct position p = {};
	gen_terminals_black(&p, 0);
	dump_stat();
#if 0
	for (int i = 0; i < n_positions; ++i)
		if (positions[i].n_children <= 0)
			dump_position(&positions[i]);
#endif
	play();
	return 0;
}

