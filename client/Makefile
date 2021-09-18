.PHONY:clean

CC = g++
CCFLAGS = -lpthread -Wall
SOURCE = client.cc

all: $(SOURCE)
	$(CC) $(SOURCE) -o client $(CCFLAGS)

clean:
	rm -f client
