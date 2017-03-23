CC = g++
CFLAGS = -std=c++11 -lcurl

all: main

main: site-tester.cpp
	$(CC) $(CFLAGS) site-tester.cpp -o main

clean:
	rm -f main
	rm -r main*
