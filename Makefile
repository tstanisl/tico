TARGET  := tico
SRCS    := \
	play.c\
	position.c \
	tico.c

CC = gcc
CFLAGS = -std=gnu99 -O3 -Wall -pedantic -g3
LDFLAGS = -rdynamic
LIBS    = 

OBJS    := ${SRCS:.c=.o} 
DEPS    := ${SRCS:.c=.dep} 
XDEPS   := $(wildcard ${DEPS}) 


.PHONY: all clean distclean 
all:: ${TARGET} 

ifneq (${XDEPS},) 
include ${XDEPS} 
endif 

${TARGET}: ${OBJS} 
	${CC} -o $@ $^ ${LIBS} ${LDFLAGS} 

${OBJS}: %.o: %.c %.dep 
	${CC} ${CFLAGS} -o $@ -c $< 

${DEPS}: %.dep: %.c Makefile 
	${CC} ${CFLAGS} -MM $< > $@

clean:
	rm ${OBJS}
	rm ${TARGET}
	rm ${DEPS}

distclean: clean

