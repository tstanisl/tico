#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PIECES 4
#define SIZE 5

#define SWAP(type,a,b) \
	do { type __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

struct position {
	uint8_t white[PIECES];
	uint8_t black[PIECES];
	int n_children;
	bool finalized;
	struct position *next;
};

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
	putchar('\n');

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
		if (!is_terminal(p->white))
			dump_position(p);
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
	struct position p = {};
	gen_terminals_black(&p, 0);
	return 0;
}

