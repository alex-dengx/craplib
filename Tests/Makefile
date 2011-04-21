CC = g++
CPPFLAGS = -std=gnu++98 -pedantic -DDEBUG

all:
	@echo -e "Compiling Tests..."
	@$(CC) $(CPPFLAGS) -o Tests Tests.cpp -L.. -I.. -lcrap -lpthread 2> /dev/null

clean:
	@rm Tests
