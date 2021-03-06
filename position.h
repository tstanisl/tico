#ifndef POSITION_H

#include <stdbool.h>
#include <stdint.h>

#define PIECES 4
#define SIZE 5
#define EMPTY (SIZE * SIZE + 1)

enum position_state {
	PS_UNKNOWN = 0,
	PS_WIN = 1,
	PS_LOSE = 2,
	PS_QUEUED = 3,
};

struct position {
	uint8_t white[PIECES];
	uint8_t black[PIECES];
	uint8_t state;
	uint8_t n_children;
	int terminal_distance;
	struct position *next;
};

bool is_terminal(uint8_t A[PIECES]);
void dump_position(struct position *p);
void dump_position_short(struct position *p);
int count_children(struct position *p);
typedef void (*foreach_child_cb)(struct position *);
void foreach_child(struct position *p, foreach_child_cb cb, bool reverse);
void gen_taken(bool taken[SIZE * SIZE], struct position *p);
void sort4(uint8_t A[PIECES]);
void swap_position(struct position *p);
void assert_position(struct position *p);
static inline bool is_phase1_position(struct position *p)
{
	return p->white[PIECES - 1] == EMPTY;
}

#endif
