CFLAGS	= -Wall -Werror --pedantic -std=c++11 -g
CC		= g++
COBJS	= client.o
SOBJS	= server.o
srcs	= $(wildcard *.cpp)
deps	= $(srcs:.cpp=.d)

all		: client server

client	: 	$(COBJS)
		$(CC) -o $@ $(COBJS)

%.o: %.cpp
		$(CC) -MMD -MP $(CFLAGS) -c $< -o $@

server	:	server.o
		$(CC) -o $@ server.o

.PHONY: clean

mop:
	$(RM) $(COBJS) $(SOBJS) $(deps)
clean: mop
	$(RM) a.out core

-include $(deps)
