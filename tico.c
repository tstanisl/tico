#include "play.h"
#include "position.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
#if 0
	for (int i = 0; i < n_positions; ++i)
		if (positions[i].n_children <= 0)
			dump_position(&positions[i]);
#endif
	for (;;)
		play(&user_player, &user_player);
	return 0;
}

