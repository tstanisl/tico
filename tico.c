#include "ai_perfect.h"
#include "play.h"
#include "position.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	struct player_fo *ai_perfect = ai_perfect_init();
	if (!ai_perfect) {
		fprintf(stderr, "ai_perfect_init() failed\n");
		return -1;
	}
	srand(time(0));
	for (;;)
		play(ai_perfect, &user_player);
	return 0;
}

