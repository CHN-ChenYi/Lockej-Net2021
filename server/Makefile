.PHONY:clean

CC = g++
CCFLAGS = -lpthread -Wall
SOURCE = Server.cc Pool.cc

all: $(SOURCE)
	$(CC) $(SOURCE) -o server $(CCFLAGS)

clean:
	rm -f server
