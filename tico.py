positions = set()

size = 5
n_pieces = 4

trans = (\
	lambda x: (x[0], x[1]), \
	lambda x: (x[0], size-1-x[1]), \
	lambda x: (size-1-x[0], x[1]), \
	lambda x: (size-1-x[0], size-1-x[1]), \
	lambda x: (x[1], x[0]), \
	lambda x: (x[1], size-1-x[0]), \
	lambda x: (size-1-x[1], x[0]), \
	lambda x: (size-1-x[1], size-1-x[0]))

def uniq_position(position):
	#print('pos = ', position)
	best = list(position)
	for t in trans:
		#print(t((0,1,)))
		position_ = []
		for p in position:
			position_.append(t(p))
		#print(position_)
		best = min(best, sorted(position_))

	#print('best = {}'.format(best))
	return tuple(best)
	
def gen_position(prefix):
	if len(prefix) == n_pieces:
		#check if is used
		position = uniq_position(prefix)
		positions.add(position)
		#print('{} -> {}'.format(prefix, add_position(prefix)))
		return
	for x in range(size):
		for y in range(size):
			piece = (x,y)
			if piece in prefix:
				continue
			gen_position(prefix + (piece,))
			

gen_position(())
for p in sorted(positions):
	print(' '.join(map(lambda x: str(x[1] + size * x[0]), p)))
