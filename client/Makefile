.PHONY:clean

CC = g++
CCFLAGS = -lpthread -Wall -std=c++17
SOURCE = client.cc

all: $(SOURCE)
	$(CC) $(SOURCE) -o client $(CCFLAGS) -DHEARTBEAT

naive: $(SOURCE)
	$(CC) $(SOURCE) -o client $(CCFLAGS)

clean:
	rm -f client
