.PHONY:clean

CC = g++
CCFLAGS = -std=c++17 -lpthread -Wall -g
SOURCE = Server.cc Pool.cc

all: $(SOURCE)
	$(CC) $(SOURCE) -o server $(CCFLAGS)

clean:
	rm -f server
