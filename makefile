# makefile

all: shell

shell: shell.cpp
	g++ -g -w -std=c++14 -o shell shell.cpp -lpthread -lrt

clean:
	rm -rf *.o shell
