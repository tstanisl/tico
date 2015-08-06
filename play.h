#ifndef PLAY_H
#define PLAY_H

struct position;
struct player_fo {
	int (*cb)(struct player_fo *fo, struct position *p);
};

extern struct player_fo user_player;

void play(struct player_fo *white, struct player_fo *black);

#endif
