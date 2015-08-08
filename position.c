#include "position.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_terminal(uint8_t A[PIECES])
{
	// phase I position is never a terminal
	if (A[PIECES - 1] == EMPTY)
		return false;

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

int count_empty(struct position *p)
{
	int n_empty = 0;
	for (int i = 0; i < PIECES; ++i) {
		if (p->white[i] == EMPTY)
			++n_empty;
		if (p->black[i] == EMPTY)
			++n_empty;
	}
	return n_empty;
}

int count_children(struct position *p)
{
	// terminal have no children
	if (is_terminal(p->black))
		return 0;

	int n_empty = count_empty(p);
	if (n_empty)
		return SIZE * SIZE - (2 * PIECES - n_empty);

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
		if (p->white[i] != EMPTY)
			taken[p->white[i]] = true;
		if (p->black[i] != EMPTY)
			taken[p->black[i]] = true;
	}
}

#define SWAP(type,a,b) \
	do { type __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

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

void foreach_child_phase1(struct position *p, foreach_child_cb cb)
{
	bool taken[SIZE * SIZE];
	gen_taken(taken, p);

	struct position child;
	memcpy(child.white, p->black, PIECES);
	for (int i = 0; i < SIZE * SIZE; ++i) {
		if (taken[i])
			continue;
		memcpy(child.black, p->white, PIECES);
		child.black[PIECES - 1] = i;
		sort4(child.black);
		cb(&child);
	}
}

void foreach_child_phase1_reverse(struct position *p, foreach_child_cb cb)
{
	struct position child;
	memcpy(child.black, p->white, PIECES);
	for (int i = 0; i < PIECES; ++i) {
		if (p->black[i] == EMPTY)
			return;
		memcpy(child.white, p->black, PIECES);
		child.white[i] = EMPTY;
		sort4(child.white);
		cb(&child);
	}
}

void foreach_child(struct position *p, foreach_child_cb cb, bool reverse)
{
	if (is_phase1_position(p)) {
		if (reverse)
			foreach_child_phase1_reverse(p, cb);
		else
			foreach_child_phase1(p, cb);
		return;
	}

	bool taken[SIZE * SIZE];
	gen_taken(taken, p);

	uint8_t *white, *black;
	uint8_t *child_white, *child_black;
	struct position child;
	if (reverse) {
		white = p->black;
		black = p->white;
		child_white = child.black;
		child_black = child.white;
	} else {
		white = p->white;
		black = p->black;
		child_white = child.white;
		child_black = child.black;
	}
	memcpy(child_white, black, PIECES);
	for (int i = 0; i < PIECES; ++i) {
		int v = white[i];
		int x = v % SIZE;
		int y = v / SIZE;
		if (x > 0 && !taken[v - 1]) {
			memcpy(child_black, white, PIECES);
			child_black[i] = v - 1;
			sort4(child_black);
			cb(&child);
		}
		if (x + 1 < SIZE && !taken[v + 1]) {
			memcpy(child_black, white, PIECES);
			child_black[i] = v + 1;
			sort4(child_black);
			cb(&child);
		}

		if (y > 0 && !taken[v - SIZE]) {
			memcpy(child_black, white, PIECES);
			child_black[i] = v - SIZE;
			sort4(child_black);
			cb(&child);
		}

		if (y + 1 < SIZE && !taken[v + SIZE]) {
			memcpy(child_black, white, PIECES);
			child_black[i] = v + SIZE;
			sort4(child_black);
			cb(&child);
		}

		if (reverse) { // checking phase 1 move
			memcpy(child_black, white, PIECES);
			child_black[i] = EMPTY;
			sort4(child_black);
			cb(&child);
		}
	}
}

void dump_position_short(struct position *p)
{
	for (int i = 0; i < PIECES; ++i)
		printf("%d ", p->white[i]);
	putchar(' ');
	for (int i = 0; i < PIECES; ++i)
		printf("%d ", p->black[i]);
	printf("%c d=%d\n", "UWLQ"[p->state], p->terminal_distance);
}

void dump_position(struct position *p)
{
	dump_position_short(p);

	char dump[SIZE * SIZE];
	memset(dump, '.', sizeof dump);

	for (int i = 0; i < PIECES; ++i)
		if (p->white[i] != EMPTY)
			dump[p->white[i]] = '@';

	for (int i = 0; i < PIECES; ++i)
		if (p->black[i] != EMPTY)
			dump[p->black[i]] = 'O';

	for (int i = 0; i < SIZE * SIZE; ++i) {
		putchar(dump[i]);
		if ((i + 1) % SIZE == 0)
			putchar('\n');
	}
	putchar('\n');
}

void swap_position(struct position *p)
{
	uint8_t tmp[PIECES];
	memcpy(tmp, p->white, PIECES);
	memcpy(p->white, p->black, PIECES);
	memcpy(p->black, tmp, PIECES);
}

void assert_position(struct position *p)
{
	for (int i = 1; i < PIECES; ++i)
		if (p->white[i - 1] >= p->white[i])
			abort();
	for (int i = 1; i < PIECES; ++i)
		if (p->black[i - 1] >= p->black[i])
			abort();
	for (int i = 0; i < PIECES; ++i)
		if (p->white[i] >= SIZE * SIZE)
			abort();
	for (int i = 0; i < PIECES; ++i)
		if (p->black[i] >= SIZE * SIZE)
			abort();
}
