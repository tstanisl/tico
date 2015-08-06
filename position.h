#ifndef POSITION_H

#include <stdbool.h>
#include <stdint.h>

#define PIECES 4
#define SIZE 5

struct position {
	uint8_t white[PIECES];
	uint8_t black[PIECES];
	int n_children;
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

#endif
